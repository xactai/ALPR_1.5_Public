#include "TFFmpeg.h"

using namespace std;
//----------------------------------------------------------------------------------
static void CustomLog(void *ptr, int level, const char* fmt, va_list vl)
{
    //only errors and fatals, no warnings
    if(level<24){
        string Str = fmt;
        //no error while decoding message
        if(Str.find("error while decoding") == string::npos){
            printf(fmt,vl);
        }
    }
};
//----------------------------------------------------------------------------------
TFFmpeg::TFFmpeg(void) : GrabFetch(true), GrabPause(false), GrabStop(false), ElapsedTime(0)
{
    CamName = "";
    decoder_ctx = avcodec_alloc_context3(NULL);
    decoder_ctx->opaque = this;
    Tag = PrevTag = 0;
}
//----------------------------------------------------------------------------------
TFFmpeg::~TFFmpeg(void)
{
    CloseFFmpeg();
}
//----------------------------------------------------------------------------------
int TFFmpeg::InitFFmpeg(const TCamInfo *Parent)
{
    int ret = 0;

    //get the required settings from ThreadCam
    PipeInfo = *Parent;
    FPS      = PipeInfo.CamFPS;
    AskedFPS = PipeInfo.CamFPS;
    CamName  = PipeInfo.CamName;

    if(AskedFPS<=0.0) AskedFPS=0.01;

    //FFmpeg av_log() callback
    av_log_set_callback(CustomLog);

    // open the input file
    ret = OpenFFmpeg();
    if(ret < 0) return ret;

    //print info
    StreamInfo(input_ctx);

    if(PipeInfo.CamLoop && (CamFrames==0)){
        PLOG_WARNING << CamName << " FFMPEG: Cannot rewind the video stream, play it only once.";
        return -1;
    }

    ThGr = std::thread([=] { FFmpegThread(); });

    return 0;
}
//----------------------------------------------------------------------------------------
void TFFmpeg::Pause(void)
{
     GrabPause.store(true);
}
//----------------------------------------------------------------------------------------
void TFFmpeg::Continue(void)
{
     GrabPause.store(false);
}
//----------------------------------------------------------------------------------
int TFFmpeg::OpenFFmpeg(void)
{
    // open the input file
    if(avformat_open_input(&input_ctx, PipeInfo.Device.c_str(), NULL, NULL) != 0) {
        PLOG_ERROR << CamName << " FFMPEG: Cannot open input device: " << PipeInfo.Device;
        return -1;
    }
#ifndef __aarch64__
    // get hardware acceleration
    type = av_hwdevice_find_type_by_name("cuda");
    if(type == AV_HWDEVICE_TYPE_NONE){
        PLOG_ERROR << CamName << " FFMPEG: No CUDA acceleration possible.";
        return -1;
    }
#endif
    // allocate packet
    packet = av_packet_alloc();
    if(!packet){
        PLOG_ERROR << CamName << " FFMPEG: Failed to allocate AVPacket.";
        return -1;
    }
    // find the stream info
    if(avformat_find_stream_info(input_ctx, NULL) < 0){
        PLOG_ERROR << CamName << " FFMPEG: Cannot find input stream information.";
        return -1;
    }
    // find the video stream information
    video_stream = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if(video_stream < 0){
        PLOG_ERROR << CamName << " FFMPEG: Cannot find a video stream in the input file.";
        return -1;
    }
#ifndef __aarch64__
    // check if suitable for HW
    for(int i=0; ;i++){
        const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
        if(!config){
            PLOG_ERROR << CamName << " FFMPEG: Decoder "<< decoder->name <<" does not support device type "<< av_hwdevice_get_type_name(type);
            return -1;
        }
        if(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type==type) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }
#endif
    // alloc space for the decoder
    void* Inst = decoder_ctx->opaque;
    if (!(decoder_ctx = avcodec_alloc_context3(decoder))){
        PLOG_ERROR << CamName << " FFMPEG: Cannot alloc space for the decoder.";
        return AVERROR(ENOMEM);
    }

#ifndef __aarch64__
    //restore the pointer (avcodec_alloc_context3 will reset it)
    decoder_ctx->opaque = Inst;
#endif

    AVStream *video = input_ctx->streams[video_stream];
    // fill the decoder with the found values
    if(avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0){
        PLOG_ERROR << CamName << " FFMPEG: Cannot fill the decoder.";
        return -1;
    }
    // info
    AVCodecParameters *codecParams = video->codecpar;
    CamWidth  = codecParams->width;
    CamHeight = codecParams->height;
    CamCodec  = avcodec_get_name(codecParams->codec_id);
    CamFrames = video->nb_frames;
    CamPixels = (int) decoder_ctx->pix_fmt;
    CamFPS    = av_q2d(video->r_frame_rate);
    if(CamFPS <= 0) {
        PLOG_ERROR << CamName << " FFMPEG: Cannot find the FPS of the stream.";
        return -1;
    }
    else{
        FPS=min(AskedFPS,CamFPS);
    }

#ifndef __aarch64__
    decoder_ctx->get_format = get_hw_format_static;  // AVCodecContext->get_format is a callback function

    if (av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0) < 0) {
        PLOG_ERROR << CamName << " FFMPEG: Failed to create specified HW device.";
        return -1;
    }
    decoder_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
