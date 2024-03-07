#include "TOstreamer.h"

using namespace std;
//----------------------------------------------------------------------------------
TOstreamer::TOstreamer(void) : GrabFetch(true), GrabPause(false), GrabStop(false), ElapsedTime(0)
{
    CamName = "";
    Tag = PrevTag = 0;
}
//----------------------------------------------------------------------------------
TOstreamer::~TOstreamer(void)
{
    CloseGstreamer();
}
//----------------------------------------------------------------------------------
int TOstreamer::InitOstreamer(const TCamInfo *Parent)
{
    int ret = 0;

    // get the required settings from ThreadCam
    PipeInfo = *Parent;
    FPS      = PipeInfo.CamFPS;
    AskedFPS = PipeInfo.CamFPS;
    PipeLine = PipeInfo.Device;
    CamName  = PipeInfo.CamName;

    if(AskedFPS<=0.0) AskedFPS=0.01;

    // open the input file and give some info
    ret = InfoStreamer();
    if(ret < 0) return ret;
    // at this point, we now know the pipeline works

    // fire up the thread
    ThGr = std::thread([=] { StreamerThread(); });

    return 0;
}
//----------------------------------------------------------------------------------------
void TOstreamer::Pause(void)
{
    GrabPause.store(true);
}
//----------------------------------------------------------------------------------------
void TOstreamer::Continue(void)
{
    GrabPause.store(false);
}
//----------------------------------------------------------------------------------
int TOstreamer::InfoStreamer(void)
{
    CamFrames = 0;
    // Check if the folder/picture exists
    if(!filesystem::exists(PipeLine)) {
        PLOG_ERROR << CamName << " OpenCVstreamer: Error opening folder: " << PipeLine;
        return -1;
    }

    DirPics.clear();

    if(PipeInfo.CamFrames==1){
        //store the picture
        filesystem::path filePath = PipeLine;
        // Use extension() to get the file extension
        std::string extension = filePath.extension().string();
        if (extension == ".jpg" || extension == ".jpeg" || extension == ".png" || extension == ".bmp") {
            DirPics.push_back(PipeLine);
            CamFrames++;
        }
    }
    else{
        //store the file names
        std::vector<std::filesystem::directory_entry> Entries;
        for(const auto& entry : filesystem::directory_iterator(PipeLine)) {
            std::string extension = entry.path().extension().string();
            if (extension == ".jpg" || extension == ".jpeg" || extension == ".png" || extension == ".bmp") {
                DirPics.push_back(entry.path().string());
                CamFrames++;
            }
        }
    }

    if(CamFrames==0){
        PLOG_ERROR << CamName << " OpenCVstreamer: No suitable pictures found in the folder: " << PipeLine;
        return -1;
    }

    if(AskedFPS<=0.0) AskedFPS=1.0;
    PLOG_INFO << CamName << " OpenCVstreamer: FPS: " << AskedFPS;
    PLOG_INFO << CamName << " OpenCVstreamer: Pictures : " << CamFrames;

    return 0;
}
//----------------------------------------------------------------------------------
bool TOstreamer::GetFrame(cv::Mat& m)
{
    size_t i;
    bool Success=false;
    std::mutex mtx;

    if(GrabTag.load()!=PrevTag){
        //get frame
        {
            std::lock_guard<std::mutex> lock(mtx);
            Smat.copyTo(m);
        }   //unlock()
        //update tag
        PrevTag=GrabTag.load();

        //update FileName
        i = FileIndex.load();
        if(i>=0 && i<DirPics.size()){
            std::filesystem::path path_obj(DirPics[i]);
            FileName = path_obj.filename().string();
        }

        //let the other thread know we are ready
        GrabFetch.store(true);
        Success= (!m.empty());
    }
    return Success;
}
//----------------------------------------------------------------------------------
void TOstreamer::CloseGstreamer(void)
{
    GrabStop.store(true);

    if(ThGr.joinable()){
        ThGr.join();                //wait until the grab thread ends
    }
}
//----------------------------------------------------------------------------------
// Inside the Thread
//----------------------------------------------------------------------------------
void TOstreamer::StreamerThread(void)
{
    std::mutex mtx;
    size_t PicCnt = 0;

    Tag = 0;

    Tstart = std::chrono::steady_clock::now();

    while(GrabStop.load() == false){
        if(PicCnt<DirPics.size()){
            TmpMat = cv::imread(DirPics[PicCnt]);
            if(TmpMat.empty()){
                PLOG_ERROR << CamName << " OpenCVstreamer: Error loading picture: " << DirPics[PicCnt];
            }
            else{
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    // Resize the image if needed (keep TmpMat to its fixed sized)
                    if((PipeInfo.CamWidth!=TmpMat.cols) || (PipeInfo.CamHeight!=TmpMat.rows)){
                        cv::resize(TmpMat, Smat, cv::Size(PipeInfo.CamWidth, PipeInfo.CamHeight));
                    }
                    else{
                        TmpMat.copyTo(Smat);
                    }
                }   //unlock()
                FileIndex.store(PicCnt);
                GrabFetch.store(false);
                GrabTag.store(++Tag);                                                               //remember PrevTag=0 and Tag!=PrevTag to fetch
                ElapsedTime.store(static_cast<slong64>(static_cast<double>(PicCnt*1000)/CamFPS));   //note, time can jump back to 0, when the slide show rewinds!!!
            }
            //wait to meet FPS
            Tend  = std::chrono::steady_clock::now();
            Tgone = std::chrono::duration_cast<std::chrono::milliseconds> (Tend - Tstart).count();
            Twait = floor(1000.0/AskedFPS - Tgone);
            if(Twait > 0){
                std::this_thread::sleep_for(std::chrono::milliseconds(Twait));
            }
            Tstart = std::chrono::steady_clock::now();
            //loop ?
            PicCnt++;
            if(PicCnt == DirPics.size() && PipeInfo.CamLoop) PicCnt=0;
        }
        else{
            //pause -> wait 100 mSec
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
//----------------------------------------------------------------------------------
