#ifndef TFFMPEG_H
#define TFFMPEG_H

#include "General.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}
//----------------------------------------------------------------------------------
class TFFmpeg : public TCamInfo
{
private:
    AVFormatContext *input_ctx = NULL;
    enum AVHWDeviceType type;
    AVPacket *packet = NULL;
#ifdef __aarch64__
    AVCodec *decoder = NULL;
#else
    const AVCodec *decoder = NULL;
#endif
    int video_stream;
    enum AVPixelFormat hw_pix_fmt;
    AVCodecContext *decoder_ctx = NULL;
    AVBufferRef *hw_device_ctx = NULL;
    AVFrame *frame = NULL;
    AVFrame *sw_frame = NULL;
    AVFrame *tmp_frame = NULL;
    AVFrame *pFrameRGB = NULL;
    uint8_t *buffer = NULL;
    // threading
    size_t Tag, PrevTag;
    cv::Mat TmpMat;
    cv::Mat Smat;
    std::atomic<bool> GrabFetch;
    std::atomic<size_t> GrabTag;
    // parent
    TCamInfo PipeInfo;
    // adjust FPS
    double Fsum, Fraction, AskedFPS;
    int    Isum, FrSkip, Twait;
    std::chrono::steady_clock::time_point Tstart, Tend;
    float Tgone;

protected:
    std::thread ThGr;
    std::atomic<int>  FrameIndex;
    std::atomic<bool> GrabPause;
    std::atomic<bool> GrabStop;

    void FFmpegThread(void);

    static enum AVPixelFormat get_hw_format_static(AVCodecContext* s, const enum AVPixelFormat* fmt) {
        TFFmpeg* instance = reinterpret_cast<TFFmpeg*>(s->opaque);
        return instance->get_hw_format(s, fmt);
    }

    enum AVPixelFormat get_hw_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts) {
        const enum AVPixelFormat* p;

        for (p = pix_fmts; *p != -1; p++) {
            if (*p == hw_pix_fmt)
                return *p;
        }
        PLOG_ERROR << "\nFailed to get HW surface format.\n";
        return AV_PIX_FMT_NONE;
    }

public:
    TFFmpeg(void);
    virtual ~TFFmpeg();

    float FPS;                                  //actual FPS = min(AskFPS,CamFPS)
    std::atomic<slong64> ElapsedTime;           //get elapsed mSec from the start based on CamFPS and FrameCnt
                                                //note, time can jump back to 0, when the video rewinds!!!
    int  InitFFmpeg(const TCamInfo *Parent);    //start the stream in a new thread
    void StreamInfo(AVFormatContext *formatContext);

    int  OpenFFmpeg(void);
    bool GetFrame(cv::Mat& m);
    void CloseFFmpeg(void);

    void Pause(void);
    void Continue(void);
};
//----------------------------------------------------------------------------------
#endif // TFFMPEG_H
