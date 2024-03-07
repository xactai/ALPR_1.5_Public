#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include "General.h"
#include "ThreadCam.h"
#include "Settings.h"
#include "MJPG_sender.h"
#include "ProcessFrame.h"
#include "FPS.h"
#include "CamCluster.h"

#define VIEW_WIDTH  640
#define VIEW_HEIGHT 480

//----------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------
int       UsedCams;
float     FPS;
float     Temp;
TSettings MySettings(SETTING_FILE);         //the settings (they are loaded here for the first time)
TFPS      MyFPS;
yoloFastestv2 yoloF2;
//----------------------------------------------------------------------------------------
float GetTemp(void)
{
    ifstream myfile;
    float temp=0.0;
    string line;

    myfile.open("/sys/class/thermal/thermal_zone0/temp");
    if (myfile.is_open()) {
        getline(myfile,line);
        temp = stoi(line)/1000.0;
        myfile.close();
    }
    return temp;
}
//----------------------------------------------------------------------------------------
static std::mutex mtx_mjpeg;

void send_mjpeg(cv::Mat& mat, int port, int timeout, int quality)
{
    try {
        std::lock_guard<std::mutex> lock(mtx_mjpeg);
        static MJPG_sender wri(port, timeout, quality);

        wri.write(mat);
    }
    catch (...) {
        std::cerr << " Error in send_mjpeg() function \n";
    }
}
//---------------------------------------------------------------------------
int main()
{
    cv::Mat FrameTotal;

    TCamCluster Cams(VIEW_WIDTH, VIEW_HEIGHT);

    TProcessFrame Prm(&Cams);

    //init some variables
    FPS =0.0; UsedCams=0;

    //init network
    yoloF2.init(true);      //we use the GPU of the Jetson Nano
    yoloF2.loadModel("yolo-fastestv2-opt.param","yolo-fastestv2-opt.bin");

    std::vector< std::string > RTSPstr;

    if(MySettings.CCTV1!="" && MySettings.CCTV1!="none"){
        UsedCams++; RTSPstr.push_back(MySettings.CCTV1);
        if(MySettings.CCTV2!="" && MySettings.CCTV2!="none"){
            UsedCams++; RTSPstr.push_back(MySettings.CCTV2);
            if(MySettings.CCTV3!="" && MySettings.CCTV3!="none"){
                UsedCams++; RTSPstr.push_back(MySettings.CCTV3);
                if(MySettings.CCTV4!="" && MySettings.CCTV4!="none"){
                    UsedCams++; RTSPstr.push_back(MySettings.CCTV4);
                }
            }
        }
    }

    if(UsedCams==0){
        cout << "No camera defined in the setting!\nPress <Ctrl>+<C> to quit" << endl;
        while(1) sleep(1);
        return 0;
    }

    Cams.Start(RTSPstr);        //run the pipes

    if(MySettings.ShowOutput){
        cout << "Entering main loop" << endl;
        cout << "Hit ESC to exit\n" << endl;
    }
    else{
        cout << "Entering main loop" << endl;
        cout << "Hit <Ctrl>+<C> to exit\n" << endl;
    }

    MyFPS.Init();

    while(true)
    {
        //process the different cameras
        for(int i=0; i<UsedCams; i++){
            Prm.ExecuteFrame(i);
        }

        //get an overview image
        Cams.GetComposedView(FrameTotal);

        //get the frame rate (just for debugging)
        FPS=MyFPS.FrameRate;
        putText(FrameTotal, cv::format("FPS %0.2f", FPS),cv::Point(10,20),cv::FONT_HERSHEY_SIMPLEX,0.6, cv::Scalar(0, 0, 255));
        MyFPS.Update();

        //show info on console
        if(!MySettings.PrintOutput){
           std::cout << '\r' << "FPS : " << std::fixed << std::setprecision(2) << FPS << "  -  " << std::setprecision(1) <<  GetTemp() << " C" << std::flush;
        }

        //send the overview into the world
        if(MySettings.MjpegPort>7999){
            //send the result to port 8090
            send_mjpeg(FrameTotal, MySettings.MjpegPort, 500000, 70);
        }

        //show the overview on the screen.
        if(MySettings.ShowOutput){
            //show the result
            cv::imshow("IP cameras",FrameTotal);
            char esc = cv::waitKey(5);
            if(esc == 27) break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}
