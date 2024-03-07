#include "General.h"
#include "ProcessPipe.h"
#include "Tjson.h"
#include "yolo_v2_class.hpp"	        // imported functions from .so
#include <iostream>
#include <chrono>
#include <ctime>

//----------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------
extern Detector *YoloV4net;
extern Detector *PlateNet;
extern Tjson    Js;
extern size_t   FrameCnt;       //used to store pictures on disk with different naming.
extern void     publishMQTTMessage(const string& topic, const string payload);
//---------------------------------------------------------------------------
static cv::Mat MorphKernal = cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 11, 11 ), cv::Point( 5, 5 ) );
//---------------------------------------------------------------------------
// TProcessPipe
//---------------------------------------------------------------------------
TProcessPipe::TProcessPipe(void)
{
    DNNrdy = false;
    MJpeg  = 0;
    MJ     = nullptr;
}
//---------------------------------------------------------------------------
TProcessPipe::~TProcessPipe(void)
{
    if(MJ != NULL) delete MJ;
}
//---------------------------------------------------------------------------
void TProcessPipe::InitPipe(const int Nr)
{
    Tjson TJ;
    std::string Str;

    TJ.Jvalid = true;       //force the json to be valid, else no GetSetting() will work
    CamIndex  = Nr;
    // global settings
    CamWidth  = Js.WorkWidth;
    CamHeight = Js.WorkHeight;

    // local settings
    MatEmpty = cv::Mat(CamHeight, CamWidth, CV_8UC3, cv::Scalar(128,128,128));
    MatEmpty.copyTo(Pic);

    json Jstr=Js.j["STREAM_"+std::to_string(CamIndex+1)];

    if(!TJ.GetSetting(Jstr,"MJPEG_PORT",MJpeg)) MJpeg=0;
    if(!TJ.GetSetting(Jstr,"CAM_NAME",CamName)) CamName=to_string(CamIndex+1);

    //get DNN rect
    if(!TJ.GetSetting(Jstr["DNN_Rect"],"x_offset",DnnRect.x))     std::cout << "Using default value" << std::endl;
    if(!TJ.GetSetting(Jstr["DNN_Rect"],"y_offset",DnnRect.y))     std::cout << "Using default value" << std::endl;
    if(!TJ.GetSetting(Jstr["DNN_Rect"],"width",DnnRect.width))    std::cout << "Using default value" << std::endl;
    if(!TJ.GetSetting(Jstr["DNN_Rect"],"height",DnnRect.height))  std::cout << "Using default value" << std::endl;

    //check DNN rect with our camera
    if(DnnRect.x < 0) DnnRect.x = 0;
    if(DnnRect.y < 0) DnnRect.y = 0;
    if(DnnRect.width >  CamWidth){
        PLOG_WARNING << "\nDnn_rect width (" << DnnRect.width <<") larger than working width (" << CamWidth << ") !\n";
        DnnRect.width  = CamWidth;
    }
    if(DnnRect.height > CamHeight){
        PLOG_WARNING << "\nDnn_rect height (" << DnnRect.height <<") larger than working height (" << CamHeight << ") !\n";
        DnnRect.height = CamHeight;
    }
    if((DnnRect.x+DnnRect.width ) >  CamWidth){
        PLOG_WARNING << "\nDnn_rect x (" << DnnRect.x <<") + Dnn_rect width (" << DnnRect.width <<") larger than working width (" << CamWidth << ") !\n";
        DnnRect.x = CamWidth - DnnRect.width;
    }
    if((DnnRect.y+DnnRect.height) > CamHeight){
        PLOG_WARNING << "\nDnn_rect y (" << DnnRect.y <<") + Dnn_rect height (" << DnnRect.height <<") larger than working height (" << CamHeight << ") !\n";
        DnnRect.y = CamHeight- DnnRect.height;
    }

    if(MJpeg>7999){
        MJ = new MJPGthread();
        MJ->Init(MJpeg);
        PLOG_INFO << "Opened MJpeg port: " << MJpeg;
    }

    UseNetRect = (DnnRect.x!= 0 || DnnRect.y!=0 || DnnRect.width!=CamWidth || DnnRect.height!=CamHeight);

    Track.Init();
}
//---------------------------------------------------------------------------
void TProcessPipe::Start(void)
{
    std::string Jstr="STREAM_"+std::to_string(CamIndex+1);

    CamWorking = StartCam(Js.j[Jstr]);
}
//----------------------------------------------------------------------------------------
bool TProcessPipe::ExecuteDNN(void)
{
    if(!CamWorking) return false;

    bool Success;
    int x,y,b, Wf,Hf;
    int Wd, Ht, Wn, Hn;
    float Ratio=1.0;
    cv::Mat Mdst;

    if( GetFrame(Pic, PicTime) ){
        //we got a picture, lets see
        //width or height may differ. (depends on the camera settings)
        Wd=Pic.cols;  Ht=Pic.rows;
        if(Wd!=CamWidth || Ht!=CamHeight){
            if(Wd > Ht){
                Ratio = (float)CamWidth / Wd;
                Wn = CamWidth;
                Hn = Ht * Ratio;
            }
            else{
                Ratio = (float)CamHeight / Ht;
                Hn = CamHeight;
                Wn = Wd * Ratio;
            }
            resize(Pic, Mdst, cv::Size(Wn,Hn), 0, 0, cv::INTER_LINEAR); // resize to Wn,Hn resolution

            //now work with the Mdst frame
            ProcessDNN(Mdst);

            //next place it in a proper output frame
            if(Hn<CamHeight){
                MatEmpty.copyTo(Pic);       //to create gray borders
                Hf=(CamHeight-Hn)/2;
                Mdst.copyTo(Pic(cv::Rect(0, Hf, CamWidth, Mdst.rows)));
            }
            else{
                if(Wn<CamWidth){
                    MatEmpty.copyTo(Pic);   //to create gray borders
                    Wf=(CamWidth-Wn)/2;
                    if(Mdst.cols>Wd){}
                    Mdst.copyTo(Pic(cv::Rect(Wf, 0, Mdst.cols, CamHeight)));
                }
                else{
                    //just resize
                    resize(Mdst, Pic, cv::Size(Wd,Ht), 0, 0, cv::INTER_LINEAR); // resize to Wn,Hn resolution
                }
            }
        }
        else{
            //width and height are equal the view, work direct on Pic
            ProcessDNN(Pic);
        }

        //place name at the corner
        cv::Size label_size = cv::getTextSize(CamName.c_str(), cv::FONT_HERSHEY_SIMPLEX, 0.6, 1, &b);
        x = CamWidth  - label_size.width - 4;
        y = CamHeight - b - 4;
        cv::putText(Pic, CamName.c_str(), cv::Point(x,y), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 0));

        //send the result to the socket
        if(MJpeg>0){
            cv::Mat FrameMJPEG(Js.MJPEG_Height, Js.MJPEG_Width, CV_8UC3);
            cv::resize(Pic,FrameMJPEG,FrameMJPEG.size(),0,0);
            MJ->Send(FrameMJPEG);
        }

        Success = true;
    }
    else{
        Success = false;
    }

    return Success;
}
//----------------------------------------------------------------------------------
void TProcessPipe::ProcessDNN(cv::Mat& frame)
{
    cv::Mat DNNframe;

    //store the frame_full only if directory name is valid
    //note it stores a MASSIVE bulk of pictures on your disk!
    if(Js.FoI_Folder!="none"){
        FoiDirName = Js.FoI_Folder+"/"+CamName+"-"+to_string(FrameCnt)+"_utc.jpg";
        cv::imwrite(FoiDirName, frame);
    }

    BoxObjects.clear();

    if(UseNetRect){
        //initial check, no need to check every frame because all variables will never change
        if(!DNNrdy){
            if(DnnRect.x     <          0) DnnRect.x      = 0;
            if(DnnRect.width > frame.cols) DnnRect.width  = frame.cols;
            if(DnnRect.y     <          0) DnnRect.y      = 0;
            if(DnnRect.height> frame.rows) DnnRect.height = frame.rows;
            if((DnnRect.x+DnnRect.width ) > frame.cols ) DnnRect.x=frame.cols-DnnRect.width;
            if((DnnRect.y+DnnRect.height) > frame.rows ) DnnRect.y=frame.rows-DnnRect.height;
            DNNrdy = true;
        }
        frame(DnnRect).copyTo(DNNframe);

        BoxObjects = YoloV4net->detect(DNNframe,Js.ThresCar);

        //shift the outcome to its position in the original frame
        for(size_t i = 0; i < BoxObjects.size(); i++) {
            BoxObjects[i].x += DnnRect.x;
            BoxObjects[i].y += DnnRect.y;
        }
    }
    else{
        if(!DNNrdy){
            DNNrdy = true;
        }
        BoxObjects = YoloV4net->detect(frame,Js.ThresCar);
    }

    RepairRareLabels(BoxObjects,6,CAR);         //train    -> car
    RepairRareLabels(BoxObjects,7,CAR);         //truck    -> car
    RepairRareLabels(BoxObjects,28,CAR);        //suitcase -> car
    RepairRareLabels(BoxObjects,3,BIKE);        //bicycle  -> motorcycle

    CleanLabels(BoxObjects);                    //remove elephants, zebras, birds and other unused labels

    //get the tracking ids
    switch(CamSource){
        case USE_NONE    : { break; }
        case USE_PICTURE : { DummyTracks(BoxObjects);  break; }
        case USE_FOLDER  : { DummyTracks(BoxObjects);  break; }
        case USE_VIDEO   : { Track.update(BoxObjects); break; }
        case USE_CAMERA  : { Track.update(BoxObjects); break; }
    }
}
//---------------------------------------------------------------------------
void TProcessPipe::DummyTracks(std::vector<bbox_t>& boxes)
{
    for(size_t i=0; i<boxes.size(); i++){
        boxes[i].track_id = i+1;
    }
}
//---------------------------------------------------------------------------
void TProcessPipe::RepairRareLabels(std::vector<bbox_t>& boxes,const unsigned int Lreplace,const unsigned int Ltarget)
{
    size_t i, n;
    cv::Rect Rc, Rt, Ro;
    int St, So;
    float Sd, Se;
    bool found;

    for(i=0; i<boxes.size(); i++){
        if(boxes[i].obj_id == Lreplace){
            Rt =cv::Rect(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h);
            //see if we have a car in the list with approx the same size
            found=false;
            for(Se=0.5, n=0; n<boxes.size(); n++){
                if(boxes[n].obj_id == Ltarget){
                    Rc =cv::Rect(boxes[n].x,boxes[n].y,boxes[n].w,boxes[n].h);
                    //get overlap (So) between the two boxes
                    Ro = Rc & Rt; St=Rt.area(); So=Ro.area();
                    if(So>0){
                        Sd=fabs((((float)St)/So)-1.0);
                        //find the best matching overlap
                        //the more the overlap equals the size of the truck box,
                        //the better the fit.
                        //start with 0.5 - 1.5, hence Se=0.5 in the for declaration
                        if(Sd<Se){  Se=Sd; found=true;  }
                    }
                }
            }
            if(!found){
                //truck only -> lets label it as car
                boxes[i].obj_id=Ltarget;
            }
            else{
                //truck equals a car in size and position -> remove the entry
                boxes.erase(boxes.begin() + i);
                i--;
            }
        }
    }
}
//---------------------------------------------------------------------------
void TProcessPipe::CleanLabels(std::vector<bbox_t>& boxes)
{
    for(size_t i=0; i<boxes.size(); i++){
        if((boxes[i].obj_id != CAR) &&
           (boxes[i].obj_id != BIKE) &&
            (boxes[i].obj_id != PERSON)){
            //remove item
            boxes.erase(boxes.begin() + i);
            i--;
        }
    }
}
//----------------------------------------------------------------------------------

