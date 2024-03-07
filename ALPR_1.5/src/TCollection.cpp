#include "TWorker.h"

using namespace std;
//----------------------------------------------------------------------------------
TWorker::TWorker(void): ColStop(false)
{
}
//----------------------------------------------------------------------------------
TWorker::~TWorker(void)
{
    CloseCollection();
}
//----------------------------------------------------------------------------------
void TWorker::InitCollection(void)
{
//    int ret = 0;
//
//    // get the required settings from ThreadCam
//    PipeInfo = *Parent;
//    FPS      = PipeInfo.CamFPS;
//    AskedFPS = PipeInfo.CamFPS;
//    PipeLine = PipeInfo.Device;
//    CamName  = PipeInfo.CamName;
//
//    if(AskedFPS<=0.0) AskedFPS=0.01;
//
//    // open the input file and give some info
//    ret = InfoStreamer();
//    if(ret < 0) return ret;
//    // at this point, we now know the pipeline works
//
    // fire up the thread
    ThGr = std::thread([=] { LicenseThread(); });
//
//    return 0;
}
//----------------------------------------------------------------------------------
//int TWorker::InfoStreamer(void)
//{
//    CamFrames = 0;
//    // Check if the folder/picture exists
//    if(!filesystem::exists(PipeLine)) {
//        PLOG_ERROR << CamName << " OpenCVstreamer: Error opening folder: " << PipeLine;
//        return -1;
//    }
//
//    DirPics.clear();
//
//    if(PipeInfo.CamFrames==1){
//        //store the picture
//        filesystem::path filePath = PipeLine;
//        // Use extension() to get the file extension
//        std::string extension = filePath.extension().string();
//        if (extension == ".jpg" || extension == ".jpeg" || extension == ".png" || extension == ".bmp") {
//            DirPics.push_back(PipeLine);
//            CamFrames++;
//        }
//    }
//    else{
//        //store the file names
//        std::vector<std::filesystem::directory_entry> Entries;
//        for(const auto& entry : filesystem::directory_iterator(PipeLine)) {
//            std::string extension = entry.path().extension().string();
//            if (extension == ".jpg" || extension == ".jpeg" || extension == ".png" || extension == ".bmp") {
//                Entries.push_back(entry);
//            }
//            // Sort Entries alphabetically
//            std::sort(Entries.begin(), Entries.end(), [](const auto& a, const auto& b) {
//                return a.path() < b.path();
//            });
//            // Now Entries are sorted and stored
//            for (const auto& entry : Entries) {
//                DirPics.push_back(entry.path().string());
//                CamFrames++;
//            }
//        }
//    }
//
//    if(CamFrames==0){
//        PLOG_ERROR << CamName << " OpenCVstreamer: No suitable pictures found in the folder: " << PipeLine;
//        return -1;
//    }
//
//    if(AskedFPS<=0.0) AskedFPS=1.0;
//    PLOG_INFO << CamName << " OpenCVstreamer: FPS: " << AskedFPS;
//    PLOG_INFO << CamName << " OpenCVstreamer: Pictures : " << CamFrames;
//
//    return 0;
//}
//----------------------------------------------------------------------------------
//bool TWorker::GetFrame(cv::Mat& m)
//{
//    bool Success=false;
//    std::mutex mtx;
//
//    if(GrabTag.load()!=PrevTag){
//        //get frame
//        {
//            std::lock_guard<std::mutex> lock(mtx);
//            Smat.copyTo(m);
//        }   //unlock()
//        //update tag
//        PrevTag=GrabTag.load();
//        //let the other thread know we are ready
//        GrabFetch.store(true);
//        Success= (!m.empty());
//    }
//    return Success;
//}
//----------------------------------------------------------------------------------
void TWorker::CloseCollection(void)
{
    //clean up
    ColStop.store(true);

    cout << "Stop" << endl;

    if(ThGr.joinable()){
        ThGr.join();                //wait until the grab thread ends
    }
}
//----------------------------------------------------------------------------------
// Inside the Thread
//----------------------------------------------------------------------------------
void TWorker::LicenseThread(void)
{
//    std::mutex mtx;
//    size_t PicCnt = 0;
//
//    Tag = 0;
//
//    Tstart = std::chrono::steady_clock::now();

    while(ColStop.load() == false){
//        if(PicCnt<DirPics.size()){
//            TmpMat = cv::imread(DirPics[PicCnt]);
//            if(TmpMat.empty()){
//                PLOG_ERROR << CamName << " OpenCVstreamer: Error loading picture: " << DirPics[PicCnt];
//            }
//            else{
//                {
//                    std::lock_guard<std::mutex> lock(mtx);
//                    // Resize the image if needed (keep TmpMat to its fixed sized)
//                    if((PipeInfo.CamWidth!=TmpMat.cols) || (PipeInfo.CamHeight!=TmpMat.rows)){
//                        cv::resize(TmpMat, Smat, cv::Size(PipeInfo.CamWidth, PipeInfo.CamHeight));
//                    }
//                    else{
//                        TmpMat.copyTo(Smat);
//                    }
//                }   //unlock()
//                GrabFetch.store(false);
//                GrabTag.store(++Tag);                                                               //remember PrevTag=0 and Tag!=PrevTag to fetch
//                ElapsedTime.store(static_cast<slong64>(static_cast<double>(PicCnt*1000)/CamFPS));   //note, time can jump back to 0, when the slide show rewinds!!!
//            }
//            //wait to meet FPS
//            Tend  = std::chrono::steady_clock::now();
//            Tgone = std::chrono::duration_cast<std::chrono::milliseconds> (Tend - Tstart).count();
//            Twait = floor(1000.0/AskedFPS - Tgone);
//            if(Twait > 0){
//                std::this_thread::sleep_for(std::chrono::milliseconds(Twait));
//            }
//            Tstart = std::chrono::steady_clock::now();
//            //loop ?
//            PicCnt++;
//            if(PicCnt == DirPics.size() && PipeInfo.CamLoop) PicCnt=0;
//        }
//        else{
            //pause -> wait 10 mSec
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
//        }
    }
    cout << "Stop bye bye" << endl;

}
//----------------------------------------------------------------------------------
