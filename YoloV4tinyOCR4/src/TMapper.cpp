#include "TMapper.h"
#include "Tjson.h"
#include "General.h"

//----------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------

extern size_t FrameCnt;
extern Detector *PlateNet;
extern Tjson  Js;
extern TOCR Ocr;

extern void publishMQTTMessage(const string& topic, const string payload);
//---------------------------------------------------------------------------
static cv::Mat MorphKernal = cv::getStructuringElement( cv::MORPH_RECT, cv::Size( 11, 11 ), cv::Point( 5, 5 ) );
//----------------------------------------------------------------------------------------
TMapper::TMapper()
{
    //ctor
}
//----------------------------------------------------------------------------------------
TMapper::~TMapper()
{
    //dtor
}
//----------------------------------------------------------------------------------------
void TMapper::Init(const int _ID,const int Width,const int Height,const double Warp,
                   const double Gain,const std::string CamName,const cv::Rect _DnnRect)
{
    Wd=Width;   Ht=Height;
    Wp=Warp;    Gn=Gain;
    MapMat=cv::Mat(Height,Width, CV_8UC3, cv::Scalar(0, 0, 0));
    CamID=CamName; ID=_ID; DnnRect=_DnnRect;

    Portal.Init(to_string(ID));
}
//----------------------------------------------------------------------------------------
void TMapper::Update(cv::Mat& frame, std::vector<bbox_t>& box)
{
    size_t b, i, n;
    char ChrCar;
    bool Found;
    bool Valid=false;
    int Xo,Yo,Sz;
    float Dz, Dd;
    std::string NameStr;
    chrono::steady_clock::time_point Tactual = chrono::steady_clock::now();

    //check if it is new object
    ChrCar='a';
    ChrCar--;       //due to ChrCar++ first before using
    for(b=0;b<box.size();b++){
        //safeguard
        if((int)box[b].x <         0) box[b].x = 0;
        if((int)box[b].w >frame.cols) box[b].w = frame.cols;
        if((int)box[b].y <         0) box[b].y = 0;
        if((int)box[b].h >frame.rows) box[b].h = frame.rows;
        if((int)(box[b].x+box[b].w) > frame.cols) box[b].x=frame.cols-box[b].w;
        if((int)(box[b].y+box[b].h) > frame.rows) box[b].y=frame.rows-box[b].h;

        //get some info from the object
        Xo=box[b].x+(box[b].w/2);
        Yo=Warp(box[b].y+(box[b].h));
        Sz=box[b].w*box[b].h;
        if(Sz==0.0) continue;

        //check the size if large enough
        switch(box[b].obj_id){
            case PERSON    : Valid=(Sz > Js.MinPersonSize); break;
            case MOTORBIKE : Valid=(Sz > Js.MinTwoSize); break;
            case CAR       : Valid=(Sz > Js.MinFourSize); break;
        }

        //see if we must store the picture of the vehicle
        if((box[b].obj_id!=PERSON) && Valid){
            NameStr=""; ChrCar++;
            if(Js.Car_Folder!="none"){
                cv::Mat frame_car = frame(cv::Rect(box[b].x, box[b].y, box[b].w, box[b].h));
                NameStr = Js.Car_Folder+"/"+CamID+"-"+to_string(FrameCnt)+"_"+ChrCar+"_utc.jpg";
                cv::imwrite(NameStr, frame_car);
            }
        }

        //search for the object in the list
        Found=false;
        for(i=0;i<MapList.size();i++){
            if((MapList[i].obj_id == box[b].obj_id) && (MapList[i].track_id == box[b].track_id)){ n=i; Found=true; break; }
        }
        if(Found){
            //object already in the scene, so update
            TMapObject *Mo = &MapList[n];
            Mo->operator=(box[b]);
            //set time
            Mo->Tick   = FrameCnt;
            Mo->Tend   = Tactual;
            //set location and size
            Mo->Loc    = Mo->Predict.Feed(Xo, Yo);
            Mo->Size   = Sz;
            Mo->Valid  = Valid;
            strcpy(Mo->Name, NameStr.c_str());
            Mo->CarChr = ChrCar;
            if(Mo->Valid){
                CheckLicense(frame, *Mo);
                Mo->ValidTicks++;
                Mo->Inside_t1 = Mo->Inside;
                Portal.Predict(Mo->Predict,Mo->ArrivalDist,Mo->ArrivalTime,Mo->Inside);
            }
        }
        else{
            //check if track_id flipped
            for(i=0;i<MapList.size();i++){
                if(MapList[i].obj_id == box[b].obj_id){
                    Dz=((float)MapList[i].Size)/Sz;
                    Dd=EUCLIDEAN(MapList[i].Loc.X, MapList[i].Loc.Y, Xo, Yo);
                    //90% < size < 110%
                    //max 60 distance
                    if(Dz>0.9 && Dz<1.1 && Dd<60.0){ n=i; Found=true; break; }
                }
            }
            if(Found){
                //an object of the same make, size and location
                //modify the track_id also
                TMapObject *Mo = &MapList[n];
                Mo->operator=(box[b]);
                Mo->track_id = box[b].track_id;
                //set time
                Mo->Tick   = FrameCnt;
                Mo->Tend   = Tactual;
                //set location and size
                Mo->Loc    = Mo->Predict.Feed(Xo,Yo);
                Mo->Size   = Sz;
                Mo->Valid  = Valid;
                strcpy(Mo->Name, NameStr.c_str());
                Mo->CarChr = ChrCar;
                if(Mo->Valid){
                    CheckLicense(frame, *Mo);
                    Mo->ValidTicks++;
                    Mo->Inside_t1 = Mo->Inside;
                    Portal.Predict(Mo->Predict,Mo->ArrivalDist,Mo->ArrivalTime,Mo->Inside);
                }
            }
            else{
                //now we are sure it is a new object
                TMapObject Mo;
                Mo        = box[b];
                //set time
                Mo.Tick   = FrameCnt;
                Mo.Tstart = Tactual;
                Mo.Tend   = Tactual;
                //set location and size
                Mo.Loc = Mo.Predict.Reset(Xo,Yo);
                Mo.Size   = Sz;
                Mo.Valid  = Valid;
                Mo.ValidTicks = 0;
                strcpy(Mo.Name, NameStr.c_str());
                Mo.CarChr = ChrCar;
                Mo.PlatePoll.Init(Js.PlateNrChars);
                if(Mo.Valid){
                    CheckLicense(frame, Mo);
                }
                MapList.push_back(Mo);
            }
        }
    }
    CleanList();

    CheckInspection();

    if(Js.ShowBirdView){
        PlotDebug();
    }

//    for(auto &i : box){
//        if(i.obj_id==CAR){
//            int x=(i.x+(i.w/2));
//            int y=Warp((i.y+i.h));
//            cv::line(MapMat,cv::Point(x, y),cv::Point(x+2, y),cv::Scalar(255, 255, 255),4);
//        }
//    }
//    cv::Mat m2;
//    resize(MapMat, m2, cv::Size(MapMat.cols/2,MapMat.rows/2), 0, 0, cv::INTER_LINEAR);
//    cv::imshow("Map",m2);
}
//----------------------------------------------------------------------------------------
void TMapper::CleanList(void)
{
    size_t i;
    int Telapse;
    chrono::steady_clock::time_point Tactual = chrono::steady_clock::now();

    //clean the list (objects not updated for the last 2 seconds)
    for(i=0;i<MapList.size();i++){
        if(MapList[i].Tick != FrameCnt){
            if(MapList[i].obj_id == CAR){
                PLOG_DEBUG << " Exit = " << MapList[i].track_id << "  @ " << MapList[i].ValidTicks;
                if(MapList[i].ValidTicks > Js.MinFourTicks){
                    //remove the valid objects after 2 Sec (to repair track mistakes)
                    Telapse = (chrono::duration_cast<chrono::milliseconds>(Tactual-MapList[i].Tend).count()); //mSec
                    if(Telapse>2000){
                        if(MapList[i].obj_id == CAR && MapList[i].Loc.Velocity!=0.0){
                            //check if the object bounce to the DNN_rect
                            PLOG_INFO << " map  left=" << MapList[i].x << "  top=" << MapList[i].y << "  right=" << (MapList[i].x+MapList[i].w) << "  bottom=" << (MapList[i].y+MapList[i].h);
                            PLOG_INFO << " dnn  left=" << DnnRect.x << "  top=" << DnnRect.y << "  right=" << (DnnRect.x+DnnRect.width) << "  bottom=" << (DnnRect.y+DnnRect.height);
                            bool left  =((int) MapList[i].x <= (DnnRect.x+10));
                            bool top   =((int) MapList[i].y <= (DnnRect.y+10));
                            bool right =((int)(MapList[i].x+MapList[i].w)>=(DnnRect.x+DnnRect.width-10));
                            bool bottom=((int)(MapList[i].y+MapList[i].h)>=(DnnRect.y+DnnRect.height-10));
                            PLOG_INFO << " left=" << left << "  top=" << top << "  right=" << right << "  bottom=" << bottom;
                            if(left || top || right || bottom){
                                //object is crossed the border and will not re-enter the list.
                                Portal.Update(MapList[i].Predict);
                                Portal.CalcBorder();
                            }
                            //get license plate. Best found if FrameOcrOn is used
                            //or a single OCR from the best plate image detected.
                            if(Js.FrameOcrOn){
                                MapList[i].PlatePoll.GetBestOcr(PlateOCR);
                            }
                            else{
                                if(MapList[i].PlateFuzzy > 0.0){
                                    //lets read the OCR from the best found plate (if possible)
                                    Ocr.OCR(Plate, PlateOCR);
                                }
                            }
                        }
                        //remove the object from the list
                        MapList.erase(MapList.begin() + i);
                        i--;
                    }
                }
                else{
                    //remove the non-valid objects straight away when not detected any more
                    MapList.erase(MapList.begin() + i);
                    i--;
                }
            }
            else{
                //remove the non-valid objects straight away when not detected any more
                MapList.erase(MapList.begin() + i);
                i--;
            }
        }
    }
}
//----------------------------------------------------------------------------------------
void TMapper::CheckInspection(void)
{
    size_t v,p;
    float Dist;
    int Telapse;

    //check inspection of vehicles
    for(v=0; v<MapList.size(); v++){
        //see if there is a vehicle with track_id and Used in the scene
        if((MapList[v].obj_id == CAR) && (MapList[v].track_id > 0) && (MapList[v].Valid)){
            //persons
            for(p=0; p<MapList.size(); p++){
                //see if there is a person with track_id and Used in the scene
                if((MapList[p].obj_id == 0) && (MapList[p].track_id > 0) && (MapList[p].Valid)){
                    //get the distance between vehicle and person
                    Dist = MapList[v].Loc.Euclidean( MapList[p].Loc );
                    //are they close enough?
                    if(Dist < Wd*Js.InspectDistance){
                        if(MapList[v].Person_ID<0){
                            MapList[v].TFinp     = std::chrono::steady_clock::now();
                            MapList[v].Person_ID = (int) MapList[p].track_id;
                        }
                        MapList[v].TLinp     = std::chrono::steady_clock::now();
                        Telapse = (chrono::duration_cast<chrono::milliseconds>(MapList[v].TLinp-MapList[v].TFinp).count()); //mSec
//                        cout << Telapse << " mSec" << endl;
                        if(!MapList[v].Inspected){
                            //do not reset the check
                            if(Telapse > (Js.InspectTime*1000)){
//                                cout << "TRUEEE" << endl;
                                MapList[v].Inspected=true;
                            }
                        }
                    }
                }
            }
        }
    }
}
//----------------------------------------------------------------------------------------
void TMapper::CheckLicense(cv::Mat& frame, TMapObject& Mo)
{
    if(Mo.obj_id!=CAR) return;
    if(Mo.Loc.Velocity == 0.0) return;

    size_t   mp;
    cv::Mat  BWframe, Eframe, Mframe;
    cv::Rect MemRect;
    float RatioEdge, RatioRect;
    int   tooTall, tooWide;
    float outsideFocusX;
    float outsideFocusY;
    float nearPredict = 0.0;
    float MemEdge = 0.0;
    float Wfuzzy;
    std::string OCRstr;

    Mo.PlatePlot.width=0;       //reset drawing the plate

    //Create the ROI
    cv::Mat frame_car = frame(cv::Rect(Mo.x, Mo.y, Mo.w, Mo.h));

    //detect plates
    vector<bbox_t> Bplate = PlateNet->detect(frame_car,Js.ThresPlate);

    if(Bplate.size()==0) return;

    //get the best probability
    for(size_t p=0;p<Bplate.size(); p++){
        //not too small !
        if((Bplate[p].w < 25)||(Bplate[p].h < 5)) continue;
        if(Bplate[p].prob > nearPredict){ nearPredict = Bplate[p].prob; mp=p; }
    }

    if(nearPredict>0.0){
        //found the nearest plate. check if this plate is better readable than the one we got
        //enlarge the plate somewhat
        Bplate[mp].x=std::max(                  1,(int)Bplate[mp].x-3);
        Bplate[mp].y=std::max(                  1,(int)Bplate[mp].y-3);
        Bplate[mp].w=std::min(Mo.w-Bplate[mp].x-1,     Bplate[mp].w+6);
        Bplate[mp].h=std::min(Mo.h-Bplate[mp].y-1,     Bplate[mp].h+6);

        tooTall = Bplate[mp].h > 100;
        tooWide = Bplate[mp].w > 400;
        outsideFocusX = (Bplate[mp].x < 0.08 * Mo.w) || ((Bplate[mp].x+Bplate[mp].w) > 0.95 * Mo.w);
        outsideFocusY = (Bplate[mp].y < 0.08 * Mo.h) || ((Bplate[mp].y+Bplate[mp].h) > 0.95 * Mo.h);
        RatioRect = (float)Bplate[mp].w / (float)Bplate[mp].h;

        if(!tooTall && !tooWide && !outsideFocusX && !outsideFocusY && RatioRect>Js.PlateRatio && RatioRect<6.0){
            cv::Rect Prect=cv::Rect(Bplate[mp].x, Bplate[mp].y, Bplate[mp].w, Bplate[mp].h);
            //get the frame with the vehicle and convert it to gray scale
            cv::cvtColor(frame_car(Prect), BWframe, CV_BGR2GRAY);
            //get morphology
            cv::morphologyEx(BWframe, Mframe, cv::MORPH_BLACKHAT, MorphKernal);
            //get vertical(!) edges -> license plates has a lot of them
            cv::Sobel(Mframe,Eframe,CV_16S, 1, 0, 3);
            cv::convertScaleAbs(Eframe,Eframe);
            cv::threshold(Eframe, Eframe, 64, 255, cv::THRESH_BINARY);

            RatioEdge = (float)(100*cv::countNonZero(Eframe))/(Eframe.rows*Eframe.cols);
            if(MemEdge<RatioEdge && RatioEdge>7.0f){
                MemEdge = RatioEdge;
                MemRect = Prect;
                Mo.PlatePlot = cv::Rect(Prect.x+Mo.x, Prect.y+Mo.y, Prect.width, Prect.height);
                //see if we must store the picture of the plate
                if(Js.Plate_Folder!="none"){
                    cv::Mat frame_plate = frame(Mo.PlatePlot);
                    std::string NameStr = Js.Plate_Folder+"/"+CamID+"-"+to_string(FrameCnt)+"_"+Mo.CarChr+"_1_utc.jpg";
                    cv::imwrite(NameStr, frame_plate);
                    strcpy(Mo.PlateName, NameStr.c_str());
                }
                if(Js.FrameOcrOn){
                    //lets read the OCR (if possible)
                    cv::Mat frame_plate = frame(Mo.PlatePlot);
                    float Pb=Ocr.OCR(frame_plate,OCRstr);
                    Mo.PlatePoll.Add(OCRstr,Pb);
                    //publish the best result so far
                    PlateQuality  = Mo.PlatePoll.GetBestOcr(PlateOCR);
                    strcpy(Mo.PlateOCR, PlateOCR.c_str());
                    PLOG_DEBUG << " Plate = " << OCRstr;
                }
            }
        }
    }

    //see if we can find the 'best' plate over time
    if(MemEdge > 3.0){
        //we have a valid plate found, but is the better than the one we found
        //first hit will always be stored due to PlateFuzzy=0
        Wfuzzy = (MemEdge*MemRect.width)/pow(Mo.Loc.Velocity, 0.33);
        //look if its a better plate
        if(Mo.PlateFuzzy < Wfuzzy){
            Mo.PlateFuzzy = Wfuzzy;
            Mo.PlateRect  = cv::Rect(MemRect.x+Mo.x, MemRect.y+Mo.y, MemRect.width, MemRect.height);
            Mo.PlateEdge  = MemEdge;      //MemEdge hold the highest edge ratio found in MemRect
            //this is global, one plate per pipe
            Plate         = frame_car(MemRect).clone();
            PlateObjectID = Mo.obj_id;
            PlateTrackID  = Mo.track_id;
            if(!Js.FrameOcrOn){
                PlateOCR  = "";
            }
            else{
                PlateQuality  = Wfuzzy;
            }
        }
    }
}
//----------------------------------------------------------------------------------------
bool TMapper::GetInfo(const bbox_t box,bool &Valid,cv::Rect& rt)
{
    bool Found=false;

    for(auto &i : MapList){
        if((i.obj_id == box.obj_id) && (i.track_id == box.track_id)){
            Found=true;  Valid=i.Valid; rt=i.PlatePlot;
            break;
        }
    }

    return Found;
}
//----------------------------------------------------------------------------------------
float TMapper::Warp(float Y)
{
    float Exp = Wp;
    float Mx2 = Ht*Gn;
    float D1  = pow(Ht,Exp);
    float E1  = D1/Ht;
    float In  = LIM(Y, 0, Ht);

    float Out = (D1-pow((Mx2-In),Exp))/E1;

    if(Out<0)   Out=0;
    if(Out>=Ht) Out=Ht-1;

    return Out;
}
//----------------------------------------------------------------------------------------
float TMapper::InvWarp(float X)
{
    float Exp = Wp;
    float Mx2 = Ht*Gn;
    float D1  = pow(Ht,Exp);
    float E1  = D1/Ht;
    float In  = LIM(X, 0, Ht);

    float Out = Mx2-pow((D1-In*E1),(1.0/Exp));

    if(Out<0)   Out=0;
    if(Out>=Ht) Out=Ht-1;

    return Out;
}
//----------------------------------------------------------------------------------------
void TMapper::PlotDebug(void)
{
    float f;
    bool in;
    char text[256];
    int x,y,x1,y1,x2,y2,baseLine;
    cv::Scalar Color;
    cv::Mat m2;
    cv::Mat m(Ht,Wd, CV_8UC3, cv::Scalar(48, 10, 36));

    for(auto i:MapList){
        //the prediction
        switch(i.obj_id){
            case PERSON    : Color=cv::Scalar(0, 255, 255); break;
            case CAR       : Color=cv::Scalar(255, 0, 255); break;
            case MOTORBIKE : Color=cv::Scalar(255, 255, 0); break;
        }
        if(i.obj_id==CAR){
            i.Predict.Predict(x,y);
            if(i.Valid) {
                if(i.Inside) cv::circle(m,cv::Point(x,y),12,Color,CV_FILLED);
                else         cv::circle(m,cv::Point(x,y),12,cv::Scalar(255, 255, 255),CV_FILLED);
            }
            else        cv::circle(m,cv::Point(x,y),4,Color,2);

            sprintf(text, " %i - %.3f", i.track_id, i.Loc.Velocity);
//            sprintf(text, " %i", i.track_id);
            baseLine = 0;
            cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, &baseLine);

            y = y - label_size.height - baseLine;
            if (y < 0) y = 0;
            if (x + label_size.width > m.cols) x = m.cols - label_size.width;

            cv::putText(m, text, cv::Point(x, y + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);

            i.Predict.Predict(x1,y1,8);
            i.Predict.Predict(x2,y2,30);
            cv::line(m,cv::Point(x1,y1),cv::Point(x2,y2),cv::Scalar(0, 0, 255),2);

            //crossing the border
            DPoint Pt=Portal.Predict(i.Predict,f,x,in);
            cv::circle(m,cv::Point(Pt.X,Pt.Y),12,cv::Scalar(255, 255, 0),CV_FILLED);
//            cout << Pt.X << " - " << Pt.Y << endl;

        }
    }

//    TOrinVec Vec=Portal.Location();
//    if(Vec.X<8) Vec.X=8;
//    if(Vec.Y<8) Vec.Y=8;
//    if(Vec.X>(Wd-8)) Vec.X=Wd-8;
//    if(Vec.Y>(Ht-8)) Vec.Y=Ht-8;
//    cv::circle(m,cv::Point(Vec.X,Vec.Y),12,cv::Scalar(0, 255, 255),CV_FILLED);

    Portal.Border.Get(-150,x1,y1); Portal.Border.Get(150,x2,y2);
    cv::line(m,cv::Point(x1,y1),cv::Point(x2,y2),cv::Scalar(0, 255, 255),2);

    resize(m, m2, cv::Size(m.cols/2,m.rows/2), 0, 0, cv::INTER_LINEAR);
    cv::imshow("Debug",m2);
}
//----------------------------------------------------------------------------------------
