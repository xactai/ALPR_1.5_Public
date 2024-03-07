#include "CamCluster.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
//---------------------------------------------------------------------------
// TCamCluster
//----------------------------------------------------------------------------------
TCamCluster::TCamCluster(int _Width, int _Height)
{
    Count=0;
    CamGrb=NULL;
    Width = _Width;
    Height = _Height;
    MatEmpty = cv::Mat(Height, Width, CV_8UC3, cv::Scalar(128,128,128));
}
//----------------------------------------------------------------------------------
TCamCluster::~TCamCluster()
{
    DeleteCamArray();
}
//----------------------------------------------------------------------------------
void TCamCluster::DeleteCamArray(void)
{
    if(CamGrb!=NULL){
        //first, gracefully end your threads
        for(int i=0; i<Count; i++){
            if(CamGrb[i]->GrabOpen.load()) CamGrb[i]->Quit();
        }
        //now you can free the object
        for(int i=0; i<Count; i++){
            delete CamGrb[i];
        }
        delete[] CamGrb;

        //free the frames
        for(int i=0; i<Count; i++){
            MatAr[i].release();
        }
        delete[] MatAr;

        Count=0; CamGrb=NULL;
    }
}
//----------------------------------------------------------------------------------
void TCamCluster::InitCamArray(int NewSize)
{
    DeleteCamArray();

    if(NewSize==0) return;

    cv::Mat MatEmpty(Height, Width, CV_8UC3, cv::Scalar(128,128,128));

    Count=NewSize;

    MatAr  = new cv::Mat[Count];
    for(int i=0; i<Count; i++){
        MatAr[i]  = MatEmpty.clone();       //empty frames for now
    }

    CamGrb = new ThreadCam*[Count];
    for(int i=0; i<Count; i++){
        CamGrb[i] = new ThreadCam(Width,Height);
    }
}
//----------------------------------------------------------------------------------
void TCamCluster::Start(std::vector< std::string > Devs)     //array of RTSP locations.
{
    int i, s=(int) Devs.size();

    if(s==0) return;

    InitCamArray(s);

    for(i=0;i<s;i++){
        CamGrb[i]->Start(Devs[i]);
    }
}
//----------------------------------------------------------------------------------
bool TCamCluster::GetFrame(const int DevNr)      //returns false when no frame is available.
{
    bool Success = false;

    if( DevNr<0 || DevNr>=Count) return Success;

    if(CamGrb[DevNr]->PipeLine!=""){
        Success = CamGrb[DevNr]->GetFrame(MatAr[DevNr]);

        if(!Success) MatEmpty.copyTo(MatAr[DevNr]);
    }
    return Success;
}
//----------------------------------------------------------------------------------
void TCamCluster::GetComposedView(cv::Mat& m)                 //returns all frames in one large picture
{
    cv::Mat FrameCon1, FrameCon2, FrameTotal;

    if(Count==1) MatAr[0].copyTo(m);
    else{
        if(Count==2) cv::hconcat(MatAr[0],MatAr[1],m);
        else{
            cv::hconcat(MatAr[0],MatAr[1],FrameCon1);
            if(Count==3) cv::hconcat(MatAr[2],MatEmpty,FrameCon2);
            else         cv::hconcat(MatAr[2],MatAr[3],FrameCon2);
            cv::vconcat(FrameCon1,FrameCon2,m);
        }
    }
}
//----------------------------------------------------------------------------------
