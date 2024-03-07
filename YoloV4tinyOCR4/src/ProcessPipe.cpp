#include "General.h"
#include "ProcessPipe.h"
#include "Tjson.h"
#include "yolo_v2_class.hpp"	        // imported functions from .so
#include <iostream>
#include <chrono>
#include <ctime>

//----------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------

extern Tjson    Js;
extern Detector *YoloV4net;
extern Detector *PlateNet;
extern size_t   FrameCnt;
extern ofstream dbf;

extern void publishMQTTMessage(const string& topic, const string payload);

//---------------------------------------------------------------------------
static const unsigned char MarkColor[6][3] = {{0, 137, 114}, {0, 114, 137},
                                              {137, 0, 114}, {114, 0, 137},
                                              {137, 114, 0}, {114, 137, 0}};
//---------------------------------------------------------------------------
static const char* class_names[] = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
    "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"
};
//---------------------------------------------------------------------------
static cv::Mat MorphKernal = cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 11, 11 ), cv::Point( 5, 5 ) );
//---------------------------------------------------------------------------
// TProcessPipe
//---------------------------------------------------------------------------
TProcessPipe::TProcessPipe(void): DnnRect(0,0,WORK_WIDTH,WORK_HEIGHT)
{
    CamNr  = -1;
    MJpeg  = 0;
    CamGrb = NULL;
    MJ     = NULL;
}
//---------------------------------------------------------------------------
TProcessPipe::~TProcessPipe(void)
{
    if(MJ     != NULL) delete MJ;
    if(CamGrb != NULL) delete CamGrb;
}
//---------------------------------------------------------------------------
void TProcessPipe::Init(const int Nr)
{
    Tjson TJ;

    TJ.Jvalid = true;       //force the json to be valid, else no GetSetting() will work
    CamNr     = Nr;
    Width     = Js.WorkWidth;
    Height    = Js.WorkHeight;

    CamGrb = new ThreadCam(Width,Height);

    MatEmpty = cv::Mat(Height, Width, CV_8UC3, cv::Scalar(128,128,128));

    MatEmpty.copyTo(MatAr);

    json Jstr=Js.j["STREAM_"+std::to_string(CamNr+1)];

    TJ.GetSetting(Jstr,"MJPEG_PORT",MJpeg);
    if(MJpeg>7999){
        MJ = new MJPGthread();
        MJ->Init(MJpeg);
    }
    if(!TJ.GetSetting(Jstr,"CAM_NAME",CamName)) CamName=to_string(Nr+1);
    if(!TJ.GetSetting(Jstr,"MQTT_TOPIC",TopicName)) TopicName=CamName;
    TJ.GetSetting(Jstr,"WARP",Warp); if(Warp<1.0) Warp=1.0;
    TJ.GetSetting(Jstr,"GAIN",Gain); if(Gain<1.0) Gain=1.0;

    //get DNN rect
    if(!TJ.GetSetting(Jstr["DNN_Rect"],"x_offset",DnnRect.x))     std::cout << "Using default value" << std::endl;
    if(!TJ.GetSetting(Jstr["DNN_Rect"],"y_offset",DnnRect.y))     std::cout << "Using default value" << std::endl;
    if(!TJ.GetSetting(Jstr["DNN_Rect"],"width",DnnRect.width))    std::cout << "Using default value" << std::endl;
    if(!TJ.GetSetting(Jstr["DNN_Rect"],"height",DnnRect.height))  std::cout << "Using default value" << std::endl;

    //check DNN rect with our camera
    if(DnnRect.x     <     0) DnnRect.x     = 0;
    if(DnnRect.width > Width) DnnRect.width = Width;
    if(DnnRect.y     <     0) DnnRect.y     = 0;
    if(DnnRect.height>Height) DnnRect.height = Height;
    if((DnnRect.x+DnnRect.width ) >  Width) DnnRect.x=Width -DnnRect.width;
    if((DnnRect.y+DnnRect.height) > Height) DnnRect.y=Height-DnnRect.height;

    //get MQTT client ID
//    if(!TJ.GetSetting(Jstr["DNN_Rect"],"x_offset",DnnRect.x))     std::cout << "Using default value" << std::endl;

    UseNetRect = (DnnRect.x!= 0 || DnnRect.y!=0 || DnnRect.width!=Width || DnnRect.height!=Height);
    MapperRdy = false;
}
//---------------------------------------------------------------------------
void TProcessPipe::StartThread(void)
{
    std::string Jstr="STREAM_"+std::to_string(CamNr+1);

    CamGrb->Start(Js.j[Jstr]);
}
//---------------------------------------------------------------------------
void TProcessPipe::StopThread(void)
{
    CamGrb->Quit();
}
//----------------------------------------------------------------------------------------
void TProcessPipe::ExecuteCam(void)
{
    int x,y,b;
    int Wd,Ht;
    int Wn,Hn;
    int Wf,Hf;
    cv::Mat Mdst;
    bool Success = false;
    float Ratio=1.0;

    if(CamGrb->PipeLine=="") return;

    Success = CamGrb->GetFrame(MatAr);

    if(Success){
        //width or height may differ. (depends on the camera settings)
        Wd=MatAr.cols;
        Ht=MatAr.rows;

        if(Wd!=Width || Ht!=Height){
            if(Wd > Ht){
                Ratio = (float)Width / Wd;
                Wn = Width;
                Hn = Ht * Ratio;
            }
            else{
                Ratio = (float)Height / Ht;
                Hn = Height;
                Wn = Wd * Ratio;
            }
            resize(MatAr, Mdst, cv::Size(Wn,Hn), 0, 0, cv::INTER_LINEAR); // resize to 640xHn or Wnx480 resolution
            //now work with the Mdst frame
            ProcessDNN(Mdst);

            //next place it in a proper 640x480 frame
            MatEmpty.copyTo(MatAr);
            if(Hn<Height){
                Hf=(Height-Hn)/2;
                Mdst.copyTo(MatAr(cv::Rect(0, Hf, Width, Mdst.rows)));
            }
            else{
                Wf=(Width-Wn)/2;
                Mdst.copyTo(MatAr(cv::Rect(Wf, 0, Mdst.cols, Height)));
            }
        }
        else{
            //width and height are equal the view, work direct on MatAr
            ProcessDNN(MatAr);
        }
    }
    else MatEmpty.copyTo(MatAr);

    //place name at the corner
    cv::Size label_size = cv::getTextSize(CamGrb->CamName.c_str(), cv::FONT_HERSHEY_SIMPLEX, 0.6, 1, &b);
    x = Width  - label_size.width - 4;
    y = Height - b - 4;
    cv::putText(MatAr, CamGrb->CamName.c_str(), cv::Point(x,y), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 0));

    //send the result to the socket
    if(MJpeg>0){
        cv::Mat FrameMJPEG(Js.MJPEG_Height, Js.MJPEG_Width, CV_8UC3);
        cv::resize(MatAr,FrameMJPEG,FrameMJPEG.size(),0,0);
        MJ->Send(FrameMJPEG);
    }

}
//----------------------------------------------------------------------------------
void TProcessPipe::ProcessDNN(cv::Mat& frame)
{
    cv::Mat DNNframe;
    std::vector<bbox_t> BoxObjects;

    FrameWidth =frame.cols;
    FrameHeight=frame.rows;

    //store the frame_full only if directory name is valid
    //note it stores a MASSIVE bulk of pictures on your disk!
    if(Js.FoI_Folder!="none"){
        FoiDirName = Js.FoI_Folder+"/"+CamName+"-"+to_string(FrameCnt)+"_utc.jpg";
        cv::imwrite(FoiDirName, frame);
    }

    if(UseNetRect){
        //initial check, no need to check every frame because all variables will never change any more
        if(!MapperRdy){
            if(DnnRect.x     <          0) DnnRect.x      = 0;
            if(DnnRect.width > FrameWidth) DnnRect.width  = FrameWidth;
            if(DnnRect.y     <          0) DnnRect.y      = 0;
            if(DnnRect.height>FrameHeight) DnnRect.height = FrameHeight;
            if((DnnRect.x+DnnRect.width ) > FrameWidth ) DnnRect.x=FrameWidth -DnnRect.width;
            if((DnnRect.y+DnnRect.height) > FrameHeight) DnnRect.y=FrameHeight-DnnRect.height;

            Map.Init(CamNr,DnnRect.width,DnnRect.height,Warp,Gain,CamName,DnnRect);
            MapperRdy = true;
        }
        frame(DnnRect).copyTo(DNNframe);

        BoxObjects = YoloV4net->detect(DNNframe);

        //shift the outcome to its position in the original frame
        for(size_t i = 0; i < BoxObjects.size(); i++) {
            BoxObjects[i].x += DnnRect.x;
            BoxObjects[i].y += DnnRect.y;
        }
    }
    else{
        if(!MapperRdy){
            Map.Init(CamNr,FrameWidth,FrameHeight,Warp,Gain,CamName,DnnRect);
            MapperRdy = true;
        }

        BoxObjects = YoloV4net->detect(frame);
    }

    RepairRareLabels(BoxObjects,6,CAR);         //train    -> car
    RepairRareLabels(BoxObjects,7,CAR);         //truck    -> car
    RepairRareLabels(BoxObjects,28,CAR);        //suitcase -> car
    RepairRareLabels(BoxObjects,1,MOTORBIKE);   //bicycle  -> motorcycle

    //get the tracking ids
    BoxObjects = YoloV4net->tracking_id(BoxObjects,true,50,80);
//    BoxObjects = YoloV4net.tracking_id(BoxObjects,true,20,80);

    Map.Update(frame, BoxObjects);

    SnapShot.Update(Map);

    //send json into the world (port 8070)
    //must be called after Map.Update()
    if(Js.JSON_Port>0 || Js.MQTT_On){
        string send_str;

        GetJson(send_str);
        if(Js.JSON_Port>0) SendJsonHTTP(send_str, Js.JSON_Port);    //send json to port 8070
        if(Js.MQTT_On) publishMQTTMessage(TopicName,send_str);      //send mqtt into the world
    }


//    DrawObjectsLabels(frame, BoxObjects);
    DrawObjects(frame, BoxObjects);
}
//---------------------------------------------------------------------------
void TProcessPipe::RepairRareLabels(std::vector<bbox_t>& boxes,const unsigned int Lreplace,const unsigned int Ltarget)
{
    size_t i, n;
    cv::Rect Rc, Rt, Ro;
    int St, So;
    float Sd, Se;
    bool found;

    for(i=0; i<boxes.size(); i++){
        if(boxes[i].obj_id == Lreplace){
            Rt =cv::Rect(boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h);
            //see if we have a car in the list with approx the same size
            found=false;
            for(Se=0.5, n=0; n<boxes.size(); n++){
                if(boxes[n].obj_id == Ltarget){
                    Rc =cv::Rect(boxes[n].x,boxes[n].y,boxes[n].w,boxes[n].h);
                    //get overlap (So) between the two boxes
                    Ro = Rc & Rt; St=Rt.area(); So=Ro.area();
                    if(So>0){
                        Sd=fabs((((float)St)/So)-1.0);
                        //find the best matching overlap
                        //the more the overlap equals the size of the truck box,
                        //the better the fit.
                        //start with 0.5 - 1.5, hence Se=0.5 in the for declaration
                        if(Sd<Se){  Se=Sd; found=true;  }
                    }
                }
            }
            if(!found){
                //truck only -> lets label it as car
                boxes[i].obj_id=Ltarget;
            }
            else{
                //truck equals a car in size and position -> remove the entry
                boxes.erase(boxes.begin() + i);
                i--;
            }
        }
    }
}
//---------------------------------------------------------------------------
void TProcessPipe::DrawObjectsLabels(cv::Mat& frame, const std::vector<bbox_t>& boxes)
{
    char text[256];
    size_t i;
    int x,y,baseLine;

    //DNN_rect
    cv::rectangle(frame, cv::Point(DnnRect.x, DnnRect.y),
                         cv::Point(DnnRect.x+DnnRect.width, DnnRect.y+DnnRect.height), cv::Scalar(255,255,0),3);

    //the objects
    for(i = 0; i < boxes.size(); i++) {
//            sprintf(text, " %i ", boxes[i].track_id);
        sprintf(text, " %s ", class_names[boxes[i].obj_id]);

        baseLine = 0;
        cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, &baseLine);

        x = boxes[i].x;
        y = boxes[i].y - label_size.height - baseLine;
        if (y < 0) y = 0;
        if (x + label_size.width > frame.cols) x = frame.cols - label_size.width;

        cv::rectangle(frame, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

        cv::putText(frame, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);

        cv::rectangle(frame, cv::Point(boxes[i].x, boxes[i].y),
                              cv::Point(boxes[i].x+boxes[i].w, boxes[i].y+boxes[i].h), cv::Scalar(255, 255, 25), 2);
    }
}
//---------------------------------------------------------------------------
void TProcessPipe::DrawObjects(cv::Mat& frame, const std::vector<bbox_t>& boxes)
{
    char text[256];
    size_t i;
    int x,y,baseLine;
    int x1,y1,x2,y2;
    bool Valid;
    cv::Rect RtPlate;

    if(Js.PrintOnCli){
        cout << CamGrb->CamName << endl;
        for(i = 0; i < boxes.size(); i++){
            cout <<boxes[i].prob<<" - "<< class_names[boxes[i].obj_id] << " - "<< boxes[i].track_id << endl;
        }
    }

    if(Js.PrintOnRender || Js.MJPEG_Port>7999){
        //DNN_rect
        cv::rectangle(frame, cv::Point(DnnRect.x, DnnRect.y),
                             cv::Point(DnnRect.x+DnnRect.width, DnnRect.y+DnnRect.height), cv::Scalar(255,255,0),3);

        //Portal
        Map.Portal.Border.Get(-250,x1,y1);
        Map.Portal.Border.Get( 250,x2,y2);
        cv::line(frame,cv::Point(x1,Map.InvWarp(y1)),cv::Point(x2,Map.InvWarp(y2)),cv::Scalar(0, 255, 128),3);

        //the objects
        for(i = 0; i < boxes.size(); i++) {
            //see if it is a known object
            if(Map.GetInfo(boxes[i],Valid,RtPlate)){
                if(Valid){
                    //get a color
                    const unsigned char* Mcol = MarkColor[boxes[i].track_id % 6];
                    cv::Scalar mc(Mcol[0],Mcol[1],Mcol[2]);

                    sprintf(text, " %i ", boxes[i].track_id);
    //                    sprintf(text, " %s ", class_names[boxes[i].obj_id]);

                    baseLine = 0;
                    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, &baseLine);

                    x = boxes[i].x;
                    y = boxes[i].y - label_size.height - baseLine;
                    if (y < 0) y = 0;
                    if (x + label_size.width > frame.cols) x = frame.cols - label_size.width;

                    //white background for label
//                    cv::rectangle(frame, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
//                                  cv::Scalar(255, 255, 255), -1);


                    cv::putText(frame, text, cv::Point(x, y + label_size.height),
                                cv::FONT_HERSHEY_SIMPLEX, 0.8, mc, 2);


                    cv::rectangle(frame, cv::Point(boxes[i].x, boxes[i].y),
                                          cv::Point(boxes[i].x+boxes[i].w, boxes[i].y+boxes[i].h), mc, 2);

                    if(RtPlate.width>0) cv::rectangle(frame,RtPlate,cv::Scalar(0, 255, 0),2);
                }
                else{
                    cv::rectangle(frame, cv::Point(boxes[i].x, boxes[i].y),
                                         cv::Point(boxes[i].x+boxes[i].w, boxes[i].y+boxes[i].h), cv::Scalar(255,0,0));
                }

            }
            else{
                cv::rectangle(frame, cv::Point(boxes[i].x, boxes[i].y),
                                     cv::Point(boxes[i].x+boxes[i].w, boxes[i].y+boxes[i].h), cv::Scalar(255,0,0));
            }
         }

        SnapShot.Show(frame);
    }
}
//----------------------------------------------------------------------------------------
bool TProcessPipe::SendJsonHTTP(std::string send_json, int port, int timeout)
{
    if(Js.Json_Folder!="none"){
        ofstream Jfile(Js.Json_Folder+"/"+CamName+"-"+to_string(FrameCnt)+".json");
        if(Jfile.is_open()){
            Jfile << send_json;
            Jfile.close();
        }
    }

    send_json_custom(send_json.c_str(), port, timeout);

    return true;
}
//----------------------------------------------------------------------------------------
void TProcessPipe::GetJson(std::string &SendJson)
{
    bool PostComma;
    std::ostringstream os;

    os << "{\n\t\"version\": \"1.0\",";
    os << "\n\t\"camId\": \"" << CamName << "\",";
    os << "\n\t\"frameId\": " << FrameCnt << ",";
    if(Js.FoI_Folder!="none")  os << "\n\t\"frameImg\": \"" << FoiDirName << "\",";
    else                       os << "\n\t\"frameImg\": null,";
    os << "\n\t\"frameWidth\":" << FrameWidth << ",";
    os << "\n\t\"frameHeight\":" << FrameHeight << ",";
    //vehicles
    os << "\n\t\"vehicleDetails\": [";
    PostComma=false;
    for(size_t i=0; i<Map.MapList.size(); i++){
        if(Map.MapList[i].obj_id!=PERSON && Map.MapList[i].Valid && Map.MapList[i].Tick == FrameCnt){
            AddJsonVehicle(os,i,PostComma);
            PostComma=true;
        }
    }
    os << "\n\t],";
    //persons
    os << "\n\t\"personsDetails\": [";
    PostComma=false;
    for(size_t i=0; i<Map.MapList.size(); i++){
        if(Map.MapList[i].obj_id==PERSON && Map.MapList[i].Valid && Map.MapList[i].Tick == FrameCnt){
            AddJsonPerson(os,i,PostComma);
            PostComma=true;
        }
    }
    os << "\n\t]" << "\n}";

    SendJson = os.str();
}
//----------------------------------------------------------------------------------------
void TProcessPipe::AddJsonVehicle(std::ostringstream& os,const int i,const bool Comma)
{
//    std::string vehicleFrame=Map.MapList[i].Name;
//    std::string licensePlateFrame=Map.MapList[i].PlateName;
//    std::string licensePlateText=Map.MapList[i].PlateOCR;

    if(Comma) os << ",";      //at the end of the last entry

    os << "\n\t\t{";
    os << "\n\t\t\t\"vehicleId\": " << i << ",";
    os << "\n\t\t\t\"vehicleTrack\": " << Map.MapList[i].track_id << ",";
    if(Map.MapList[i].obj_id==CAR) os << "\n\t\t\t\"vehicleType\": \"Four_wheeler\",";
    else                           os << "\n\t\t\t\"vehicleType\": \"Two_wheeler\",";
    if(Js.Car_Folder!="none")      os << "\n\t\t\t\"vehicleFrame\": \"" << Map.MapList[i].Name <<"\",";
    else                           os << "\n\t\t\t\"vehicleFrame\": null,";
    os << "\n\t\t\t\"vehicleRect\": [" << Map.MapList[i].x << ", " << Map.MapList[i].y << ", " << Map.MapList[i].w << ", " << Map.MapList[i].h << "],";
    if(Map.MapList[i].PlatePlot.width > 0.0){
        os << "\n\t\t\t\"isNPDetected\": \"T\",";
        os << "\n\t\t\t\"licensePlateFrame\": \"" << Map.MapList[i].PlateName << "\",";
        os << "\n\t\t\t\"licensePlateRect\": ["  << Map.MapList[i].PlatePlot.x << ", " << Map.MapList[i].PlatePlot.y << ", " << Map.MapList[i].PlatePlot.width << ", " << Map.MapList[i].PlatePlot.height << "],";
        if(Js.FrameOcrOn) os << "\n\t\t\t\"licensePlateText\": \"" << Map.MapList[i].PlateOCR << "\",";
        else              os << "\n\t\t\t\"licensePlateText\": null,";
    }
    else{
        os << "\n\t\t\t\"isNPDetected\": \"F\",";
        os << "\n\t\t\t\"licensePlateFrame\": null,";
        os << "\n\t\t\t\"licensePlateRect\": null,";
        os << "\n\t\t\t\"licensePlateText\": null,";
    }
    if(Map.MapList[i].Inspected) os << "\n\t\t\t\"isInspected\": \"T\",";
    else                         os << "\n\t\t\t\"isInspected\": \"F\",";
    os << "\n\t\t\t\"vehicleDetectedDateTimeUTC\": \""+NiceTimeUTC(Map.MapList[i].Tstart)+"\"";
    os << "\n\t\t}";
}
//----------------------------------------------------------------------------------------
void TProcessPipe::AddJsonPerson(std::ostringstream& os,const int i,const bool Comma)
{
    if(Comma) os << ",";      //at the end of the last entry

    os << "\n\t\t{";
    os << "\n\t\t\t\"personId\": " << i << ",";
    os << "\n\t\t\t\"personTrack\": " <<  Map.MapList[i].track_id << ",";
    os << "\n\t\t\t\"personRect\": [" << Map.MapList[i].x << ", " << Map.MapList[i].y << ", " << Map.MapList[i].w << ", " << Map.MapList[i].h << "],";
    os << "\n\t\t}";
}
//----------------------------------------------------------------------------------

