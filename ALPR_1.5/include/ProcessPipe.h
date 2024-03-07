#ifndef PROCESSPIPE_H
#define PROCESSPIPE_H

#include <opencv2/opencv.hpp>
#include "ThreadCam.h"
#include "BYTETracker.h"
#include "MJPGthread.h"
//----------------------------------------------------------------------------------
class TProcessPipe : public ThreadCam
{
private:
    cv::Mat     MatEmpty;                           //gray (empty) frame
    bool        UseNetRect;                         //quick check whether we must use NetRect
    bool        DNNrdy;                             //check DNN rect once
    int         MJpeg;
    MJPGthread *MJ;
protected:
    cv::Rect    DnnRect;                            //roi used by pre-DNN models (not the ALPR darknet models)
    std::vector<bbox_t> BoxObjects;
    std::string FoiDirName;
    BYTETracker Track;

    void ProcessDNN(cv::Mat& frame);
    void DummyTracks(std::vector<bbox_t>& boxes);
    void CleanLabels(std::vector<bbox_t>& boxes);
    void TrackLabels(std::vector<bbox_t>& boxes);
    void RepairRareLabels(std::vector<bbox_t>& boxes,const unsigned int Lreplace,const unsigned int Ltarget);
public:
    bool CamWorking;                                //the camera could start
    cv::Mat     Pic;                                //latest frame received
    slong64     PicTime;                            //time stamp belonging to Pic

    TProcessPipe(void);
    virtual ~TProcessPipe(void);

    void InitPipe(const int Nr);                    //sets the parameters of the pipe
    void Start(void);                               //starts the grab thread (e.g. connect to CCTV)

    bool ExecuteDNN(void);                          //execute DNN, if new frame is available
};
//----------------------------------------------------------------------------------
#endif // PROCESSPIPE_H

