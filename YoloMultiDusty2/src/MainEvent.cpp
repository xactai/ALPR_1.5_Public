#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include "General.h"
#include "ThreadCam.h"
#include "MJPG_sender.h"
#include "ProcessPipe.h"
#include "FPS.h"
#include "Tjson.h"

//----------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------
// put config.json global, so everbody can use the settings
Tjson           Js;

// all the pipes (usually the connected CCTVs)
TProcessPipe **ProPipe;

// some other global components
float           Temp;
TFPS            MyFPS;
yoloFastestv2   yoloF2;
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

    //init network
    yoloF2.init(true);      //we use the GPU of the Jetson Nano
    yoloF2.loadModel("yolo-fastestv2-opt.param","yolo-fastestv2-opt.bin");

    //start the engines
    ProPipe = new TProcessPipe*[Js.UsedStreams];
    for(int i=0; i<Js.UsedStreams; i++){
        ProPipe[i] = new TProcessPipe();
        ProPipe[i]->Init(i);
    }

    //start the threads
    for(int i=0; i<Js.UsedStreams; i++){
        ProPipe[i]->StartThread();
    }

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
            //send the result to port 8090
            send_mjpeg(FrameTotal, Js.MJPEG_Port, 500000, 70);
        }

        //show the overview on the screen.
        if(Js.PrintOnRender){
            //show the result
            cv::imshow("IP cameras",FrameTotal);
            char esc = cv::waitKey(5);
            if(esc == 27) break;
        }
    }

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

    return 0;
}
