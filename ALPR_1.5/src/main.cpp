#include <iostream>
#include <opencv2/opencv.hpp>
#include <gflags/gflags.h>
#include <stdio.h>
#include "TMapper.h"
#include "FPS.h"
#include "MQTT.h"
#include "MJPGthread.h"

#define VERSION "1.5.0"

#define RECORD_VIDEO false

using namespace std;

//----------------------------------------------------------------------------------------
// Define the command line flags
//----------------------------------------------------------------------------------------

DEFINE_string(config, "./config.json", "location of the config.json settings file");
DEFINE_int32(verbose, 3, "verbose severity");
DEFINE_bool(bird_view, false, "shows diagnostic bird view screen");
DEFINE_bool(record, false, "record the output to ALPR_out.mp4 (slows processing time)");
DEFINE_string(mqtt_server, "localhost:1883", "MQTT server address");
DEFINE_string(mqtt_client_id, "Xinthe_parking", "MQTT client ID");

//----------------------------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------------------------

Tjson   Js;
TFPS    MyFPS;
TOCR    Ocr;
slong64 FrameCnt=0;

// Global Gflags
bool Gflag_Bird;

//the DNN models
Detector *YoloV4net;
Detector *PlateNet;
Detector *OcrNet;

//with their labels
vector<string> OCRnames;
vector<string> PlateNames;

//----------------------------------------------------------------------------------------
// format plog output
//----------------------------------------------------------------------------------------
namespace plog
{
    class MyFormatter
    {
    public:
        static util::nstring header() // This method returns a header for a new file. In our case it is empty.
        {
            return util::nstring();
        }

        static util::nstring format(const Record& record) // This method returns a string from a record.
        {
            util::nostringstream ss;

            if(FLAGS_verbose>4){
                ss << PLOG_NSTR("[") << record.getFunc() << PLOG_NSTR("@") << record.getLine() << PLOG_NSTR("] ");
            }
            ss << record.getMessage() << PLOG_NSTR("\n");

            return ss.str();
        }
    };
}
//----------------------------------------------------------------------------------
// Publish MQTT message with a JSON payload
void publishMQTTMessage(const string& topic, const string payload)
{
    mqtt_publish(topic, payload);
}
//----------------------------------------------------------------------------------
// Message handler for the MQTT subscription for any desired control channel topic
int handleMQTTControlMessages(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    printf("MQTT message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n\n", message->payloadlen, (char*)message->payload);

    return 1;
}
//----------------------------------------------------------------------------------
void GetComposedView(TMapper **ProPipe, cv::Mat& Frame)          //returns all frames in one large picture
{
    cv::Mat Mdst;
    cv::Rect srcRect, dstRect;
    int x, y, t;
    int Wd, Ht;
    int Wn, Hn;
    int Wf, Hf;
    float RatioWd=1.0;
    float RatioHt=1.0;
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
            if(!ProPipe[t]->Pic.empty()){
                //width or height may differ per frame. (depends on the camera settings)
                Wd=ProPipe[t]->Pic.cols;
                Ht=ProPipe[t]->Pic.rows;
                if(Wd!=TmbWd || Ht!=TmbHt){
                    RatioWd = (float)TmbWd / Wd;
                    RatioHt = (float)TmbHt / Ht;
                    if(RatioHt < RatioWd){
                        Wn = Wd * RatioHt;
                        Hn = Ht * RatioHt;
                    }
                    else{
                        Wn = Wd * RatioWd;
                        Hn = Ht * RatioWd;
                    }
                    resize(ProPipe[t]->Pic, Mdst, cv::Size(Wn,Hn), 0, 0, cv::INTER_LINEAR);
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
                    ProPipe[t]->Pic.copyTo(Frame(cv::Rect(x*TmbWd, y*TmbHt, TmbWd, TmbHt)));
                }
            }
        }
    }
}
//----------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    bool Success;
    TMapper **Maps;
    std::string Jstr;
    cv::Mat FrameTotal;
    cv::VideoWriter video;
    MJPGthread *MJ=nullptr;

#ifdef __aarch64__
    cout << "Detected a Jetson Nano or Jetson Orin Nano (aarch64-linux-gnu)" << endl << endl;
#else
    cout << "Detected a Linux PC (x86_64-linux-gnu)" << endl << endl;