#endif

//    if (avcodec_open2(decoder_ctx, decoder, NULL) < 0) {
    int ret = avcodec_open2(decoder_ctx, decoder, NULL);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        fprintf(stderr, "Error opening codec: %s\n", errbuf);

        PLOG_ERROR << CamName << " FFMPEG: Failed to open codec for stream "<< video_stream;
        return -1;
    }

    if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc())) {
        PLOG_ERROR << CamName << " FFMPEG: Can not alloc frame";
        av_frame_free(&frame);
        av_frame_free(&sw_frame);
        return -1;
    }

    pFrameRGB = av_frame_alloc();

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, decoder_ctx->width, decoder_ctx->height, 1);

    buffer = (uint8_t *)av_malloc( numBytes * sizeof(uint8_t) );

    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_BGR24, decoder_ctx->width, decoder_ctx->height, 1);

    TmpMat.create(decoder_ctx->height, decoder_ctx->width, CV_8UC3);

    // init the while loop
    Fraction = CamFPS / AskedFPS;
    if(Fraction<1.0){
        AskedFPS = CamFPS;
        Fraction = 1.0;
    }
    Isum = FrSkip = 0;
    Fsum = 0.0;

    return 0;
}
//----------------------------------------------------------------------------------
void TFFmpeg::StreamInfo(AVFormatContext *formatContext)
{
    for(unsigned int i = 0; i < formatContext->nb_streams; i++) {
        AVStream *stream = formatContext->streams[i];
        AVCodecParameters *codecParams = stream->codecpar;
        if (codecParams->codec_type == AVMEDIA_TYPE_VIDEO) {
            PLOG_INFO << CamName << " FFMPEG: Width: " << codecParams->width;
            PLOG_INFO << CamName << " FFMPEG: Height: " << codecParams->height;
            PLOG_INFO << CamName << " FFMPEG: FPS: " << av_q2d(stream->r_frame_rate);
            PLOG_INFO << CamName << " FFMPEG: Frames : " << CamFrames;
            if(codecParams->codec_id == AV_CODEC_ID_HEVC) {
                PLOG_INFO << CamName << " FFMPEG: Codec Type: h265";
            } else {
                PLOG_INFO << CamName << " FFMPEG: Codec Type: " << avcodec_get_name(codecParams->codec_id);
            }
            PLOG_INFO << CamName << " FFMPEG: Pixel Format: " << av_get_pix_fmt_name(decoder_ctx->pix_fmt);
        }
    }
}
//----------------------------------------------------------------------------------
bool TFFmpeg::GetFrame(cv::Mat& m)
{
    bool Success=false;
    std::mutex mtx;

    if(GrabTag.load()!=PrevTag){
        //get frame
        {
            std::lock_guard<std::mutex> lock(mtx);
            Smat.copyTo(m);
        }   //unlock()
        //update tag
        PrevTag=GrabTag.load();
        FileName = to_string(FrameIndex.load());
        //let the other thread know we are ready
        GrabFetch.store(true);
        Success= (!m.empty());
    }
    return Success;
}
//----------------------------------------------------------------------------------
void TFFmpeg::CloseFFmpeg(void)
{
    GrabStop.store(true);

    if(ThGr.joinable()){
        ThGr.join();                //wait until the grab thread ends
    }

    //clean up
    avformat_close_input(&input_ctx);
    av_packet_free(&packet);
    avcodec_free_context(&decoder_ctx);
    av_frame_free(&frame);
    av_frame_free(&sw_frame);
    av_frame_free(&pFrameRGB);
    av_free(buffer);
}
//----------------------------------------------------------------------------------
// Inside the Thread
//----------------------------------------------------------------------------------
void TFFmpeg::FFmpegThread(void)
{
    int ret = 0;
    slong64 FrameCount = 0;
    std::mutex mtx;

    Tag = 0;

    Tstart = std::chrono::steady_clock::now();

    while((ret >= 0) && (GrabStop.load() == false)) {
        if(GrabPause.load() == false){
            ret = av_read_frame(input_ctx, packet);
            if (packet->stream_index == video_stream) {
                if(avcodec_send_packet(decoder_ctx, packet) == 0) {
                    while(1) {
                        ret = avcodec_receive_frame(decoder_ctx, frame);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                            ret = 0; // AVERROR(EAGAIN) and AVERROR_EOF are not 'real' streaming errors, just end of packet.
                            break;
                        }
                        else{
                            if (ret < 0) break;
                        }
#ifdef __aarch64__
                        tmp_frame = frame;
#else
                        if (frame->format == hw_pix_fmt) {
                            // retrieve data from GPU to CPU
                            if((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
                                fprintf(stderr, "Error transferring the data to system memory\n");
                                break;
                            }
                            tmp_frame = sw_frame;
                        }
                        else tmp_frame = frame;
#endif
                        FrameCount++;
                        // adjust frame rate
                        if(FrSkip>0) FrSkip--;
                        else{
                            struct SwsContext* img_convert_ctx = sws_getContext(tmp_frame->width,
                                                                                tmp_frame->height,
                                                                                (enum AVPixelFormat)tmp_frame->format,
                                                                                tmp_frame->width,
                                                                                tmp_frame->height,
                                                                                AV_PIX_FMT_BGR24,
                                                                                SWS_FAST_BILINEAR,
                                                                                NULL,
                                                                                NULL,
                                                                                NULL);

                            sws_scale(img_convert_ctx, tmp_frame->data, tmp_frame->linesize, 0, tmp_frame->height, pFrameRGB->data, pFrameRGB->linesize);

                            sws_freeContext(img_convert_ctx);

                            // fill the frame data to the provided cv::Mat
                            memcpy(TmpMat.data, pFrameRGB->data[0], TmpMat.total() * TmpMat.elemSize());

                            {
                                std::lock_guard<std::mutex> lock(mtx);
                                // Resize the image if needed (keep TmpMat to its fixed sized)
                                if((PipeInfo.CamWidth!=tmp_frame->width) || (PipeInfo.CamHeight!=tmp_frame->height)){
                                    cv::resize(TmpMat, Smat, cv::Size(PipeInfo.CamWidth, PipeInfo.CamHeight));
                                }
                                else{
                                    TmpMat.copyTo(Smat);
                                }
                            }   //unlock()
                            FrameIndex.store(FrameCount);
                            GrabFetch.store(false);
                            GrabTag.store(++Tag);                                                                 //remember PrevTag=0 and Tag!=PrevTag to fetch
                            ElapsedTime.store(static_cast<slong64>(static_cast<double>(FrameCount*1000)/CamFPS)); //note, time can jump back to 0, when the video rewinds!!!
                            //wait until the frame is used
                            while((GrabFetch.load() == false) && (GrabStop.load() == false)){
                                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                            }

                            if(PipeInfo.CamSync){
                                Tend  = std::chrono::steady_clock::now();
                                Tgone = std::chrono::duration_cast<std::chrono::milliseconds> (Tend - Tstart).count();
                                Twait = floor(1000.0/AskedFPS - Tgone);
                                if(Twait > 0){
                                    std::this_thread::sleep_for(std::chrono::milliseconds(Twait));
                                }
                            }

                            // calaculate number of frames to be skipped
                            FrSkip+=floor(Fraction-1.0);
                            Fsum+=Fraction; Isum+=Fraction;
                            if((floor(Fsum)-Isum)>0){
                                FrSkip++; Fsum=0.0; Isum=0;
                            }

                            Tstart = std::chrono::steady_clock::now();
                        }
                        //rewind
                        //are we in a loop and do we know the numbers of frames?
                        if(PipeInfo.CamLoop && (CamFrames>0)){
                            if(FrameCount > CamFrames-2){
                                //rewind back
                                av_seek_frame(input_ctx, 0, 0, AVSEEK_FLAG_FRAME);
                                FrameCount = 0;
                                Tag = 0;
                                Isum = FrSkip = 0;
                                Fsum = 0.0;
                            }
                        }
                    }
                }
            }
            av_packet_unref(packet);
        }
        else{
            //pause -> wait 100 mSec
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
//----------------------------------------------------------------------------------
