#ifndef TOstreamer_H
#define TOstreamer_H

#include "General.h"

//----------------------------------------------------------------------------------
class TOstreamer : public TCamInfo
{
private:
    // threading
    size_t Tag, PrevTag;
    cv::Mat TmpMat;
    cv::Mat Smat;
    std::atomic<int>  FileIndex;
    std::atomic<bool> GrabFetch;
    std::atomic<size_t> GrabTag;
    // parent
    TCamInfo PipeInfo;
    // adjust FPS
    double Fsum, Fraction, AskedFPS;
    int    Isum, FrSkip, Twait;
    std::chrono::steady_clock::time_point Tstart, Tend;
    float Tgone;

    int  InfoStreamer(void);
    std::vector<std::string> DirPics;                       //the list of pictures found in the folder
protected:
    std::thread ThGr;
    std::atomic<bool> GrabPause;
    std::atomic<bool> GrabStop;

    void StreamerThread(void);
public:
    TOstreamer(void);
    virtual ~TOstreamer();

    float FPS;                                      //actual FPS = AskFPS
    std::atomic<slong64> ElapsedTime;               //get elapsed mSec from the start based on CamFPS and FrameCnt
                                                    //note, time can jump back to 0, when the video rewinds!!!
    int  InitOstreamer(const TCamInfo *Parent);     //start the stream in a new thread

    bool GetFrame(cv::Mat& m);
    void CloseGstreamer(void);

    void Pause(void);
    void Continue(void);
};
//----------------------------------------------------------------------------------
#endif // TOstreamer_H
