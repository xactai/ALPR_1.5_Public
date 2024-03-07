#include "General.h"
#include "ProcessFrame.h"
#include "net.h"
#include "Settings.h"
#include "yolo-fastestv2.h"

using namespace std;

extern const char* class_names[];
extern TSettings MySettings;
extern yoloFastestv2 yoloF2;

//---------------------------------------------------------------------------
// TProcessFrame
//---------------------------------------------------------------------------
TProcessFrame::TProcessFrame(TCamCluster *CamClst)
{
    MyCams = CamClst;
}
//---------------------------------------------------------------------------
TProcessFrame::~TProcessFrame(void)
{
}
//----------------------------------------------------------------------------------------
void TProcessFrame::ExecuteFrame(const int FrmNr)
{
    int x,y,b;
    int Wd,Ht;
    int Wn,Hn;
    int Wf,Hf;
    cv::Mat Mdst;
    bool Success;
    float Ratio=1.0;
    int ViewWidth  = MyCams->Width;
    int ViewHeight = MyCams->Height;
    cv::Mat FrameE(ViewHeight, ViewWidth, CV_8UC3, cv::Scalar(128,128,128));

    Success=MyCams->GetFrame(FrmNr);

    if(Success){
        WorkingOnFrameNr=FrmNr;
        //width or height may differ per frame. (depends on the camera settings)
        Wd=MyCams->MatAr[FrmNr].cols;
        Ht=MyCams->MatAr[FrmNr].rows;

        if(Wd!=ViewWidth || Ht!=ViewHeight){
            if(Wd > Ht){
                Ratio = (float)ViewWidth / Wd;
                Wn = ViewWidth;
                Hn = Ht * Ratio;
            }
            else{
                Ratio = (float)ViewHeight / Ht;
                Hn = ViewHeight;
                Wn = Wd * Ratio;
            }
            resize(MyCams->MatAr[FrmNr], Mdst, cv::Size(Wn,Hn), 0, 0, cv::INTER_CUBIC); // resize to 640xHn or Wnx480 resolution
            //now work with the Mdst frame
            ProcessDNN(Mdst);

            //next place it in a proper 640x480 frame
            FrameE.copyTo(MyCams->MatAr[FrmNr]);
            if(Hn<ViewHeight){
                Hf=(ViewHeight-Hn)/2;
                Mdst.copyTo(MyCams->MatAr[FrmNr](cv::Rect(0, Hf, ViewWidth, Mdst.rows)));
            }
            else{
                Wf=(ViewWidth-Wn)/2;
                Mdst.copyTo(MyCams->MatAr[FrmNr](cv::Rect(Wf, 0, Mdst.cols, ViewHeight)));
            }
        }
        else{
            //width and height are equal the view, work direct on MatAr[]
            ProcessDNN(MyCams->MatAr[FrmNr]);
        }
    }
    //place name at the corner
    std::string CamName="Cam "+std::to_string(FrmNr+1);         //+1 so MatAr[0] will be Cam 1
    cv::Size label_size = cv::getTextSize(CamName.c_str(), cv::FONT_HERSHEY_SIMPLEX, 0.6, 1, &b);
    x = ViewWidth  - label_size.width - 4;
    y = ViewHeight - b - 4;
    cv::putText(MyCams->MatAr[FrmNr], CamName.c_str(), cv::Point(x,y), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 0));
}
//----------------------------------------------------------------------------------
void TProcessFrame::ProcessDNN(cv::Mat& frame)
{
    std::vector<TargetBox> objects;

    yoloF2.detection(frame, objects);

    DrawObjects(frame, objects);
}
//---------------------------------------------------------------------------
void TProcessFrame::DrawObjects(cv::Mat& frame, const std::vector<TargetBox>& boxes)
{
    if(MySettings.PrintOutput){
        std::cout << "Cam " << WorkingOnFrameNr+1 << endl;      //+1 so MatAr[0] will be Cam 1
        for(size_t i = 0; i < boxes.size(); i++){
            std::cout <<boxes[i].score<<" "<< class_names[boxes[i].cate] << std::endl;
        }
    }

    if(MySettings.ShowOutput || MySettings.MjpegPort>7999){
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

            cv::rectangle (frame
            , cv::Point(boxes[i].x1, boxes[i].y1),
                           cv::Point(boxes[i].x2, boxes[i].y2), cv::Scalar(255,0,0));
        }
    }
}
//----------------------------------------------------------------------------------

