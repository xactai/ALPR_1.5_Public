#ifndef TWorker_H
#define TWorker_H

#include "General.h"

//----------------------------------------------------------------------------------
class TWorker
{
private:
//    // threading
//    size_t Tag, PrevTag;
//    cv::Mat TmpMat;
//    cv::Mat Smat;
//    std::atomic<bool> GrabFetch;
//    std::atomic<size_t> GrabTag;
//    // parent
//    TCamInfo PipeInfo;
//    // adjust FPS
//    double Fsum, Fraction, AskedFPS;
//    int    Isum, FrSkip, Twait;
//    std::chrono::steady_clock::time_point Tstart, Tend;
//    float Tgone;
//
//    int  InfoStreamer(void);
//    std::vector<std::string> DirPics;                       //the list of pictures found in the folder
protected:
    std::thread ThGr;
    std::atomic<bool> ColStop;

    void LicenseThread(void);
public:
    TWorker(void);
    virtual ~TWorker();

    void InitCollection(void);
    void CloseCollection(void);
};
//----------------------------------------------------------------------------------
#endif // TWorker_H
