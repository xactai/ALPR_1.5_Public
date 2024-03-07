#include "General.h"
#include "TAlbum.h"
#include "Tjson.h"

//----------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------

extern Tjson  Js;
extern slong64 FrameCnt;
extern Detector *PlateNet;

//----------------------------------------------------------------------------------------
// TAlbum
//----------------------------------------------------------------------------------------
TAlbum::TAlbum()
{
    for(int i=0;i<ALBUM_SIZE;i++) Ar[i]=nullptr;
}
//----------------------------------------------------------------------------------------
TAlbum::~TAlbum()
{
    FreeAll();
}
//----------------------------------------------------------------------------------------
void TAlbum::Update(TMapObject &Mo,cv::Mat &Img)
{
    int i,p=-1;
    slong64 old=FrameCnt;

    for(i=0;i<ALBUM_SIZE;i++){
        if(Ar[i]!=nullptr){
            //found the active worker.
            if(Ar[i]->Cars[0]->TrackID == Mo.track_id) p=i;
        }
    }
    if(p<0){
        //new vehicle, add to the list
        for(i=0;i<ALBUM_SIZE;i++){
            if(Ar[i]==nullptr){
                //free entry, let's us it
                Ar[i] = new TWorker();  p=i;
            }
        }
    }
    if(p<0){
        //no worker left, remove the oldest (smallest Tick)
        for(i=0;i<ALBUM_SIZE;i++){
            if(Ar[i]->Cars[0]->Tick < old ){
                old = Ar[i]->Cars[0]->Tick; p=i;
            }
        }
        if(p>=0 && p<ALBUM_SIZE){
            delete Ar[p];           //delete worker p
            Ar[p] = new TWorker();  //and allocate an empty one
        }
    }
    if(p>=0 && p<ALBUM_SIZE){
        Ar[p]->Update(Mo, Img);
    }
}
//----------------------------------------------------------------------------------------
void TAlbum::Execute(TMapObject &Mo)
{
    int i,p=-1;

    for(i=0;i<ALBUM_SIZE;i++){
        if(Ar[i]!=nullptr){
            //found the active worker.
            if(Ar[i]->Cars[0]->TrackID == Mo.track_id) p=i;
        }
    }
    if(p>=0 && p<ALBUM_SIZE){
        Ar[p]->Execute(Mo);
    }
}
//----------------------------------------------------------------------------------------
void TAlbum::Remove(TMapObject &Mo)
{
    for(int i=0;i<ALBUM_SIZE;i++){
        if(Ar[i]!=nullptr){
            //found the active worker.
            if(Ar[i]->Cars[0]->TrackID == Mo.track_id){
                delete Ar[i];           //delete worker p
                Ar[i]=nullptr;
            }
        }
    }
}
//----------------------------------------------------------------------------------------
void TAlbum::Show(cv::Mat& frame)
{
    int b, i, w, h;
    int Wx=10;
    int Wd=0;
    int Ht=0;
    int baseLine;

    //get max width and height
    for(i=0;i<ALBUM_SIZE;i++){
        if(Ar[i]!=nullptr){
            baseLine=0;
            b=Ar[i]->BestPlate;
            if(b>=0 && b<WORKER_SIZE){
                TCar* car = Ar[i]->Cars[b];
                if(std::strlen(car->OCR)>1){
                    cv::Size label_size = cv::getTextSize(car->OCR, cv::FONT_HERSHEY_SIMPLEX, 1, 2, &baseLine);
                    Wd+= max(2*car->Prect.width+20, label_size.width+20);
                    Ht = max(Ht, 2*car->Prect.height+label_size.height+30);
                }
                else{
                    Wd+= 2*car->Prect.width+20;
                    Ht = max(Ht, 2*car->Prect.height+20);
                }
            }
        }
    }
    //plot
    Wd=min(Wd, frame.cols-1);
    if(Wd>0 && Ht>0){
        cv::Mat Overview(Ht, Wd, CV_8UC3, cv::Scalar(255,255,255));
        for(i=0;i<ALBUM_SIZE;i++){
            if(Ar[i]!=nullptr){
                baseLine=0;
                cv::Mat SnapBig, Snap;
                b=Ar[i]->BestPlate;
                if(b>=0 && b<WORKER_SIZE){
                    TCar* car = Ar[i]->Cars[b];
                    Snap = car->Picture(car->Prect);
                    cv::resize(Snap, SnapBig, cv::Size(2*car->Prect.width, 2*car->Prect.height));
                    if(std::strlen(car->OCR)>1){
                        cv::Size label_size = cv::getTextSize(car->OCR, cv::FONT_HERSHEY_SIMPLEX, 1, 2, &baseLine);
                        w = SnapBig.cols;  h = SnapBig.rows;
                        if((Wx+w)<frame.cols){
                            cv::putText(Overview, car->OCR, cv::Point(Wx, 10 + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0,0,0), 2);
                            SnapBig.copyTo(Overview(cv::Rect(Wx, label_size.height+20, w, h)));
                        }
                        else break;
                        w = max(SnapBig.cols, label_size.width);
                    }
                    else{
                        w = SnapBig.cols;
                        h = SnapBig.rows;
                        if((Wx+w)<frame.cols){
                            SnapBig.copyTo(Overview(cv::Rect(Wx, 10, w, h)));
                        }
                        else break;
                    }
                    Wx+=w+20;
                }
            }
        }
        Overview.copyTo(frame(cv::Rect(0, frame.rows-Ht, Wd, Ht)));
    }
}
//----------------------------------------------------------------------------------------
void TAlbum::FreeAll(void)
{
    for(int i=0;i<ALBUM_SIZE;i++){
        if(Ar[i]!=nullptr) delete Ar[i];        //delete the worker
        Ar[i]=nullptr;
    }
}
//----------------------------------------------------------------------------------------
