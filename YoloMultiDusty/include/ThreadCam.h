#ifndef THREADCAM_H
#define THREADCAM_H

#include <thread>
#include <atomic>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
//----------------------------------------------------------------------------------
struct TCamInfo                     //camera info
{
public:
    int         CamWidth {0};
    int         CamHeight {0};
    std::string CamCodec;
    std::string PipeLine;           //the actual used pipeline
};
//----------------------------------------------------------------------------------
class ThreadCam : public TCamInfo
{
private:
    int  Width;                         //width of captured image
    int  Height;                        //height of captured image
    size_t Tag,PrevTag;
    std::string device;                 //the device (can be a pipeline)
    cv::Mat mm;                         //most actual frame
    cv::VideoCapture cap;               //the capture object
    std::mutex mtx;
protected:
    std::thread ThGr;
    void GrabThread(void);
    void GetPipeLine(void);
public:
    std::atomic<bool> GrabOpen;
    std::atomic<bool> GrabStop;
    std::atomic<size_t> GrabTag;

    ThreadCam(int Width, int Height);   //wanted Width and Height of received cv::Mat
    virtual ~ThreadCam(void);           //(Camera ratio will overrule width or height)

    void Start(std::string dev);        //setup a new grabbing thread
    bool GetFrame(cv::Mat& m);          //returns false when no frame is available.
    void Quit(void);
};
//----------------------------------------------------------------------------------
#endif // THREADCAM_H
