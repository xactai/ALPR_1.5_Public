#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include "yolo_v2_class.hpp"	        // imported functions from .so
#include "General.h"
#include "ThreadCam.h"
#include "MJPGthread.h"
#include "ProcessPipe.h"
#include "FPS.h"
#include "Tjson.h"

//#define RECORD_VIDEO

//----------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------
// put config.json global, so everbody can use the settings
Tjson        Js;

// all the pipes (usually the connected CCTVs)
TProcessPipe **ProPipe;

// Darknet YoloV4-tiny
Detector YoloV4net("./models/yolov4-tiny.cfg", "./models/yolov4-tiny.weights");

// some other global components
float           Temp;
TFPS            MyFPS;
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
//----------------------------------------------------------------------------------
void GetComposedView(cv::Mat& Frame)          //returns all frames in one large picture
{
    cv::Mat Mdst;
    cv::Rect srcRect, dstRect;
    int x, y, t;
    int Wd, Ht;
    int Wn, Hn;
    int Wf, Hf;
    float Ratio=1.0;
    int TmbWd = Js.ThumbWidth;
    int TmbHt = Js.ThumbHeight;
    int NrX, NrY;                   //number of used thumb in x,y direction
    int Total = Js.UsedStreams;     //number of total thumbs
                                    //suppose  4 - 5 - 8 - 9 - 10   (= Total)
    NrX = ceil(sqrt(Total));        //you get  2 - 3 - 3 - 3 -  4   (ceil rounds upwards)
    NrY = ceil(((float)Total)/NrX); //you get  2 - 2 - 3 - 3 -  3   (always NrX * NrY >= Total)

    Frame = cv::Mat(NrY*TmbHt, NrX*TmbWd, CV_8UC3, cv::Scalar(128,128,128));

    for(y=0;y<NrY;y++){
        for(x=0;x<NrX;x++){
            t=x+y*NrX;
            if(t>=Total) break;

            //width or height may differ per frame. (depends on the camera settings)
            Wd=ProPipe[t]->MatAr.cols;
            Ht=ProPipe[t]->MatAr.rows;
            if(Wd!=TmbWd || Ht!=TmbHt){
                if(Wd > Ht){
                    Ratio = (float)TmbWd / Wd;
                    Wn = TmbWd;
                    Hn = Ht * Ratio;
                }
                else{
                    Ratio = (float)TmbHt / Ht;
                    Hn = TmbHt;
                    Wn = Wd * Ratio;
                }
                resize(ProPipe[t]->MatAr, Mdst, cv::Size(Wn,Hn), 0, 0, cv::INTER_LINEAR);
                //next place it proper in the large frame
                if(Hn<TmbHt){
                    Hf=(TmbHt-Hn)/2;
                    Mdst.copyTo(Frame(cv::Rect(x*TmbWd, y*TmbHt+Hf, TmbWd, Mdst.rows)));
                }
                else{
                    Wf=(TmbWd-Wn)/2;
                    Mdst.copyTo(Frame(cv::Rect(x*TmbWd+Wf, y*TmbHt, Mdst.cols, TmbHt)));
                }
            }
            else{
                ProPipe[t]->MatAr.copyTo(Frame(cv::Rect(x*TmbWd, y*TmbHt, TmbWd, TmbHt)));
            }
        }
    }
}
//---------------------------------------------------------------------------
int main()
{
    bool Success;
    cv::Mat FrameTotal;
    MJPGthread *MJ=NULL;

    //Js takes care for printing errors.
    Js.LoadFromFile("./config.json");

    Success = Js.GetSettings();
    if(!Success){
        return -1;
    }

    cout << "ALPR Version : " << Js.Version << endl;
    cout << "Used Streams : " << Js.UsedStreams << endl;
    cout << endl;

    if(Js.UsedStreams==0){
        cout << "No camera defined in the setting!\nPress <Ctrl>+<C> to quit" << endl;
        while(1) sleep(1);
        return 0;
    }

    //start the engines
    ProPipe = new TProcessPipe*[Js.UsedStreams];
    for(int i=0; i<Js.UsedStreams; i++){
        ProPipe[i] = new TProcessPipe();
        ProPipe[i]->Init(i);
    }

    if(Js.MJPEG_Port>7999){
        MJ = new MJPGthread();
        MJ->Init(Js.MJPEG_Port);
    }

    //start the threads
    for(int i=0; i<Js.UsedStreams; i++){
        ProPipe[i]->StartThread();
    }

    #ifdef RECORD_VIDEO
        //get an overview image
        GetComposedView(FrameTotal);
        cv::VideoWriter video("output.avi",cv::VideoWriter::fourcc('M','J','P','G'),10,cv::Size(FrameTotal.cols,FrameTotal.rows));
    #endif

    if(Js.PrintOnRender){
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
        for(int i=0; i<Js.UsedStreams; i++){
            ProPipe[i]->ExecuteCam();
        }

        //get an overview image
        GetComposedView(FrameTotal);

        //get the frame rate (just for debugging)
        putText(FrameTotal, cv::format("FPS %0.2f", MyFPS.FrameRate),cv::Point(10,20),cv::FONT_HERSHEY_SIMPLEX,0.6, cv::Scalar(0, 0, 255));
        MyFPS.Update();

        //show info on console
        if(!Js.PrintOnCli && !Js.PrintOnRender){
           std::cout << '\r' << "FPS : " << std::fixed << std::setprecision(2) << MyFPS.FrameRate << "  -  " << std::setprecision(1) <<  GetTemp() << " C" << std::flush;
        }

        //send the overview into the world
        if(Js.MJPEG_Port>7999){
            //send the result to the socket
            MJ->Send(FrameTotal);
        }

        //show the overview on the screen.
        if(Js.PrintOnRender){
            //show the result
            cv::imshow("IP cameras",FrameTotal);
            char esc = cv::waitKey(5);
            if(esc == 27) break;
        }

        #ifdef RECORD_VIDEO
            video.write(FrameTotal);
        #endif
    }

    #ifdef RECORD_VIDEO
        video.release();
    #endif

    cv::destroyAllWindows();

    //stop the threads
    for(int i=0; i<Js.UsedStreams; i++){
        ProPipe[i]->StopThread();
    }

    //now you can free the object
    for(int i=0; i<Js.UsedStreams; i++){
        delete ProPipe[i];
    }
    delete[] ProPipe;

    delete MJ;

    return 0;
}
