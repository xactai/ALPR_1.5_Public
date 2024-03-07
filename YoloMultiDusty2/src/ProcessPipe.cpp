#include "General.h"
#include "ProcessPipe.h"
#include "net.h"
#include "Tjson.h"
#include "yolo-fastestv2.h"

using namespace std;

extern Tjson         Js;
extern yoloFastestv2 yoloF2;

//---------------------------------------------------------------------------
const char* class_names[] = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
    "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"
};
//---------------------------------------------------------------------------
// TProcessPipe
//---------------------------------------------------------------------------
TProcessPipe::TProcessPipe(void)
{
    CamNr  = -1;
    CamGrb = NULL;
}
//---------------------------------------------------------------------------
TProcessPipe::~TProcessPipe(void)
{
    if(CamGrb != NULL) delete CamGrb;
}
//---------------------------------------------------------------------------
void TProcessPipe::Init(const int Nr)
{
    CamNr   = Nr;
    Width   = Js.WorkWidth;
    Height  = Js.WorkHeight;

    CamGrb = new ThreadCam(Width,Height);

    MatEmpty = cv::Mat(Height, Width, CV_8UC3, cv::Scalar(128,128,128));

    MatEmpty.copyTo(MatAr);

    //check DNN rect with our camera
    NetRect = Js.DnnRect;
    if(NetRect.x     <     0) NetRect.x     = 0;
    if(NetRect.width > Width) NetRect.width = Width;
    if(NetRect.y     <     0) NetRect.y     = 0;
    if(NetRect.height>Height) NetRect.height = Height;
    if((NetRect.x+NetRect.width ) >  Width) NetRect.x=Width -NetRect.width;
    if((NetRect.y+NetRect.height) > Height) NetRect.y=Height-NetRect.height;

    UseNetRect = (NetRect.x!= 0 || NetRect.y!=0 || NetRect.width!=Width || NetRect.height!=Height);
    RdyNetRect = false;
}
//---------------------------------------------------------------------------
void TProcessPipe::StartThread(void)
{
    std::string str="STREAM_"+std::to_string(CamNr+1);

    CamGrb->Start(Js.j[str]);
}
//---------------------------------------------------------------------------
void TProcessPipe::StopThread(void)
{
    CamGrb->Quit();
}
//----------------------------------------------------------------------------------------
void TProcessPipe::ExecuteCam(void)
{
    int x,y,b;
    int Wd,Ht;
    int Wn,Hn;
    int Wf,Hf;
    cv::Mat Mdst;
    bool Success;
    float Ratio=1.0;

    if(CamGrb->PipeLine=="") return;

    Success = CamGrb->GetFrame(MatAr);

    if(Success){
        //width or height may differ. (depends on the camera settings)
        Wd=MatAr.cols;
        Ht=MatAr.rows;

        if(Wd!=Width || Ht!=Height){
            if(Wd > Ht){
                Ratio = (float)Width / Wd;
                Wn = Width;
                Hn = Ht * Ratio;
            }
            else{
                Ratio = (float)Height / Ht;
                Hn = Height;
                Wn = Wd * Ratio;
            }
            resize(MatAr, Mdst, cv::Size(Wn,Hn), 0, 0, cv::INTER_LINEAR); // resize to 640xHn or Wnx480 resolution
            //now work with the Mdst frame
            ProcessDNN(Mdst);

            //next place it in a proper 640x480 frame
            MatEmpty.copyTo(MatAr);
            if(Hn<Height){
                Hf=(Height-Hn)/2;
                Mdst.copyTo(MatAr(cv::Rect(0, Hf, Width, Mdst.rows)));
            }
            else{
                Wf=(Width-Wn)/2;
                Mdst.copyTo(MatAr(cv::Rect(Wf, 0, Mdst.cols, Height)));
            }
        }
        else{
            //width and height are equal the view, work direct on MatAr
            ProcessDNN(MatAr);
        }
    }
    else MatEmpty.copyTo(MatAr);

    //place name at the corner
    cv::Size label_size = cv::getTextSize(CamGrb->CamName.c_str(), cv::FONT_HERSHEY_SIMPLEX, 0.6, 1, &b);
    x = Width  - label_size.width - 4;
    y = Height - b - 4;
    cv::putText(MatAr, CamGrb->CamName.c_str(), cv::Point(x,y), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 0));
}
//----------------------------------------------------------------------------------
void TProcessPipe::ProcessDNN(cv::Mat& frame)
{
    cv::Mat DNNframe;
    std::vector<TargetBox> objects;

    if(UseNetRect){
        //initial check, no need to check every frame because all variables will never change any more
        if(!RdyNetRect){
            if(NetRect.x     <         0) NetRect.x     = 0;
            if(NetRect.width >frame.cols) NetRect.width = frame.cols;
            if(NetRect.y     <         0) NetRect.y     = 0;
            if(NetRect.height>frame.rows) NetRect.height = frame.rows;
            if((NetRect.x+NetRect.width ) > frame.cols) NetRect.x=frame.cols -NetRect.width;
            if((NetRect.y+NetRect.height) > frame.rows) NetRect.y=frame.rows-NetRect.height;
            RdyNetRect = true;
        }
        frame(NetRect).copyTo(DNNframe);

        yoloF2.detection(DNNframe, objects);

        //shift the outcome to its position in the original frame
        for(size_t i = 0; i < objects.size(); i++) {
            objects[i].x1 += NetRect.x;   objects[i].x2 += NetRect.x;
            objects[i].y1 += NetRect.y;   objects[i].y2 += NetRect.y;
        }
    }
    else{
        yoloF2.detection(frame, objects);
    }

    DrawObjects(frame, objects);
}
//---------------------------------------------------------------------------
void TProcessPipe::DrawObjects(cv::Mat& frame, const std::vector<TargetBox>& boxes)
{
    if(Js.PrintOnCli){
        std::cout << CamGrb->CamName << endl;      //+1 so MatAr[0] will be Cam 1
        for(size_t i = 0; i < boxes.size(); i++){
            std::cout <<boxes[i].score<<" "<< class_names[boxes[i].cate] << std::endl;
        }
    }

    if(Js.PrintOnRender || Js.MJPEG_Port>7999){
        for(size_t i = 0; i < boxes.size(); i++) {

            char text[256];
            sprintf(text, "%s %.1f%%", class_names[boxes[i].cate], boxes[i].score * 100);

            int baseLine = 0;
            cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

            int x = boxes[i].x1;
            int y = boxes[i].y1 - label_size.height - baseLine;
            if (y < 0) y = 0;
            if (x + label_size.width > frame.cols) x = frame.cols - label_size.width;

            cv::rectangle(frame, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                          cv::Scalar(255, 255, 255), -1);

            cv::putText(frame, text, cv::Point(x, y + label_size.height),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

            cv::rectangle (frame, cv::Point(boxes[i].x1, boxes[i].y1),
                                  cv::Point(boxes[i].x2, boxes[i].y2), cv::Scalar(255,0,0));
        }
    }
}
//----------------------------------------------------------------------------------

