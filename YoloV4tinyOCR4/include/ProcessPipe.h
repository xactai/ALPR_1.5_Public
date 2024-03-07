#ifndef PROCESSPIPE_H
#define PROCESSPIPE_H

#include <opencv2/opencv.hpp>
#include "ThreadCam.h"
#include "MJPGthread.h"
#include "TSnapShot.h"
#include "yolo_v2_class.hpp"	        // imported functions from .so
#include "TMapper.h"

//----------------------------------------------------------------------------------
class TProcessPipe
{
private:
    int  CamNr;
    int  Width;                                     //width and height of the frame we process. see config.json
    int  Height;                                    //need not be the camera resolution (smaller will speed up calcus)
    int  FrameWidth;                                //received frame width
    int  FrameHeight;                               //received frame height
    int  MJpeg;
    double Warp;                                    //Warp coefficient (birds view)
    double Gain;                                    //Gain coefficient (birds view)
    ThreadCam  *CamGrb;
    MJPGthread *MJ;
    cv::Rect    DnnRect;                            //roi used by pre-DNN models (not the ALPR darknet models)
    cv::Mat     MatEmpty;                           //gray (empty) frame
    cv::Mat     Gframe;                             //gaussian blur image
    cv::Mat     BWframe;                            //black/white image
    cv::Mat     Mframe;                             //morphology image
    cv::Mat     Eframe;                             //erosion image
    bool        UseNetRect;                         //quick check whether we must use NetRect
    bool        MapperRdy;                          //check if Mapper is initiated
    //some administration
    std::string CamName;
    std::string TopicName;
    std::string FoiDirName;
protected:
    void ProcessDNN(cv::Mat& frame);
    void RepairRareLabels(std::vector<bbox_t>& boxes,const unsigned int Lreplace,const unsigned int Ltarget);
    void DrawObjects(cv::Mat& frame, const std::vector<bbox_t>& boxes);
    void DrawObjectsLabels(cv::Mat& frame, const std::vector<bbox_t>& boxes);
    void GetJson(std::string &SendJson);
    bool SendJsonHTTP(std::string send_json,int port = 8070, int timeout = 400000);
    void AddJsonVehicle(std::ostringstream& os,const int i,const bool Comma);
    void AddJsonPerson(std::ostringstream& os,const int i,const bool Comma);
public:
    cv::Mat     MatAr;                              //frame received from CamGrb
    TMapper     Map;                                //map the object routes
    TSnapShot   SnapShot;                           //currently best license plate image

    TProcessPipe(void);
    virtual ~TProcessPipe(void);
    void Init(const int Nr);
    void StartThread(void);
    void StopThread(void);
    void ExecuteCam(void);
};
//----------------------------------------------------------------------------------
#endif // PROCESSPIPE_H

