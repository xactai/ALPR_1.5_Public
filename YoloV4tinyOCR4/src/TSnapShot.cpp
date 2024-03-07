#include "TSnapShot.h"
#include "General.h"
//---------------------------------------------------------------------------
TSnapShot::TSnapShot()
{
    Sfuzzy=0.0;
    Mocr  = "";
    obj_id=0;
    track_id=0;
}
//---------------------------------------------------------------------------
TSnapShot::~TSnapShot()
{
    //dtor
}
//---------------------------------------------------------------------------
void TSnapShot::Update(TMapper& Map)
{
    TLast=std::chrono::steady_clock::now();

    if(Map.MapList.size()>0){
        //new object -> reset
        if(Map.PlateObjectID!=obj_id || Map.PlateTrackID!=track_id){ Sfuzzy=0.0; Mocr=""; }

        //need update?
        if((Sfuzzy < Map.PlateQuality) || (Mocr != Map.PlateOCR)){
            obj_id   = Map.PlateObjectID;
            track_id = Map.PlateTrackID;
            Sfuzzy   = Map.PlateQuality;
            Mocr     = Map.PlateOCR;
            Refresh(Map.Plate);
        }
    }
    else{ Sfuzzy=0.0; Mocr = ""; }
}
//---------------------------------------------------------------------------
void TSnapShot::Show(cv::Mat& frame)
{
    float Telapse = (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-TLast).count())/1000.0;

    if(Telapse<10.0){
        //insert
        if(!Snap.empty()){
            Snap.copyTo(frame(cv::Rect(0, frame.rows-Snap.rows, Snap.cols, Snap.rows)));
        }
    }
}
//---------------------------------------------------------------------------
void TSnapShot::Refresh(cv::Mat& frame)
{
    char text[256];
    int w,h,baseLine=0;

    if(!frame.empty()){
        //merge in the bottom left corner
        cv::Mat SnapBig;
        if(Mocr!=""){
            cv::resize(frame, SnapBig, cv::Size(2*frame.cols, 2*frame.rows));
            sprintf(text, "%s", Mocr.c_str());
            cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 1, 2, &baseLine);
            w = std::max(SnapBig.cols+20, label_size.width+20);
            h = label_size.height+SnapBig.rows+30;

            cv::Mat Info(h, w, CV_8UC3, cv::Scalar(255,255,255));
            cv::putText(Info, text, cv::Point(10, 10 + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0,0,0), 2);
            SnapBig.copyTo(Info(cv::Rect(10, label_size.height+20, SnapBig.cols, SnapBig.rows)));
            Snap = Info.clone();
        }
        else{
            cv::resize(frame, SnapBig, cv::Size(2*frame.cols, 2*frame.rows));
            cv::Mat Info(SnapBig.rows+20, SnapBig.cols+20, CV_8UC3, cv::Scalar(255,255,255));
            SnapBig.copyTo(Info(cv::Rect(10, 10, SnapBig.cols, SnapBig.rows)));
            Snap = Info.clone();
        }
    }
}
//---------------------------------------------------------------------------
