#ifndef THREADCAM_H
#define THREADCAM_H

#include "General.h"
#include "TFFmpeg.h"
#include "TGstreamer.h"
#include "TOstreamer.h"
//----------------------------------------------------------------------------------
class ThreadCam : public TCamInfo
{
friend class TProcessPipe;
private:
protected:
    TFFmpeg    *Fstreamer;
    TGstreamer *Gstreamer;
    TOstreamer *Ostreamer;
public:
    ThreadCam(void);
    virtual ~ThreadCam(void);

    bool StartCam(const json& s);
    bool GetFrame(cv::Mat& m, slong64& mSec);

    void Pause(void);
    void Continue(void);
    double GetTbase(void);
};
//----------------------------------------------------------------------------------
#endif // THREADCAM_H