#endif

    // Parse the flags
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    Gflag_Bird = FLAGS_bird_view;

    //export MQTT environment
    setenv("MQTT_SERVER", FLAGS_mqtt_server.c_str(), 1); // The third argument '1' specifies to overwrite if it already exists
    setenv("MQTT_CLIENT_ID", FLAGS_mqtt_client_id.c_str(), 1);

    //init logger
    static plog::ConsoleAppender<plog::MyFormatter> consoleAppender;
    plog::init(plog::verbose, &consoleAppender);

    switch(FLAGS_verbose){
        case 0 : plog::get()->setMaxSeverity(plog::none);    break;
        case 1 : plog::get()->setMaxSeverity(plog::fatal);   break;
        case 2 : plog::get()->setMaxSeverity(plog::error);   break;
        case 3 : plog::get()->setMaxSeverity(plog::warning); break;
        case 4 : plog::get()->setMaxSeverity(plog::info);    break;
        case 5 : plog::get()->setMaxSeverity(plog::debug);   break;
        case 6 : plog::get()->setMaxSeverity(plog::verbose); break;
        default: plog::get()->setMaxSeverity(plog::none);    break;
    }
    PLOG_INFO << "Loaded command line options";
    PLOG_INFO << "Loaded logger";

    // Load JSON
    // Js takes care for printing errors.
    Js.LoadFromFile(FLAGS_config);
    Success = Js.GetSettings();
    if(!Success){
        cout << "Error loading settings from the json file!\n\nPress <Ctrl>+<C> to quit" << endl;
        while(1) sleep(10);
        return 0;
    }
    if(Js.Version != VERSION){
        cout << "Wrong version of the json file!" << endl;
        cout << "\nSoftware version is " << VERSION << endl;
        cout << "JSON file is " << Js.Version << endl;
        cout << "\nPress <Ctrl>+<C> to quit" << endl;
        while(1) sleep(10);
        return 0;
    }
    PLOG_INFO << "Loaded JSON";

    gflags::SetVersionString(Js.Version);

	Detector MyYoloV4net(Js.Cstr+".cfg", Js.Cstr+".weights");
    YoloV4net = &MyYoloV4net;

	Detector MyPlateNet(Js.Lstr+".cfg", Js.Lstr+".weights");
    PlateNet = &MyPlateNet;

	Detector MyOcrNet(Js.Ostr+".cfg", Js.Ostr+".weights");
    OcrNet = &MyOcrNet;

    // Connect MQTT messaging
    if(mqtt_start(handleMQTTControlMessages) != 0) {
        cout << "MQTT NOT started: have you set the ENV varables?\nPress <Ctrl>+<C> to quit" << endl;
        while(1) sleep(10);
        return 0;
    }
    mqtt_connect();
	mqtt_subscribe("parking");
    PLOG_INFO << "Loaded MQTT";

	// load OCRnames
    ifstream file(Js.Ostr+".names");
	if(file.is_open()){
        for(string line; file >> line;) OCRnames.push_back(line);
        file.close();
        PLOG_INFO << "OCR chars loaded";
	}
	else{
        cout << "No OCR character names defined in the setting!\nPress <Ctrl>+<C> to quit" << endl;
        while(1) sleep(1);
        return 0;
	}

	//load PlateNames
    ifstream pfile(Js.Lstr+".names");
	if(pfile.is_open()){
        for(string line; pfile >> line;) PlateNames.push_back(line);
        pfile.close();
        PLOG_INFO << "Plates loaded";
	}
	else{
        cout << "No Plate names defined in the setting!\nPress <Ctrl>+<C> to quit" << endl;
        while(1) sleep(1);
        return 0;
	}

	//load number of used streams
    if(Js.UsedStreams==0){
        cout << "No camera defined in the setting!\nPress <Ctrl>+<C> to quit" << endl;
        while(1) sleep(10);
        return 0;
    }

	//make the folders
    if(Js.Json_Folder!="none"){   MakeDir(Js.Json_Folder);   PLOG_INFO << "Created Json folder"; }
    if(Js.FoI_Folder!="none"){    MakeDir(Js.FoI_Folder);    PLOG_INFO << "Created FoI folder"; }
    if(Js.Car_Folder!="none"){    MakeDir(Js.Car_Folder);    PLOG_INFO << "Created Car folder"; }
    if(Js.Plate_Folder!="none"){  MakeDir(Js.Plate_Folder);  PLOG_INFO << "Created Plate folder"; }
    if(Js.Render_Folder!="none"){ MakeDir(Js.Render_Folder); PLOG_INFO << "Created Render folder"; }

	//load MJPEG_Port
    MJ = new MJPGthread();
    if(Js.MJPEG_Port>7999){
        MJ->Init(Js.MJPEG_Port);
        PLOG_INFO << "Opened MJpeg port: " << Js.MJPEG_Port;
    }

    Maps = new TMapper*[Js.UsedStreams];
    for(int i=0; i<Js.UsedStreams; i++){
        Maps[i] = new TMapper();
        Maps[i]->Init(i);
        Maps[i]->Start();
    }
    PLOG_INFO << "Loaded Streams";
    PLOG_INFO << "Start streaming";

    if(FLAGS_record){
        //get an (empty) overview image to determine the required size.
        GetComposedView(Maps, FrameTotal);
        video.open("ALPR_out.mp4", cv::VideoWriter::fourcc('a','v','c','1'), 15, cv::Size(FrameTotal.cols, FrameTotal.rows));
        PLOG_INFO << "Loaded Video recorder";
    }

    if(Js.PrintOnRender){
        cout << "\nEntering main loop" << endl;
        cout << "Hit ESC to exit\n" << endl;
    }
    else{
        cout << "\nEntering main loop" << endl;
        cout << "Hit <Ctrl>+<C> to exit\n" << endl;
    }

    MyFPS.Init();

    FrameCnt=0;
    while(true){
        FrameCnt++;
        //process the different cameras
        for(int i=0; i<Js.UsedStreams; i++){
            Maps[i]->Execute();
        }

        GetComposedView(Maps, FrameTotal);

        //get the frame rate (just for debugging)
//        putText(FrameTotal, cv::format("%0.3f mSec", MyFPS.Latency),cv::Point(10,20),cv::FONT_HERSHEY_SIMPLEX,0.6, cv::Scalar(0, 0, 255));
        putText(FrameTotal, cv::format("FPS %0.2f", MyFPS.FrameRate),cv::Point(10,20),cv::FONT_HERSHEY_SIMPLEX,0.6, cv::Scalar(0, 0, 255));
        MyFPS.Update();

        //show info on console
        if(!Js.PrintOnCli && !Js.PrintOnRender){
           std::cout << '\r' << "FPS : " << std::fixed << std::setprecision(2) << MyFPS.FrameRate << "  -  " << std::fixed << std::setprecision(1) <<  GetTemp() << " C  " << std::flush;
        }

        //send the overview into the world
        if(Js.MJPEG_Port>7999){
            //send the result to the socket
            cv::Mat FrameMJPEG(Js.MJPEG_Height, Js.MJPEG_Width, CV_8UC3);
            cv::resize(FrameTotal,FrameMJPEG,FrameMJPEG.size(),0,0);
            MJ->Send(FrameMJPEG);
        }

        //store the frame_full only if directory name is valid
        //note it stores a MASSIVE bulk of pictures on your disk!
        if(Js.Render_Folder!="none"){
            cv::imwrite(Js.Render_Folder+"/"+to_string(FrameCnt)+"_utc.jpg", FrameTotal);
        }

        //show the overview on the screen.
        if(Js.PrintOnRender){
            //show the result
            cv::imshow("ALPR overview",FrameTotal);
            char esc = cv::waitKey(1);
            if(esc == 27) break;
            if(esc == 'p'){
                for(int i=0; i<Js.UsedStreams; i++) Maps[i]->Pause();

                while(esc != 'c' && esc != 27 )  esc = cv::waitKey(50);

                if(esc == 27) break;
                else for(int i=0; i<Js.UsedStreams; i++) Maps[i]->Continue();
            }
        }

        if(FLAGS_record) video.write(FrameTotal);
    }
    if(FLAGS_record){
        video.release();
        PLOG_INFO << "Closeded video recorder. Find your ALPR_out.mp4 in the working directory";
    }

    cv::destroyAllWindows();

    //now you can free the object
    for(int i=0; i<Js.UsedStreams; i++){
        delete Maps[i];
    }
    delete[] Maps;

    delete MJ;

    mqtt_disconnect();
    mqtt_close();

    // Clean up and exit
    gflags::ShutDownCommandLineFlags();

    PLOG_INFO << "Bye bye";

    return 0;
}
//----------------------------------------------------------------------------------
