#include "TWorker.h"
#include <cstdio> // for snprintf
//----------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------
extern Tjson  Js;
extern slong64 FrameCnt;
extern Detector *PlateNet;
//----------------------------------------------------------------------------------
// TWorker
//----------------------------------------------------------------------------------
TWorker::TWorker(void)
{
    //get your array for pictures.
    Cars = new TCar*[WORKER_SIZE];
    for(int i=0; i<WORKER_SIZE; i++){
        Cars[i] = new TCar;
    }
    BestPlate=-1;
}
//----------------------------------------------------------------------------------
TWorker::~TWorker(void)
{
    for(int i=0; i<WORKER_SIZE; i++){
        delete Cars[i];
    }
    delete[] Cars;
}
//----------------------------------------------------------------------------------------
void TWorker::Update(TMapObject &Mo,cv::Mat &Img)
{
    int i, p=-1;
    float Fz, f=0.0;

    /////////////////////////
    //   get fuzzy number  //
    /////////////////////////

    //the larger the image, the larger the license
    Fz  = Mo.Size;
    //the lower the speed, the sharper the image. (less blur)
    Fz /= pow(Mo.Loc.Velocity, 0.33);
    //nose right in front of the camera (angle is 90 = M_PI_2)
    Fz *= pow(1.75,(-1*(Mo.Loc.Angle-M_PI_2)*(Mo.Loc.Angle-M_PI_2)));
    //search for an empty entry
    for(i=0; i<WORKER_SIZE && p<0; i++){
        if(Cars[i]->Tick == 0) p=i;
    }
    if(p<0){
        //not found, find highest fuzzy.
        for(i=0; i<WORKER_SIZE; i++){
            if(Cars[i]->Fuzzy>f ) f=Cars[i]->Fuzzy;
        }
        if(f*1.1 < Fz){
            //replace only the worse fuzzy when new fuzzy is 10% better
            for(i=0; i<WORKER_SIZE; i++){
                if(Cars[i]->Fuzzy<f) { f=Cars[i]->Fuzzy; p=i; }
            }
        }
    }
    //p<0 ? this fuzzy number is worse than the ones in the array
    if(p>=0 && p<WORKER_SIZE){
        Cars[p]->Tick    = FrameCnt;
        Cars[p]->TrackID = Mo.track_id;
        Cars[p]->Fuzzy   = Fz;
        Cars[p]->Picture = Img.clone();
        PLOG_DEBUG << " New fuzzy = " << Fz << " @ " << Mo.track_id;
    }
}
//----------------------------------------------------------------------------------
void TWorker::Execute(TMapObject &Mo)
{
    int i;
    float f=0.0;

    BestPlate=-1;

    // Sort the array by Fuzzy value
    sortCarPointersByFuzzy(Cars, WORKER_SIZE);

    //find the best picture
    for(i=0; i<WORKER_SIZE; i++){
        //detect plates
        GetLicense(Mo, i);
        if((int)std::strlen(Cars[i]->OCR)==Js.PlateNrChars){
            BestPlate=i; break;
        }
        else{
            if(Cars[i]->Prob>f){ f=Cars[i]->Prob; BestPlate=i; }
        }
    }
    PLOG_INFO << "  BestPlate: " << BestPlate << " @ " << Mo.track_id;
    if(BestPlate>=0 && BestPlate<WORKER_SIZE){
        //see if we must store the picture of the vehicle
        if(Js.Car_Folder!="none"){
            std::string Str = Js.Car_Folder+"/"+Mo.CamName+"-"+to_string(FrameCnt)+"_"+to_string(Mo.track_id)+"_utc.jpg";
            strcpy(Mo.CarName, Str.c_str());

            cv::imwrite(Str, Cars[BestPlate]->Picture);
        }
        //see if we must store the picture of the plate
        if(Js.Plate_Folder!="none"){
            std::string Str = Js.Plate_Folder+"/"+Mo.CamName+"-"+to_string(FrameCnt)+"_"+to_string(Mo.track_id)+"_1_utc.jpg";
            strcpy(Mo.PlateName, Str.c_str());

            cv::Mat Lmat = Cars[BestPlate]->Picture(Cars[BestPlate]->Prect);
            cv::imwrite(Str, Lmat);
        }
        memcpy(Mo.PlateOCR,Cars[BestPlate]->OCR,32);
        PLOG_INFO << "  OCR: " << Cars[BestPlate]->OCR << " | Prob: " << Cars[BestPlate]->Prob << " @ " << Mo.track_id;
    }
}
//----------------------------------------------------------------------------------
void TWorker::GetLicense(TMapObject &Mo, int p)
{
    size_t mp;
    float RatioRect;
    float nearPredict = 0.0;
    int   tooTall, tooWide;
    float outsideFocusX;
    float outsideFocusY;

    //detect plates
    if(p>=0 && p<WORKER_SIZE){
        Cars[p]->Prob=0.0;
        std::snprintf(Cars[p]->OCR, sizeof(Cars[p]->OCR), "%s", " ?? ");
        if(!Cars[p]->Picture.empty()){
            vector<bbox_t> Bplate = PlateNet->detect(Cars[p]->Picture, Js.ThresPlate);

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
                Bplate[mp].x=MAX(                  1,(int)Bplate[mp].x-3);
                Bplate[mp].y=MAX(                  1,(int)Bplate[mp].y-3);
                Bplate[mp].w=MIN(Mo.w-Bplate[mp].x-1,     Bplate[mp].w+6);
                Bplate[mp].h=MIN(Mo.h-Bplate[mp].y-1,     Bplate[mp].h+6);

                tooTall = Bplate[mp].h > 100;
                tooWide = Bplate[mp].w > 400;
                outsideFocusX = (Bplate[mp].x < 0.05 * Mo.w) || ((Bplate[mp].x+Bplate[mp].w) > 0.95 * Mo.w);
                outsideFocusY = (Bplate[mp].y < 0.05 * Mo.h) || ((Bplate[mp].y+Bplate[mp].h) > 0.95 * Mo.h);
                RatioRect = (float)Bplate[mp].w / (float)Bplate[mp].h;

                PLOG_DEBUG_IF(tooTall) << " License too tall " << Bplate[mp].h << " (100 is the limit)";
                PLOG_DEBUG_IF(tooWide) << " License too wide " << Bplate[mp].w << " (400 is the limit)";
                PLOG_DEBUG_IF(outsideFocusX) << " License outside focus X " << outsideFocusX;
                PLOG_DEBUG_IF(outsideFocusY) << " License outside focus Y " << outsideFocusY;
                PLOG_DEBUG_IF(RatioRect<Js.PlateRatio) << " License wrong rectangle ratio " << RatioRect << " ("<< Js.PlateRatio <<" limit set in json)";
                PLOG_DEBUG_IF(RatioRect>=6.0) << " License wrong rectangle ratio " << RatioRect << " (max 6.0)";

                if(!tooTall && !tooWide && !outsideFocusX && !outsideFocusY && RatioRect>Js.PlateRatio && RatioRect<6.0){
                    Cars[p]->Prect=cv::Rect(Bplate[mp].x, Bplate[mp].y, Bplate[mp].w, Bplate[mp].h);

                    cv::Mat Lmat = Cars[p]->Picture(Cars[p]->Prect);
                    Cars[p]->Prob=Ocr.OCR(Lmat, Cars[p]->OCR);  //lets read the OCR from the best found plate (if possible)
                }
            }
        }
    }
}
//----------------------------------------------------------------------------------
