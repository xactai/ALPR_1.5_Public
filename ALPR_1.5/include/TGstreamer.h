#ifndef TGstreamer_H
#define TGstreamer_H

#include "General.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}
//----------------------------------------------------------------------------------
class TGstreamer : public TCamInfo
{
private:
    // threading
    size_t Tag, PrevTag;
    cv::VideoCapture cap;               //the capture object
    cv::Mat TmpMat;
    cv::Mat Smat;
    std::atomic<bool> GrabFetch;
    std::atomic<size_t> GrabTag;
    // parent
    TCamInfo PipeInfo;
    // adjust FPS
    double Fsum, Fraction, AskedFPS;
    int    Isum, FrSkip, Twait;
    //timing
    std::chrono::steady_clock::time_point Tbegin, Tend;

    int  InfoGstreamer(void);
protected:
    std::thread ThGr;
    std::atomic<bool> GrabStop;

    void GstreamerThread(void);
public:
    TGstreamer(void);
    virtual ~TGstreamer();

    float FPS;                                      //actual FPS = min(AskFPS,CamFPS)
    std::atomic<slong64> ElapsedTime;               //get elapsed mSec from the start

    int  InitGstreamer(const TCamInfo *Parent);     //start the stream in a new thread

    bool GetFrame(cv::Mat& m);
    void CloseGstreamer(void);

    void Pause(void);               //not implemented (to prevent confusion whether your CCTV is pausing the scene or malfunctions)
    void Continue(void);            //not implemented (to prevent confusion whether your CCTV is pausing the scene or malfunctions)
};
//----------------------------------------------------------------------------------
#endif // TGstreamer_H
