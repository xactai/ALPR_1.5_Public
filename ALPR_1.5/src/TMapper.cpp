#include "TMapper.h"
#include "Tjson.h"
#include "General.h"
#include "FPS.h"
#include "Kmain.h"
//#include "TSnapShot.h"
//----------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------

extern slong64 FrameCnt;
extern Detector *PlateNet;
extern Tjson  Js;
extern TFPS   MyFPS;
extern bool   Gflag_Bird;

extern void publishMQTTMessage(const string& topic, const string payload);
//---------------------------------------------------------------------------
static cv::Mat MorphKernal = cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 11, 11 ), cv::Point( 5, 5 ) );
//---------------------------------------------------------------------------
static const char* color_names[] = {
    "Red", "Yellow", "Green", "Cyan", "Blue", "Magenta", "Black", "Grey", "White", " ?? "
};
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
//----------------------------------------------------------------------------------------
TMapper::TMapper()
{
}
//----------------------------------------------------------------------------------------
TMapper::~TMapper()
{
}
//----------------------------------------------------------------------------------------
void TMapper::Init(const int Nr)
{
    int i;
    Tjson TJ;
    std::string Str;

    InitPipe(Nr);           //Init ProcessPipe

    TJ.Jvalid = true;       //force the json to be valid, else no GetSetting() will work

    json Jstr=Js.j["STREAM_"+std::to_string(CamIndex+1)];

    TJ.GetSetting(Jstr,"WARP",Warp); if(Warp<1.0) Warp=1.0;
    TJ.GetSetting(Jstr,"GAIN",Gain); if(Gain<1.0) Gain=1.0;
    TJ.GetSetting(Jstr,"PORTAL_WIDTH",PortalWidth);
    TJ.GetSetting(Jstr,"MIN_4_SEC",MinFourSec);
    TJ.GetSetting(Jstr,"MIN_2_SEC",MinTwoSec);
    TJ.GetSetting(Jstr,"MJPEG_PORT",MJpegPort);

    if(!TJ.GetSetting(Jstr,"MQTT_TOPIC",TopicName)) TopicName=CamName;
    if(!TJ.GetSetting(Jstr,"MIN_SIZE_PIC",MinSizePic)) MinSizePic=50000;   //(50000 is 250x200 pixels)

    //get BoI rect
    BoISpeed=100.0;     //default, unrealistic high, always true
    if(!TJ.GetSetting(Jstr["BOI_Rect"],"x_offset",BoIRect.x))     std::cout << "Using default value" << std::endl;
    if(!TJ.GetSetting(Jstr["BOI_Rect"],"y_offset",BoIRect.y))     std::cout << "Using default value" << std::endl;
    if(!TJ.GetSetting(Jstr["BOI_Rect"],"width",BoIRect.width))    std::cout << "Using default value" << std::endl;
    if(!TJ.GetSetting(Jstr["BOI_Rect"],"height",BoIRect.height))  std::cout << "Using default value" << std::endl;
    if(!TJ.GetSetting(Jstr["BOI_Rect"],"speed",BoISpeed))         std::cout << "Using default value" << std::endl;

    //check BoI rect with our camera
    if(BoIRect.x < 0) BoIRect.x = 0;
    if(BoIRect.y < 0) BoIRect.y = 0;
    if(BoIRect.width >  CamWidth){
        PLOG_WARNING << "\nBoI_rect width (" << BoIRect.width <<") larger than working width (" << CamWidth << ") !\n";
        BoIRect.width  = CamWidth;
    }
    if(BoIRect.height > CamHeight){
        PLOG_WARNING << "\nBoI_rect height (" << BoIRect.height <<") larger than working height (" << CamHeight << ") !\n";
        BoIRect.height = CamHeight;
    }
    if((BoIRect.x+BoIRect.width ) >  CamWidth){
        PLOG_WARNING << "\nBoI_rect x (" << BoIRect.x <<") + BoI_rect width (" << BoIRect.width <<") larger than working width (" << CamWidth << ") !\n";
        BoIRect.x = CamWidth - BoIRect.width;
    }
    if((BoIRect.y+BoIRect.height) > CamHeight){
        PLOG_WARNING << "\nBoI_rect y (" << BoIRect.y <<") + BoI_rect height (" << BoIRect.height <<") larger than working height (" << CamHeight << ") !\n";
        BoIRect.y = CamHeight- BoIRect.height;
    }

    //get trigger
    Trigger = 0;        //set default
    if(!TJ.GetSetting(Jstr,"TRIGGER",Str)) Str="portal";
    //investigate Str in case of a wall
    lcase(Str); trim(Str);
    if(Str=="portal") Trigger = TRIG_PORTAL;
    else{
        if(Str=="wall" || Str=="trigger_wall"){
            TJ.GetSetting(Jstr["TRIGGER_WALL"],  "left",i); if(i==1) Trigger|=TRIG_LEFT;
            TJ.GetSetting(Jstr["TRIGGER_WALL"],   "top",i); if(i==1) Trigger|=TRIG_TOP;
            TJ.GetSetting(Jstr["TRIGGER_WALL"], "right",i); if(i==1) Trigger|=TRIG_RIGHT;
            TJ.GetSetting(Jstr["TRIGGER_WALL"],"bottom",i); if(i==1) Trigger|=TRIG_BOTTOM;
        }
        if(Trigger==0) Trigger = TRIG_PORTAL;  //default in case of no wall active
    }

    Portal.Init(to_string(CamIndex),PortalWidth);
}
//----------------------------------------------------------------------------------------
void TMapper::Execute(void)
{
    if(!CamWorking) return;     //still initializing

    if(ExecuteDNN()){
        switch(CamSource){
            case USE_NONE    : { break; }
            case USE_PICTURE : { MapStatic(); break; }
            case USE_FOLDER  : { MapStatic(); break; }
            case USE_VIDEO   : { MapDynamic(); break; }
            case USE_CAMERA  : { MapDynamic(); break; }
        }
    //    DrawObjectsLabels();
        DrawObjects();
    }
}
//----------------------------------------------------------------------------------------
// MapDynamic.
//----------------------------------------------------------------------------------------
void TMapper::MapDynamic(void)
{
    size_t i, n, b;
    int Xo, Yo, Sz, Pz;
    bool Valid=false;
    bool Found=false;
    std::string Str;
    char ChrCar;

    MQTTevent="";           //clear MQTT event string
    ChrCar='a';
    ChrCar--;               //due to ChrCar++ first before using

    //----------------------------------------------
    //place some safeguards and find the biggest car
    //----------------------------------------------
    for(b=0;b<BoxObjects.size();b++){
        if((int)BoxObjects[b].x <         0) BoxObjects[b].x = 0;
        if((int)BoxObjects[b].w >  CamWidth) BoxObjects[b].w = CamWidth;
        if((int)BoxObjects[b].y <         0) BoxObjects[b].y = 0;
        if((int)BoxObjects[b].h > CamHeight) BoxObjects[b].h = CamHeight;
        if((int)(BoxObjects[b].x+BoxObjects[b].w) >  CamWidth) BoxObjects[b].x=CamWidth -BoxObjects[b].w;
        if((int)(BoxObjects[b].y+BoxObjects[b].h) > CamHeight) BoxObjects[b].y=CamHeight-BoxObjects[b].h;
        Sz=BoxObjects[b].w*BoxObjects[b].h;
        //misuse the unused frames_counter
        BoxObjects[b].frames_counter = Sz;
    }
    //----------------------------------------------
    //check if it is new object
    //----------------------------------------------
    for(b=0;b<BoxObjects.size();b++){
        //get some info from the object
        Xo=BoxObjects[b].x+(BoxObjects[b].w/2);
        Yo=DoWarp(BoxObjects[b].y+(BoxObjects[b].h));
        Sz=BoxObjects[b].frames_counter;
        if(Sz==0.0) continue;
        //----------------------------------------------
        //check the size if large enough
        //----------------------------------------------
        Pz=Portal.Sz.Aver();
        switch(BoxObjects[b].obj_id){
            case PERSON : Valid=(Sz > Pz*Js.MinPersonSize/100); break;
            case BIKE   : Valid=(Sz > Pz*Js.MinTwoSize/100); break;
            case CAR    : Valid=(Sz > Pz*Js.MinFourSize/100); break;
        }
        //----------------------------------------------
        //search for the object in the list
        //----------------------------------------------
        Found=false;
        for(i=0;i<MapList.size();i++){
            if((MapList[i].obj_id == BoxObjects[b].obj_id) && (MapList[i].track_id == BoxObjects[b].track_id)){ n=i; Found=true; break; }
        }
        if(Found){
            //object already in the scene, so update
            TMapObject *Mo = &MapList[n];
            if(!Mo->Valid && Valid){
                Mo->Tvalid = PicTime;           //first time valid
            }
            Mo->operator=(BoxObjects[b]);
            //set time
            Mo->Tick   = FrameCnt;
            Mo->Tend   = PicTime;
            //set location and size
            Mo->Size   = Sz;
            Mo->Valid  = Valid;
            strcpy(Mo->CarName, Str.c_str());
            if(Mo->Valid){
                Mo->ValidTicks++;
                //bounce the wall
                Mo->Inside_t1 = Mo->Inside;
                Mo->Bounce_t1 = (Mo->Bounce & Trigger);
                CheckBounced(*Mo);
                //speed and direction
                Mo->Loc = Mo->Mov.Feed(Xo, Yo, Sz, Mo->Bounce, CamFPS);
                //portal
                Portal.Predict(Mo->Mov,Mo->Inside);
                //license
                UpdateAlbum(*Mo);
                PLOG_DEBUG_IF((Trigger & TRIG_PORTAL) && !Mo->Inside && Mo->Inside_t1) << " Track ID : "<< Mo->track_id << " - inside : " << Mo->Inside << " - Valid ticks : " <<  MapList[i].ValidTicks;
            }
        }
        else{
            //(almost) sure it is a new object
            TMapObject Mo;
            Mo = BoxObjects[b];
            //set time
            Mo.Tick         = FrameCnt;
            Mo.Tstart       = PicTime;
            Mo.Tend         = PicTime;
            Mo.Tvalid       = PicTime;
            //set location and size
            Mo.Bounce       = 0;
            Mo.Bounce_t1    = 0;
            Mo.Inside_t1    = true;
            Mo.Loc          = Mo.Mov.Reset(Xo,Yo);
            Mo.Size         = Sz;
            Mo.Valid        = Valid;
            Mo.ValidTicks   = 0;
            Mo.MQTTdone     = false;
            Mo.InspectState = 0;
            strcpy(Mo.CarName, Str.c_str());
            strcpy(Mo.CamName, this->CamName.c_str());
            //license
            UpdateAlbum(Mo);
            MapList.push_back(Mo);
        }
    }
    //----------------------------------------------
    //check if object is leaving us
    //----------------------------------------------
    for(size_t m=0; m<MapList.size(); m++){                             //don't use for(auto m:MapList) because it make copies of MapList[m] which can't be writen
        if(MapList[m].Valid && !MapList[m].MQTTdone){
            double ValidSec = MapList[m].ValidTicks * GetTbase();

            if((MapList[m].obj_id==CAR  && ValidSec>=MinFourSec) ||
               (MapList[m].obj_id==BIKE && ValidSec>=MinTwoSec)){
                if(Trigger & TRIG_PORTAL){
                    if(MapList[m].Inside_t1 && !MapList[m].Inside){     //t-1 = inside | t = outside
                        //get license
                        PlateAlbum.Execute(MapList[m]);
                        //get color
                        FillColor(MapList[m]);
                        //make event string
                        FillMQTTevent(MapList[m]);
                    }
                }
                else{
                    if((MapList[m].Bounce_t1 == 0) && (MapList[m].Bounce & Trigger)){
                        //get license
                        PlateAlbum.Execute(MapList[m]);
                        //get color
                        FillColor(MapList[m]);
                        //make event string
                        FillMQTTevent(MapList[m]);
                    }
                }
            }
        }
    }

    CleanList();    //also repair

    CheckInspection();

    if(Gflag_Bird){
        PlotBirdView();
    }
}
//----------------------------------------------------------------------------------------
// MapStatic.
//----------------------------------------------------------------------------------------
void TMapper::MapStatic(void)
{
    size_t b;
    int Xo, Yo, Sz;
    bool Valid=false;
    std::string Str;
    char ChrCar;

    MQTTevent="";           //clear MQTT event string
    ChrCar='a';
    ChrCar--;               //due to ChrCar++ first before using

    //----------------------------------------------
    //place some safeguards and find the biggest car
    //----------------------------------------------
    for(b=0;b<BoxObjects.size();b++){
        if((int)BoxObjects[b].x <         0) BoxObjects[b].x = 0;
        if((int)BoxObjects[b].w >  CamWidth) BoxObjects[b].w = CamWidth;
        if((int)BoxObjects[b].y <         0) BoxObjects[b].y = 0;
        if((int)BoxObjects[b].h > CamHeight) BoxObjects[b].h = CamHeight;
        if((int)(BoxObjects[b].x+BoxObjects[b].w) >  CamWidth) BoxObjects[b].x=CamWidth -BoxObjects[b].w;
        if((int)(BoxObjects[b].y+BoxObjects[b].h) > CamHeight) BoxObjects[b].y=CamHeight-BoxObjects[b].h;
        Sz=BoxObjects[b].w*BoxObjects[b].h;
        //misuse the unused frames_counter
        BoxObjects[b].frames_counter = Sz;
        if(Sz==0.0) continue;
    }
    //----------------------------------------------
    //reset list (static object, one time use only)
    //----------------------------------------------
    PlateAlbum.FreeAll();
    MapList.clear();
    //----------------------------------------------
    //loop through the list
    //----------------------------------------------
    for(b=0;b<BoxObjects.size();b++){
        //get some info from the object
        Xo=BoxObjects[b].x+(BoxObjects[b].w/2);
        Yo=DoWarp(BoxObjects[b].y+(BoxObjects[b].h));
        Sz=BoxObjects[b].frames_counter;
        if(Sz==0.0) continue;
        //----------------------------------------------
        //check the size if large enough
        //----------------------------------------------
        Valid=(Sz > MinSizePic);
        //----------------------------------------------
        //create a new object
        //----------------------------------------------
        TMapObject Mo;
        Mo = BoxObjects[b];
        //set time
        Mo.Tick         = FrameCnt;
        Mo.Tstart       = PicTime;
        Mo.Tend         = PicTime;
        Mo.Tvalid       = PicTime;
        //set location and size
        Mo.Bounce       = 0;
        Mo.Bounce_t1    = 0;
        Mo.Inside_t1    = true;
        Mo.Loc          = Mo.Mov.Reset(Xo,Yo);
        Mo.Loc.Velocity = 2.5;          //give it some speed (it will be checked later on in CheckLicense)
        Mo.Size         = Sz;
        Mo.Valid        = Valid;
        Mo.ValidTicks   = 100;          //give it some ticks
        Mo.MQTTdone     = false;
        Mo.InspectState = 0;
        sprintf(Mo.PlateOCR,"%s","");   //reset string Mo.PlateOCR
        strcpy(Mo.CarName, Str.c_str());
        if(Mo.obj_id!=PERSON && Mo.Valid){
            //update album
            UpdateAlbum(Mo);
            //get license
            PlateAlbum.Execute(Mo);
            //get color
            FillColor(Mo);
            //make event string
            FillMQTTevent(Mo);
        }
        //keep it alive for plotting
        MapList.push_back(Mo);
    }
}
//----------------------------------------------------------------------------------------
// Some local subroutines
//----------------------------------------------------------------------------------------
bool TMapper::CheckBoI(TMapObject& Mo)
{
    int x=Mo.x+Mo.w/2;
    int y=Mo.y+Mo.h;

    return ((x>BoIRect.x) &&
            (y>BoIRect.y) &&
            (x<(BoIRect.x+BoIRect.width)) &&
            (y<(BoIRect.y+BoIRect.height)) &&
            (Mo.Loc.Velocity<BoISpeed));
}
//----------------------------------------------------------------------------------------
void TMapper::CheckBounced(TMapObject& Mo)
{
    Mo.Bounce = 0;

    if(Mo.Loc.Angle!=0.0){
        if(Mo.Loc.Angle>=0){
            if(Mo.Loc.Angle<1.5708){
                if((int)(Mo.x+Mo.w)>=(DnnRect.x+DnnRect.width-WALL_MARGE)) Mo.Bounce |= TRIG_RIGHT;
                if((int)(Mo.y+Mo.h)>=(DnnRect.y+DnnRect.height-WALL_MARGE)) Mo.Bounce |= TRIG_BOTTOM;
            }
            else{
                if((int) Mo.x <= (DnnRect.x+WALL_MARGE)) Mo.Bounce |= TRIG_LEFT;
                if((int)(Mo.y+Mo.h)>=(DnnRect.y+DnnRect.height-WALL_MARGE)) Mo.Bounce |= TRIG_BOTTOM;
            }
        }
        else{
            if(Mo.Loc.Angle<-1.5708){
                if((int) Mo.x <= (DnnRect.x+WALL_MARGE)) Mo.Bounce |= TRIG_LEFT;
                if((int) Mo.y <= (DnnRect.y+WALL_MARGE)) Mo.Bounce |= TRIG_TOP;
            }
            else{
                if((int) Mo.y <= (DnnRect.y+WALL_MARGE)) Mo.Bounce |= TRIG_TOP;
                if((int)(Mo.x+Mo.w)>=(DnnRect.x+DnnRect.width-WALL_MARGE)) Mo.Bounce |= TRIG_RIGHT;
            }
        }
    }
}
//----------------------------------------------------------------------------------------
void TMapper::UpdateAlbum(TMapObject& Mo)
{
    if(Mo.obj_id==PERSON)  return;      //not a person
    if(!Mo.Valid)          return;      //must be Valid (no background objects)

    switch(CamSource){
        case USE_NONE    : { break; }
        case USE_PICTURE : { break; }
        case USE_FOLDER  : { break; }
        case USE_VIDEO   : { if(!CheckBoI(Mo)) return; }   //must be inside BoI
        case USE_CAMERA  : { if(!CheckBoI(Mo)) return; }   //must be inside BoI
    }
    Mo.InsideBoI = true;
    //Create the ROI
    cv::Mat frame_car = Pic(cv::Rect(Mo.x, Mo.y, Mo.w, Mo.h));
    PlateAlbum.Update(Mo, frame_car);
}
//----------------------------------------------------------------------------------------
void TMapper::FillColor(TMapObject& Mo)
{
    //    HSL
    //red     = 0   - 255 - 127
    //yellow  = 43  - 255 - 127
    //green   = 85  - 255 - 127
    //cyan    = 128 - 255 - 127
    //blue    = 171 - 255 - 127
    //magenta = 213 - 255 - 127
    //back    = 0   - 0   - 0
    //grey    = 0   - 0   - 127
    //white   = 0   - 0   - 255

//      0       1         2        3       4        5         6        7        8       9
//    "Red", "Yellow", "Green", "Cyan", "Blue", "Magenta", "Black", "Gray", "White", " ?? "

#define CLUSTERS 20     //not too less, 12-20 is ok

    if(Mo.obj_id!=CAR) return;

    int i,x,y,c,t,St,Lt,Tt;
    Point3U p;
    int Ar[10];
    Kmain Km(CLUSTERS);

    for(i=0;i<10;i++) Ar[i]=0;

    //Create the ROI
    cv::Mat frame_car = Pic(cv::Rect(Mo.x, Mo.y, Mo.w, Mo.h));

    //get the color of pixels on a 10x10 grid.
    for(y=frame_car.rows/2;y<frame_car.rows;y+=10){
        for(x=0;x<frame_car.cols;x+=10){
            BGR& bgr = frame_car.ptr<BGR>(y)[x];
            p=bgr; Km.AddPoint(p);
        }
    }
    if(Km.GetNumberOfPoints()==0) return;

    Km.Run();

    //collect the colors and get average
    St=Lt=Tt=0;
    for(i=0;i<CLUSTERS;i++){
        HSL hsl=RGB_HSL(Km.centroids[i]);
        c = ((hsl.h+21)&0xFF)/43;
        t = Km.centroids[i].cluster;
        Ar[c]+=t;
        St+=(hsl.s*t);
        Lt+=(hsl.l*t);
        Tt+=t;
    }
    St/=Tt; Lt/=Tt;

    //get the most populated cluster
    c=-1; x=0;
    for(i=0;i<6;i++){
        if(Ar[i]>c){ c=Ar[i]; x=i; }
    }
    //x holds most dominant color.
    Mo.ColIndex=x;

    PLOG_DEBUG << "ID "<< Mo.track_id <<" Color H_indexed: " << x << " (" << color_names[x] << ") S: " << St << " L:" << Lt ;

    //if no saturation, no color -> black to white
    if(St<40){
        if(Lt<85)      Mo.ColIndex=6;       //black (<85)
        else{
            if(Lt<131) Mo.ColIndex=7;       //gray  (85..130)
            else       Mo.ColIndex=8;       //white (>=130)
        }
    }

    //get time stamp
    Mo.Tcolor = PicTime;
}
//----------------------------------------------------------------------------------------
float TMapper::DoWarp(float Y)
{
    int   Ht  = DnnRect.height;
    float Mx2 = Ht*Gain;
    float D1  = pow(Ht,Warp);
    float E1  = D1/Ht;
    float In  = LIM(Y, 0, Ht);

    float Out = (D1-pow((Mx2-In),Warp))/E1;

    if(Out<0)   Out=0;
    if(Out>=Ht) Out=Ht-1;

    return Out;
}
//----------------------------------------------------------------------------------------
float TMapper::InvWarp(float X)
{
    int   Ht  = DnnRect.height;
    float Mx2 = Ht*Gain;
    float D1  = pow(Ht,Warp);
    float E1  = D1/Ht;
    float In  = LIM(X, 0, Ht);
    float Ze  = D1-In*E1;
    float Out;

    if(Ze>0.0) Out = LIM( Mx2-pow((D1-In*E1),(1.0/Warp)), 0, Ht-1);
    else       Out = Ht-1;

    return Out;
}
//----------------------------------------------------------------------------------------
void TMapper::CleanList(void)
{
    size_t n,i,p;
    int Telapse;
    float Dz, Dd;
    bool Found;

    //clean the list (objects not updated for the last 2 seconds)
    for(i=0;i<MapList.size();i++){
        if(MapList[i].Tick != FrameCnt){
            if(MapList[i].obj_id == CAR){
                if((MapList[i].ValidTicks*GetTbase()) > MinFourSec){
                    //try to fix mistakes.
                    Found=false;
                    for(p=0;p<MapList.size();p++){
                        //not the same, identical objects and p must be alive (Tick==FrameCnt)
                        if(p!=i && MapList[i].obj_id==MapList[p].obj_id && MapList[p].Tick==FrameCnt){
                            Dz=((float)MapList[i].Size)/MapList[p].Size;
                            Dd=EUCLIDEAN(MapList[i].Loc.X, MapList[i].Loc.Y, MapList[p].Loc.X, MapList[p].Loc.Y);
                            //80% < size < 120%
                            //max 120 distance
                            if(Dz>0.8 && Dz<1.2 && Dd<120.0){ n=p; Found=true; break; }
                        }
                    }
                    if(Found){
                        //i is replaced by n
                        //re-activate i by merging n with the contents of i
                        MapList[i]<<MapList[n];
                        //remove n
                        PlateAlbum.Remove(MapList[n]);
                        MapList.erase(MapList.begin() + n);
                    }
                    else{
                        //remove the objects after 2 Sec (now no repair is possible)
                        Telapse = PicTime-MapList[i].Tend;      // mSec
                        if((Telapse>2000)||(Telapse<0)) {       // Telapse < 0 means the video rewinds
                            if(MapList[i].Loc.Velocity!=0.0 && MapList[i].Mov.X>0 && MapList[i].Mov.Y>0){
                                //object is crossed the border and will not re-enter the list.
                                Portal.Update(MapList[i].Mov);
                            }
                            //in case not sent
                            FillMQTTevent(MapList[i]);
                            //remove the object from the list
                            PLOG_VERBOSE << " Exit CAR : " << MapList[i].track_id << " - Valid ticks : " << MapList[i].ValidTicks;
                            PlateAlbum.Remove(MapList[i]);
                            MapList.erase(MapList.begin() + i);
                            i--;
                        }
                    }
                }
                else{
                    //remove the non-valid objects straight away when not detected any more
                    PLOG_VERBOSE << " Exit CAR : " << MapList[i].track_id << " - Valid ticks : " << MapList[i].ValidTicks;
                    PlateAlbum.Remove(MapList[i]);
                    MapList.erase(MapList.begin() + i);
                    i--;
                }
            }
            else{
                if(MapList[i].obj_id == BIKE){
                    if((MapList[i].ValidTicks*GetTbase()) > MinTwoSec){
                        //try to fix mistakes.
                        Found=false;
                        for(p=0;p<MapList.size();p++){
                            //not the same, identical objects and p must be alive (Tick==FrameCnt)
                            if(p!=i && MapList[i].obj_id==MapList[p].obj_id && MapList[p].Tick==FrameCnt){
                                Dz=((float)MapList[i].Size)/MapList[p].Size;
                                Dd=EUCLIDEAN(MapList[i].Loc.X, MapList[i].Loc.Y, MapList[p].Loc.X, MapList[p].Loc.Y);
                                //80% < size < 120%
                                //max 120 distance
                                if(Dz>0.8 && Dz<1.2 && Dd<120.0){ n=p; Found=true; break; }
                            }
                        }
                        if(Found){
                            //i is replaced by n
                            //re-activate i by merging n with the contents of i
                            MapList[i]<<MapList[n];
                            //remove n
                            PlateAlbum.Remove(MapList[n]);
                            MapList.erase(MapList.begin() + n);
                        }
                        else{
                            //remove the objects after 2 Sec (now no repair is possible)
                            Telapse = PicTime-MapList[i].Tend; //mSec
                            if(Telapse>2000){
                                //remove the object from the list
                                PLOG_VERBOSE << " Exit BIKE : " << MapList[i].track_id << " - Valid ticks : " << MapList[i].ValidTicks;
                                //in case not sent
                                FillMQTTevent(MapList[i]);
                                PlateAlbum.Remove(MapList[i]);
                                MapList.erase(MapList.begin() + i);
                                i--;
                            }
                        }
                    }
                    else{
                        //remove the non-valid objects straight away when not detected any more
                        PLOG_VERBOSE << " Exit BIKE : " << MapList[i].track_id << " - Valid ticks : " << MapList[i].ValidTicks;
                        PlateAlbum.Remove(MapList[i]);
                        MapList.erase(MapList.begin() + i);
                        i--;
                    }
                }
                else{
                    //remove the non-valid objects straight away when not detected any more
                    PLOG_VERBOSE << " Exit object : " << MapList[i].track_id ;
                    PlateAlbum.Remove(MapList[i]);
                    MapList.erase(MapList.begin() + i);
                    i--;
                }
            }
        }
    }
}
//----------------------------------------------------------------------------------------
float TMapper::GetBoxDistance(TMapObject& Mo,const cv::Point& Pt)
{
    float Dist;

    int x1=Mo.Loc.X-Mo.w/2;
    int x2=Mo.Loc.X+Mo.w/2;
    int y1=Mo.Loc.Y-Mo.h;
    int y2=Mo.Loc.Y;
/*
           1           2            3

              y1┌──────────────┐
                │              │ 
           4    │      5       │     6
                │              │ 
              y2└──────────────┘
                x1            x2
           7           8            9

    In case of vehicle inspection, position 1, 2 and 3 are invalid
    A person is located above a vehicle. In real life, it means at the background

    └──
*/
    if(Pt.x<x1){                //1,4,7
        if(Pt.y<y1) Dist=FLT_MAX; //EUCLIDEAN(x1,y1,Pt.x,Pt.y);        //1
        else{
            if(Pt.y>y2) Dist=EUCLIDEAN(x1,y2,Pt.x,Pt.y);    //7
            else Dist=x1-Pt.x;                              //4
        }
    }
    else{
        if(Pt.x>x2){            //3,6,9
            if(Pt.y<y1) Dist=FLT_MAX; //EUCLIDEAN(x2,y1,Pt.x,Pt.y);    //3
            else{
                if(Pt.y>y2) Dist=EUCLIDEAN(x2,y2,Pt.x,Pt.y);//9
                else Dist=Pt.x-x2;                          //6
            }
        }
        else{                   //2,5,8
            if(Pt.y<y1) Dist=FLT_MAX; //y1-Pt.y;                       //2
            else{
                if(Pt.y>y2) Dist=Pt.y-y2;                   //8
                else  Dist=0.0;                             //5
            }
        }
    }
    return Dist;
}
//----------------------------------------------------------------------------------------
void TMapper::SetInpectionState(TMapObject& Mo)
{
    size_t p;
    float Dist;
    int Telapse;

    switch(Mo.InspectState){
        //wait for stationary car
        case 0 : {  if(Mo.Loc.Velocity<Js.InspectSpeed){
                        Mo.InspectState = 1;
                        PLOG_DEBUG << " Inspection phase 0 -> car : " << Mo.track_id << " -> Start stationary at : " << Mo.Loc.Velocity;
                    }
                 } break;
        //wait for a car to drive away
        case 1 : {  //is there a person nearby, while the car is statioary?
                    if(Mo.Loc.Velocity<Js.InspectSpeed){
                        if(Mo.TFinp==0){
                            for(p=0; p<MapList.size(); p++){
                                //see if there is a person with track_id and Used in the scene
                                if(MapList[p].obj_id==PERSON &&       //must be a person
                                   MapList[p].track_id>0 &&           //must have a track_id
                                   MapList[p].Valid){                 //must be valid (no persons at the background)
                                    //get the distance between vehicle and person
                                    Dist=GetBoxDistance(Mo, cv::Point(MapList[p].Loc.X, MapList[p].Loc.Y));
                                    //are they close enough?
                                    if(Dist < Js.InspectDistance){
                                        Mo.TFinp = PicTime;
                                        PLOG_DEBUG << " Inspection -> person nearby " << MapList[p].track_id;
                                    }
                                }
                            }
                        }
                    }
                    else{
                        Mo.TLinp = PicTime;
                        //we have a start and end time, lets check
                        Telapse = Mo.TLinp-Mo.TFinp; //mSec
                        PLOG_DEBUG << " Inspection phase 2 -> car : " << Mo.track_id << " -> drives away after : " << Telapse << " mSec";
                        if(Telapse > (Js.InspectTime*1000) && Mo.TFinp>0){
                            PLOG_DEBUG << " Set inspected car : " << Mo.track_id;
                            Mo.Inspected=true;
                            Mo.InspectState = 2;
                        }
                        else{
                            PLOG_DEBUG << " Too short a time inspected car : " << Mo.track_id;
                            //start all over again
                            Mo.InspectState = 0;
                        }
                    }
                 } break;
        //do nothing, the car is inspected
        case 2 : { ; } break;
    }
}
//----------------------------------------------------------------------------------------
void TMapper::CheckInspection(void)
{
    //check inspection of vehicles
    for(size_t v=0; v<MapList.size();v++){      //don't use for(auto v:MapList) because it make copies of MapList[v] which can't be writen
        //see if there is a vehicle with track_id and Used in the scene
        if (MapList[v].obj_id==CAR &&           //must be a four-wheeler
            MapList[v].track_id>0 &&            //must have a track_id
            MapList[v].Loc.Velocity>0.05 &&     //must have a life. (at init the velocity is 0)
            MapList[v].Valid &&                 //must be valid (no four-wheelers at the background)
            MapList[v].ValidTicks>1){           //must be lived a while
            SetInpectionState(MapList[v]);
        }
    }
}
//----------------------------------------------------------------------------------------
void TMapper::FillMQTTevent(TMapObject& Mo)
{
    std::string Tos, Toc, Toi;
    std::string Doi, Too, Dov;
    std::ostringstream oj;

    if(Mo.MQTTdone) return;     //already done
    if(!Mo.InsideBoI) return;

    //safeguard for the ColIndex names array
    if(Mo.ColIndex>9) Mo.ColIndex=9;

//tos = Time at which we started tracking the car - (mm:ss)
//toi = Time a sentry start inspecting the car - (mm:ss)
//doi = Duration of inspection of car by sentry - (in Seconds)
//too = Time at which OCR was done (Best license image).Because we are track the color at side of the car - (mm:ss)
//toc = Time-of-color or time at which color was finalized - (mm:ss)
//dov = Duration of Valid vehicle

    Tos=StreamTime.NiceTime_Sec(Mo.Tstart, Mo.Tvalid);
    Toc=StreamTime.NiceTime_Sec(Mo.Tstart, Mo.Tcolor);
    if(Mo.Inspected) Toi=StreamTime.NiceTime_Sec(Mo.Tstart, Mo.TFinp);
    else             Toi="";
    Doi=StreamTime.NiceTime_mSec(Mo.TFinp, Mo.TLinp);
    Too=StreamTime.NiceTime_Sec(Mo.Tstart, Mo.Tplate);
    Dov=StreamTime.NiceTime_mSec(Mo.ValidTicks*GetTbase());

    oj << "{\n\t\"version\": \"" << Js.Version << "\",";
    oj << "\n\t\"camId\": \"" << CamName << "\",";
    oj << "\n\t\"frameId\": \"" << FileName << "\",";
    //vehicles
    oj << "\n\t\"vehicleDetails\": [";
    oj << "\n\t\t{";
    oj << "\n\t\t\t\"vehicleTrack\": " << Mo.track_id << ",";
    if(Mo.obj_id==CAR) oj << "\n\t\t\t\"vehicleType\": \"Four_wheeler\",";
    else               oj << "\n\t\t\t\"vehicleType\": \"Two_wheeler\",";
    oj << "\n\t\t\t\"firstSeen\": \""+StreamTime.NiceTimeUTC(Mo.Tstart)+"\",";
    oj << "\n\t\t\t\"lastSeen\": \""+StreamTime.NiceTimeUTC(Mo.Tend)+"\",";
    oj << "\n\t\t\t\"licensePlateText\": \"" << Mo.PlateOCR << "\",";
    oj << "\n\t\t\t\"vehicleColor\": \"" << color_names[Mo.ColIndex] << "\",";
    if(Mo.Inspected) oj << "\n\t\t\t\"isInspected\": \"T\",";
    else             oj << "\n\t\t\t\"isInspected\": \"F\",";
    oj << "\n\t\t\t\"toj\": \""<< Tos <<"\",";
    oj << "\n\t\t\t\"toi\": \""<< Toi <<"\",";
    oj << "\n\t\t\t\"doi\": \""<< Doi <<"\",";
    oj << "\n\t\t\t\"too\": \""<< Too <<"\",";
    oj << "\n\t\t\t\"toc\": \""<< Toc <<"\",";
    oj << "\n\t\t\t\"dov\": \""<< Dov <<"\"";
    oj << "\n\t\t}";
    oj << "\n\t]" << "\n}";

    MQTTevent = oj.str();

    if(Js.MQTT_On){
        publishMQTTMessage(TopicName,MQTTevent);        //send mqtt into the world
    }
    //send json into the world (port 8070)
    if(Js.JSON_Port>0){
        SendJsonHTTP(MQTTevent, Js.JSON_Port);           //send json to port 8070
    }

    Mo.MQTTdone = true;
}
//----------------------------------------------------------------------------------------
bool TMapper::SendJsonHTTP(std::string send_json, int port, int timeout)
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
void TMapper::DrawObjectsLabels(void)
{
    char text[256];
    int x,y,baseLine;

    //DNN_rect
    cv::rectangle(Pic, cv::Point(DnnRect.x, DnnRect.y),
                       cv::Point(DnnRect.x+DnnRect.width, DnnRect.y+DnnRect.height), cv::Scalar(255,255,0),3);

    //the objects
    for(const auto i:BoxObjects) {
        sprintf(text, " %s %i ", class_names[i.obj_id],i.track_id);

        baseLine = 0;
        cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

        x = i.x;
        y = i.y - label_size.height - baseLine;
        if (y < 0) y = 0;
        if (x + label_size.width > Pic.cols) x = Pic.cols - label_size.width;

        cv::rectangle(Pic, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

        cv::putText(Pic, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);

        cv::rectangle(Pic, cv::Point(i.x, i.y), cv::Point(i.x+i.w, i.y+i.h), cv::Scalar(255, 255, 25), 2);
    }
}
//---------------------------------------------------------------------------
void TMapper::DrawObjects(void)
{
    char text[256];
    int p,x,y,baseLine;
    TParaLine Or,L1;
    DPoint P1,P2;
    cv::Point Pt;

    if(Js.PrintOnCli){
        cout << CamName << endl;
        for(const auto i:BoxObjects){
            cout << i.prob <<" - "<< class_names[i.obj_id] << " - "<< i.track_id << endl;
        }
    }

    if(Js.PrintOnRender || Js.MJPEG_Port>7999){
        //DNN_rect
        cv::rectangle(Pic, cv::Point(DnnRect.x, DnnRect.y),
                           cv::Point(DnnRect.x+DnnRect.width, DnnRect.y+DnnRect.height-1), cv::Scalar(255,255,0),3);
        //trigger wall
        if(Trigger & TRIG_LEFT){
            cv::line(Pic, cv::Point(DnnRect.x, DnnRect.y), cv::Point(DnnRect.x, DnnRect.y+DnnRect.height-1), cv::Scalar(0, 255,128),3);
        }
        if(Trigger & TRIG_TOP){
            cv::line(Pic, cv::Point(DnnRect.x, DnnRect.y), cv::Point(DnnRect.x+DnnRect.width, DnnRect.y), cv::Scalar(0, 255,128),3);
        }
        if(Trigger & TRIG_RIGHT){
            cv::line(Pic, cv::Point(DnnRect.x+DnnRect.width, DnnRect.y), cv::Point(DnnRect.x+DnnRect.width, DnnRect.y+DnnRect.height-1), cv::Scalar(0, 255,128),3);
        }
        if(Trigger & TRIG_BOTTOM){
            cv::line(Pic, cv::Point(DnnRect.x, DnnRect.y+DnnRect.height-1), cv::Point(DnnRect.x+DnnRect.width, DnnRect.y+DnnRect.height-1), cv::Scalar(0, 255,128),3);
        }
        if(Trigger & TRIG_PORTAL){
            //Portal (curved line due to warp)
            vector<cv::Point> Hull;
            baseLine=Portal.Length/10;          //for loop runs from -Length to + Length -> 20 points
            for(p=-Portal.Length; p<=Portal.Length; p+=baseLine){
                Portal.Border.Get(p,x,y);
                Pt.x=x; Pt.y=InvWarp(y);
                if(Pt.y<Pic.rows){
                    Hull.push_back(Pt);
                }
            }
            cv::polylines(Pic, Hull, false, cv::Scalar(0, 255, 128),3);
            Portal.Border.Get(0,x,y);
            Pt.x=x; Pt.y=InvWarp(y);
            if(Pt.y<Pic.rows){
                cv::circle(Pic,Pt,6,cv::Scalar(0, 255, 128),CV_FILLED);
            }
        }
        //draw BoI
        switch(CamSource){
            case USE_NONE    : { break; }
            case USE_PICTURE : { break; }
            case USE_FOLDER  : { break; }
            case USE_VIDEO   : { cv::rectangle(Pic, cv::Point(BoIRect.x, BoIRect.y),
                                                    cv::Point(BoIRect.x+BoIRect.width, BoIRect.y+BoIRect.height-1), cv::Scalar(76,177,34),7); break; }
            case USE_CAMERA  : { cv::rectangle(Pic, cv::Point(BoIRect.x, BoIRect.y),
                                                    cv::Point(BoIRect.x+BoIRect.width, BoIRect.y+BoIRect.height-1), cv::Scalar(76,177,34),7); break; }
        }
        //the objects
        for(auto i:MapList){
            //see if it is a known object
            if(i.Tend == PicTime){
                if(i.Valid){
                    //get a color
                    cv::Scalar mc, tc;
                    switch(i.obj_id){
                        case PERSON: { mc=cv::Scalar(255, 0,   0); tc=cv::Scalar(255, 255, 255); break;}  //blue - person
                        case BIKE:   { mc=cv::Scalar(255, 0, 255); tc=cv::Scalar(  0,   0,   0); break;}  //magenta - motorcycle
                        case CAR:    { mc=cv::Scalar(255, 255, 0); tc=cv::Scalar(  0,   0,   0); break;}  //cyan - car
                    }

                    sprintf(text, " %s %i %.2f", class_names[i.obj_id],i.track_id, i.prob);
//                    sprintf(text, " %i ", i.track_id);
    //                    sprintf(text, " %s ", class_names[i.obj_id]);

                    baseLine = 0;
                    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, &baseLine);

                    x = i.x;
                    y = i.y - label_size.height - baseLine;
                    if (y < 0) y = 0;
                    if (x + label_size.width > Pic.cols) x = Pic.cols - label_size.width;

                    cv::rectangle(Pic, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),mc, -1);
                    cv::putText(Pic, text, cv::Point(x, y + label_size.height+2), cv::FONT_HERSHEY_SIMPLEX, 0.8, tc, 2);

                    cv::rectangle(Pic, cv::Point(i.x, i.y), cv::Point(i.x+i.w, i.y+i.h), mc, 2);

                    if(i.obj_id!=PERSON){
                        //show anchor
                        x=i.x+i.w/2; y=i.y+i.h;
                        if(CheckBoI(i)){
                            if(Trigger & TRIG_PORTAL){
                                if(i.Inside) cv::circle(Pic,cv::Point(x,y),11,mc,CV_FILLED);
                                else         cv::circle(Pic,cv::Point(x,y),11,cv::Scalar(255, 255, 255),CV_FILLED);
                            }
                            else{
                                if(i.Bounce_t1) cv::circle(Pic,cv::Point(x,y),11,cv::Scalar(255, 255, 255),CV_FILLED);
                                else            cv::circle(Pic,cv::Point(x,y),11,mc,CV_FILLED);
                            }
                        }
                        else{
                            if(Trigger & TRIG_PORTAL){
                                if(i.Inside) cv::circle(Pic,cv::Point(x,y),6,mc,CV_FILLED);
                                else         cv::circle(Pic,cv::Point(x,y),6,cv::Scalar(255, 255, 255),CV_FILLED);
                            }
                            else{
                                if(i.Bounce_t1) cv::circle(Pic,cv::Point(x,y),6,cv::Scalar(255, 255, 255),CV_FILLED);
                                else            cv::circle(Pic,cv::Point(x,y),6,mc,CV_FILLED);
                            }
                        }
                    }
                }
                else{
                    cv::rectangle(Pic, cv::Point(i.x, i.y), cv::Point(i.x+i.w, i.y+i.h), cv::Scalar(255,0,0));
                }
            }
        }
        if(Js.PrintPlate){
            PlateAlbum.Show(Pic);
        }
    }
}
//----------------------------------------------------------------------------------------
void TMapper::PlotBirdView(void)
{
    bool in;
    char text[256];
    int x,y,x1,y1,x2,y2,baseLine;
    cv::Scalar Color;
    cv::Mat m2;
    cv::Mat m(DnnRect.height, DnnRect.width, CV_8UC3, cv::Scalar(48, 10, 36));

    //draw BoI
    switch(CamSource){
        case USE_NONE    : { break; }
        case USE_PICTURE : { break; }
        case USE_FOLDER  : { break; }
        case USE_VIDEO   : { y1=DoWarp(BoIRect.y); y2=DoWarp(BoIRect.y+BoIRect.height-1);
                             cv::rectangle(m, cv::Point(BoIRect.x, y1), cv::Point(BoIRect.x+BoIRect.width, y2), cv::Scalar(76,177,34),3); break; }
        case USE_CAMERA  : { y1=DoWarp(BoIRect.y); y2=DoWarp(BoIRect.y+BoIRect.height-1);
                             cv::rectangle(m, cv::Point(BoIRect.x, y1), cv::Point(BoIRect.x+BoIRect.width, y2), cv::Scalar(76,177,34),3); break; }
    }

    for(auto i:MapList){
        //the prediction
        switch(i.obj_id){
            case PERSON : Color=cv::Scalar(0,   255, 255); break;
            case CAR    : Color=cv::Scalar(255,   0, 255); break;
            case BIKE   : Color=cv::Scalar(255, 117, 140); break;
        }
        i.Mov.Predict(x,y);
        if(x>0 && y>0){
            if(i.obj_id==CAR || i.obj_id==BIKE){
                if(i.Valid) {
                    if(CheckBoI(i)){
                        if(i.Inside) cv::circle(m,cv::Point(x,y),12,Color,CV_FILLED);
                        else         cv::circle(m,cv::Point(x,y),12,cv::Scalar(255, 255, 255),CV_FILLED);
                    }
                    else
                    {
                        if(i.Inside) cv::circle(m,cv::Point(x,y),6,Color,CV_FILLED);
                        else         cv::circle(m,cv::Point(x,y),6,cv::Scalar(255, 255, 255),CV_FILLED);
                    }
                    //box
                    x1=x-i.w/2; x2=x+i.w/2; y1=y; y2=y-i.h;
                    cv::rectangle(m, cv::Point(x1,y1),cv::Point(x2,y2), cv::Scalar(188,188,188),2);
                    //legs
                    cv::line(m,cv::Point(i.Mov.X0,i.Mov.Y0),cv::Point(i.Mov.X1,i.Mov.Y1),cv::Scalar(255, 188, 188),4);
                    //crossing the border
                    DPoint Pt=Portal.Predict(i.Mov,in);
                    cv::circle(m,cv::Point(Pt.X,Pt.Y),12,cv::Scalar(255, 255, 0),CV_FILLED);
                    //movement direction
                    i.Mov.Predict(x1,y1,8);
                    i.Mov.Predict(x2,y2,30);
                    cv::line(m,cv::Point(x1,y1),cv::Point(x2,y2),cv::Scalar(0, 255, 0),2);
                    //info
                    baseLine = 0; sprintf(text, " %i", i.track_id);
                    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 1.0, 2, &baseLine);

                    y = y - label_size.height - baseLine;
                    if (y < 0) y = 0;
                    if (x + label_size.width > m.cols) x = m.cols - label_size.width;

                    cv::putText(m, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);

                    x += label_size.width;

                    baseLine = 0; sprintf(text, " %i", i.Loc.Sz);
                    label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.7, 2, &baseLine);

                    y = y + label_size.height - baseLine;
                    if (y < 0) y = 0;
                    if (x + label_size.width > m.cols) x = m.cols - label_size.width;

                    cv::putText(m, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

                    baseLine = 0; sprintf(text, " %.3f", i.Loc.Velocity);
                    label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.7, 2, &baseLine);

                    y = y - label_size.height - baseLine;
                    if (y < 0) y = 0;
                    if (x + label_size.width > m.cols) x = m.cols - label_size.width;

                    cv::putText(m, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);

                    baseLine = 0; sprintf(text, " %.2f", 180*i.Loc.Angle/M_PI);
                    label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.7, 2, &baseLine);

                    y = y + 3.5*label_size.height - baseLine;
                    if (y < 0) y = 0;
                    if (x + label_size.width > m.cols) x = m.cols - label_size.width;

                    cv::putText(m, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
                }
                else{
                    cv::circle(m,cv::Point(x,y),4,Color,2);

                    baseLine = 0; sprintf(text, " %i", i.track_id);
                    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, &baseLine);

                    y = y - label_size.height - baseLine;
                    if (y < 0) y = 0;
                    if (x + label_size.width > m.cols) x = m.cols - label_size.width;

                    cv::putText(m, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
                }
            }
            else{
                if(i.obj_id==PERSON){
                    i.Mov.Predict(x,y);
                    if(i.Valid) {
                        cv::line(m,cv::Point(x-5,y-5),cv::Point(x+5,y+5),cv::Scalar(0, 188, 188),4);
                        cv::line(m,cv::Point(x+5,y-5),cv::Point(x-5,y+5),cv::Scalar(0, 188, 188),4);
                        //legs
                        cv::line(m,cv::Point(i.Mov.X0,i.Mov.Y0),cv::Point(i.Mov.X1,i.Mov.Y1),cv::Scalar(255, 188, 188),4);
                        //movement direction
                        i.Mov.Predict(x1,y1,8);
                        i.Mov.Predict(x2,y2,30);
                        cv::line(m,cv::Point(x1,y1),cv::Point(x2,y2),cv::Scalar(0, 255, 0),2);
                    }
                    else{
                        cv::line(m,cv::Point(x-5,y-5),cv::Point(x+5,y+5),cv::Scalar(0, 188, 188),2);
                        cv::line(m,cv::Point(x+5,y-5),cv::Point(x-5,y+5),cv::Scalar(0, 188, 188),2);
                    }
                    sprintf(text, " %i", i.track_id);
                    baseLine = 0;
                    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, &baseLine);

                    y = y - label_size.height - baseLine;
                    if (y < 0) y = 0;
                    if (x + label_size.width > m.cols) x = m.cols - label_size.width;

                    cv::putText(m, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
                }
            }
        }
    }

    int p=MAX(Portal.Length,150);
    Portal.Border.Get(-p,x1,y1); Portal.Border.Get(p,x2,y2);
    cv::line(m,cv::Point(x1,y1),cv::Point(x2,y2),cv::Scalar(0, 255, 255),2);

    TParaLine Pr=Portal.Border;
    TParaLine Or=Portal.Border;

    Or.Orthogonal();

    Pr.Get(Portal.Length,Or.B.X,Or.B.Y);
    Or.Get(10,x1,y1); Or.Get(-10,x2,y2);
    cv::line(m,cv::Point(x1,y1),cv::Point(x2,y2),cv::Scalar(0, 255, 255),2);

    Pr.Get(-Portal.Length,Or.B.X,Or.B.Y);
    Or.Get(10,x1,y1); Or.Get(-10,x2,y2);
    cv::line(m,cv::Point(x1,y1),cv::Point(x2,y2),cv::Scalar(0, 255, 255),2);

    resize(m, m2, cv::Size(m.cols/2,m.rows/2), 0, 0, cv::INTER_LINEAR);
    cv::imshow(CamName,m2);
}
//----------------------------------------------------------------------------------------
