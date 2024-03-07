#include "ThreadCam.h"

using namespace std;
//----------------------------------------------------------------------------------
ThreadCam::ThreadCam(void)
{
    //init TCamInfo with requested parameters
    CamSource   = USE_NONE;

    Fstreamer = new TFFmpeg();
    Gstreamer = new TGstreamer;
    Ostreamer = new TOstreamer();
}
//----------------------------------------------------------------------------------
ThreadCam::~ThreadCam(void)
{
    delete Fstreamer;
    delete Gstreamer;
    delete Ostreamer;
}
//----------------------------------------------------------------------------------
bool ThreadCam::StartCam(const json& s)
{
    Tjson TJ;
    string Jstr;
    string Sstr;

    //force the json to be valid, else no GetSetting() will work
    TJ.Jvalid=true;

    //init TCamInfo with requested parameters
    CamSource   = USE_NONE;

    if(!TJ.GetSetting(s,"SYNC",CamSync)){
        PLOG_ERROR << "Error reading SYNC setting from the json file";
        return false;
    }
    if(!TJ.GetSetting(s,"FPS",CamFPS)){
        PLOG_ERROR << "Error reading FPS setting from the json file";
        return false;
    }
    if(CamFPS<=0.0){
        PLOG_ERROR << "Error FPS must be greater than zero";
        return false;
    }
    if(!TJ.GetSetting(s,"LOOP",CamLoop)){
        PLOG_ERROR << "Error reading LOOP setting from the json file";
        return false;
    }

    CamName = s.at("CAM_NAME");
    CamSync = s.at("SYNC");
    CamFPS  = s.at("FPS");
    CamLoop = s.at("LOOP");

    Jstr    = s.at("VIDEO_INPUT");
    if(!Jstr.empty()){
        Sstr = lcase_copy(trim_copy(Jstr));
        // picture
        if(Sstr.find("image") != string::npos){
            CamSource = USE_PICTURE;
        }
        else{
            // folder
            if(Sstr.find("folder") != string::npos){
                CamSource = USE_FOLDER;
            }
            else{
                // video (use FFmpeg)
                if(Sstr.find("video") != string::npos){
                    CamSource = USE_VIDEO;
                }
                else{
                    // camera (use Gstreamer)
                    CamSource = USE_CAMERA;
                }
            }
        }
    }
    else{
        PLOG_ERROR << "Error reading VIDEO_INPUT settings from the json file";
        return false;
    }

    //read the parameters
    if(TJ.GetSetting(s["VIDEO_INPUTS_PARAMS"], Jstr, Device)){
        PipeLine=Device;
    }
    else{
        PLOG_ERROR << "Error reading image settings from the json file";
        return false;
    }

    switch(CamSource){
        case USE_NONE    : { break; }
        case USE_PICTURE : { CamFrames=1; Ostreamer->InitOstreamer(this); break; }
        case USE_FOLDER  : { CamFrames=0; Ostreamer->InitOstreamer(this); break; }
        case USE_VIDEO   : { Fstreamer->InitFFmpeg(this); break; }
        case USE_CAMERA  : { Gstreamer->InitGstreamer(this); break; }
    }

    return true;
}
//----------------------------------------------------------------------------------------
bool ThreadCam::GetFrame(cv::Mat& m, slong64 &mSec)
{
    bool Success=true;

    switch(CamSource){
        case USE_NONE    : { mSec=0; break; }
        case USE_PICTURE : { Success=Ostreamer->GetFrame(m); mSec=Ostreamer->ElapsedTime.load(); FileName=Ostreamer->FileName; break; }
        case USE_FOLDER  : { Success=Ostreamer->GetFrame(m); mSec=Ostreamer->ElapsedTime.load(); FileName=Ostreamer->FileName; break; }
        case USE_VIDEO   : { Success=Fstreamer->GetFrame(m); mSec=Fstreamer->ElapsedTime.load(); FileName=Fstreamer->FileName;  break; }
        case USE_CAMERA  : { Success=Gstreamer->GetFrame(m); mSec=Gstreamer->ElapsedTime.load(); FileName=Gstreamer->FileName;  break; }
    }

    return Success;
}
//----------------------------------------------------------------------------------------
void ThreadCam::Pause(void)
{
    switch(CamSource){
        case USE_NONE    : { break; }
        case USE_PICTURE : { Ostreamer->Pause(); break; }
        case USE_FOLDER  : { Ostreamer->Pause(); break; }
        case USE_VIDEO   : { Fstreamer->Pause(); break; }
        case USE_CAMERA  : { break; }
    }
}
//----------------------------------------------------------------------------------------
void ThreadCam::Continue(void)
{
    switch(CamSource){
        case USE_NONE    : { break; }
        case USE_PICTURE : { Ostreamer->Continue(); break; }
        case USE_FOLDER  : { Ostreamer->Continue(); break; }
        case USE_VIDEO   : { Fstreamer->Continue(); break; }
        case USE_CAMERA  : { break; }
    }
}
//----------------------------------------------------------------------------------
double ThreadCam::GetTbase(void)
{
    double Base=0.0;

    switch(CamSource){
        case USE_NONE    : { break; }
        case USE_PICTURE : { Base = 1.0/Ostreamer->FPS; break; }
        case USE_FOLDER  : { Base = 1.0/Ostreamer->FPS; break; }
        case USE_VIDEO   : { Base = 1.0/Fstreamer->FPS; break; }
        case USE_CAMERA  : { Base = 1.0/Gstreamer->FPS; break; }
    }

    return Base;
}
//----------------------------------------------------------------------------------
