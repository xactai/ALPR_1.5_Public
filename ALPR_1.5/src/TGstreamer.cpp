#include "TGstreamer.h"

using namespace std;
//----------------------------------------------------------------------------------
TGstreamer::TGstreamer(void) : GrabFetch(true), GrabStop(false)
{
    CamName = "";
    Tag = PrevTag = 0;
}
//----------------------------------------------------------------------------------
TGstreamer::~TGstreamer(void)
{
    CloseGstreamer();
}
//----------------------------------------------------------------------------------
int TGstreamer::InitGstreamer(const TCamInfo *Parent)
{
    int ret = 0;

    // get the required settings from ThreadCam
    PipeInfo = *Parent;
    FPS      = PipeInfo.CamFPS;
    AskedFPS = PipeInfo.CamFPS;
    PipeLine = PipeInfo.Device;
    CamName  = PipeInfo.CamName;

    if(AskedFPS<=0.0) AskedFPS=0.01;

    // open the input file and give some info
    ret = InfoGstreamer();
    if(ret < 0) return ret;

    // at this point, we now know the pipeline works
    FPS=min(AskedFPS,CamFPS);

    // init the while loop
    Fraction = CamFPS / AskedFPS;
    if(Fraction<1.0){
        AskedFPS = CamFPS;
        Fraction = 1.0;
    }
    Isum = FrSkip = 0;
    Fsum = 0.0;

    // fire up the thread
    ThGr = std::thread([=] { GstreamerThread(); });

    return 0;
}
//----------------------------------------------------------------------------------------
void TGstreamer::Pause(void)
{
//    GrabPause.store(true);  //don't pause the camera
}
//----------------------------------------------------------------------------------------
void TGstreamer::Continue(void)
{
//    GrabPause.store(false);  //don't continue the camera
}
//----------------------------------------------------------------------------------
int TGstreamer::InfoGstreamer(void)
{
    AVFormatContext *formatContext = nullptr;
    AVCodecContext  *decoder_ctx   = nullptr;
#ifdef __aarch64__
    AVCodec         *decoder       = nullptr;
#else
    const AVCodec   *decoder       = nullptr;
#endif

    // open the input file
    if(avformat_open_input(&formatContext, PipeInfo.Device.c_str(), nullptr, nullptr) != 0) {
        PLOG_ERROR << CamName << " Gstreamer: Cannot open input device: " << Device;
        return -1;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        PLOG_ERROR << CamName << " Gstreamer: Could not find stream information.";
        avformat_close_input(&formatContext);
        return 1;
    }

    // find the video stream information
    int video_stream = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if(video_stream < 0){
        PLOG_ERROR << CamName << " Gstreamer: Cannot find a video stream in the input file.";
        avformat_close_input(&formatContext);
        return -1;
    }
    // alloc space for the decoder
    if (!(decoder_ctx = avcodec_alloc_context3(decoder))){
        PLOG_ERROR << CamName << " Gstreamer: Cannot alloc space for the decoder.";
        avcodec_free_context(&decoder_ctx);
        avformat_close_input(&formatContext);
        return AVERROR(ENOMEM);
    }

    AVStream *video = formatContext->streams[video_stream];
    // fill the decoder with the found values
    if(avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0){
        PLOG_ERROR << CamName << " Gstreamer: Cannot fill the decoder.";
        avcodec_free_context(&decoder_ctx);
        avformat_close_input(&formatContext);
        return -1;
    }

    // get info
    AVCodecParameters *codecParams = video->codecpar;
    CamWidth =codecParams->width;
    CamHeight=codecParams->height;
    CamCodec =avcodec_get_name(codecParams->codec_id);
    CamFPS=av_q2d(video->r_frame_rate);
    if(CamFPS <= 0) {
        PLOG_ERROR << CamName << " Gstreamer: Cannot find the FPS of the stream.";
        return -1;
    }
    CamFrames = video->nb_frames;
    CamPixels = (int) decoder_ctx->pix_fmt;

    // print info
    PLOG_INFO << CamName << " Gstreamer: Width: " << CamWidth;
    PLOG_INFO << CamName << " Gstreamer: Height: " << CamHeight;
    PLOG_INFO << CamName << " Gstreamer: FPS: " << CamFPS;
    PLOG_INFO << CamName << " Gstreamer: Frames : " << CamFrames;
    PLOG_INFO << CamName << " Gstreamer: Codec Type: " << CamCodec;
    PLOG_INFO << CamName << " Gstreamer: Pixel Format: " << av_get_pix_fmt_name(static_cast<enum AVPixelFormat>(CamPixels));

    avcodec_free_context(&decoder_ctx);
    avformat_close_input(&formatContext);

    return 0;
}
//----------------------------------------------------------------------------------
bool TGstreamer::GetFrame(cv::Mat& m)
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
        FileName = to_string(PrevTag);
        //let the other thread know we are ready
        GrabFetch.store(true);
        Success= (!m.empty());
    }
    return Success;
}
//----------------------------------------------------------------------------------
void TGstreamer::CloseGstreamer(void)
{
    GrabStop.store(true);

    if(ThGr.joinable()){
        ThGr.join();                //wait until the grab thread ends
    }
}
//----------------------------------------------------------------------------------
// Inside the Thread
//----------------------------------------------------------------------------------
void TGstreamer::GstreamerThread(void)
{
    std::mutex mtx;

    Tag    = 0;
    Tbegin = std::chrono::steady_clock::now();
    while(GrabStop.load() == false){
        // start the CCTV
        cap.release();
        while(!cap.isOpened()){
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            cap.open(PipeLine);            //open the Device with the pipeline
        }
        // run this code as long as the CCTV is open
        // and we don't receive a stop
        while((GrabStop.load() == false) && cap.isOpened()){
            // in case of a lost internet connection, cap.read will wait for a very long period.
            // once the connection is re-established, it will proceed further.
            // no need for an else branch, as it will not be visited.
            if(cap.read(TmpMat)){
                // adjust frame rate
                if(FrSkip>0) FrSkip--;
                else{
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        // Resize the image if needed (keep TmpMat to its fixed sized)
                        if((PipeInfo.CamWidth!=TmpMat.cols) || (PipeInfo.CamHeight!=TmpMat.rows)){
                            cv::resize(TmpMat, Smat, cv::Size(PipeInfo.CamWidth, PipeInfo.CamHeight));
                        }
                        else{
                            TmpMat.copyTo(Smat);
                        }
                    }   //unlock()
                    GrabFetch.store(false);
                    GrabTag.store(++Tag);               //remember PrevTag=0 and Tag!=PrevTag to fetch

                    Tend = std::chrono::steady_clock::now();
                    ElapsedTime.store(static_cast<slong64>(std::chrono::duration_cast<std::chrono::milliseconds> (Tend - Tbegin).count()));

                    // calaculate number of frames to be skipped
                    FrSkip+=floor(Fraction-1.0);
                    Fsum+=Fraction; Isum+=Fraction;
                    if((floor(Fsum)-Isum)>0){
                        FrSkip++; Fsum=0.0; Isum=0;
                    }
                }
            }
        }
    }
}
//----------------------------------------------------------------------------------
