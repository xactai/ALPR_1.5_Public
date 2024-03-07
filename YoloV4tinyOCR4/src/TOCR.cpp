#include "TOCR.h"
#include "Tjson.h"
#include "Regression.h"
#include "yolo_v2_class.hpp"	        // imported functions from .so
#include <math.h>

//----------------------------------------------------------------------------------------
using namespace std;
//----------------------------------------------------------------------------------------
extern Tjson    Js;
extern Detector *OcrNet;
extern vector<string> OCRnames;

extern int Msec;
extern int Mcnt;
//----------------------------------------------------------------------------------
// TOCR
//----------------------------------------------------------------------------------
TOCR::TOCR()
{
    //ctor
}
//----------------------------------------------------------------------------------
TOCR::~TOCR()
{
    //dtor
}
//----------------------------------------------------------------------------------------
//note, SortSingleLine can erase elements from cur_bbox_vec
//that way shorten the length of the vector. Hence size_t& bnd
void TOCR::SortSingleLine(vector<bbox_t>& cur_bbox_vec, float ch_wd, float ch_ht, size_t StartPos, size_t& StopPos)
{
    size_t i, j;
    bbox_t tmp_box;
    int d, i1, i2;

    if((StopPos-StartPos)<=1) return;

    //sort by x position
    for(i=StartPos; i<StopPos; i++){
        for(j=i+1; j<StopPos; j++){
            if(cur_bbox_vec[j].x<cur_bbox_vec[i].x){
                //swap
                tmp_box=cur_bbox_vec[j];
                cur_bbox_vec[j]=cur_bbox_vec[i];
                cur_bbox_vec[i]=tmp_box;
            }
        }
    }

    //get the distance between each char, too close? select the highest prob.
    for(i=StartPos; i<StopPos-1; i++){
        i1=cur_bbox_vec[i].x; i2=cur_bbox_vec[i+1].x;
        d=(i2-i1)*2;            //d<0? two lines and jumping from the top to the bottom line.
        if(d>=0 && d<ch_wd){
            if(cur_bbox_vec[i+1].prob < cur_bbox_vec[i].prob) cur_bbox_vec.erase(cur_bbox_vec.begin()+i+1);
            else                                              cur_bbox_vec.erase(cur_bbox_vec.begin()+i);
            StopPos--;  i--;    //one element less in the array, due to the erase
        }
    }
}
//----------------------------------------------------------------------------------------
void TOCR::SortPlate(vector<bbox_t>& cur_bbox_vec)
{
    size_t i, j, n, bnd;
    size_t len=cur_bbox_vec.size();
    bbox_t tmp_box;
    float prb, ch_wd, ch_ht;
    double A, B, R;
    TLinRegression LinReg;

    if((Js.PlateNrChars < 2) || (len < 2)) return;         //no need to investigate 1 character

    //check nr of chars
    while(len > (size_t)Js.PlateNrChars){
        //get the lowest probability
        for(prb=1000.0, i=0;i<len;i++){
            if(cur_bbox_vec[i].prob < prb){ prb=cur_bbox_vec[i].prob; n=i;}
        }
        //delete the lowest
        cur_bbox_vec.erase(cur_bbox_vec.begin()+n);
        len=cur_bbox_vec.size();
    }

    //get average width and height of the characters
    for(ch_ht=ch_wd=0.0, i=0; i<len; i++){
        ch_wd+=cur_bbox_vec[i].w;
        ch_ht+=cur_bbox_vec[i].h;
    }
    ch_wd/=len; ch_ht/=len;

    if(len > 4){
        //get linear regression through all (x,y)
        for(i=0; i<len; i++){
            LinReg.Add(cur_bbox_vec[i].x, cur_bbox_vec[i].y);
        }
        LinReg.Execute(A,B,R);
        //now you can do a warp perspective if the skew is too large.
        //in that case, you have to run the ocr detection again.
        //here we see how well a single line fits all the characters.
        //if the standard deviation is high, you have one line of text
        //if the R is low, you have a two line number plate.

//        cout << "A = " << A << "  B = " << B << "  R = " << R << endl;
    }
    else{
        R=1.0;  // with 4 or less characters, assume we got always one line.
    }
    if(R<0.08 && A>-1.0 && A<1.0){
        //two lines -> sort on y first
        for(i=0; i<len; i++){
            for(j=i+1; j<len; j++){
                if(cur_bbox_vec[j].y<cur_bbox_vec[i].y){
                    //swap
                    tmp_box=cur_bbox_vec[j];
                    cur_bbox_vec[j]=cur_bbox_vec[i];
                    cur_bbox_vec[i]=tmp_box;
                }
            }
        }

        //get the boundary between first and second line.
        for(n=0, i=0; i<len-1; i++){
            j=cur_bbox_vec[i+1].y-cur_bbox_vec[i].y;
            if(j>n){ n=j; bnd=i+1; }
        }
        //sort the first line 0 < bnd
        SortSingleLine(cur_bbox_vec, ch_wd, ch_ht, 0, bnd);

        len=cur_bbox_vec.size();        //SortSingleLine can shorten the length of the vector
        //sort the second line bnd < len
        SortSingleLine(cur_bbox_vec, ch_wd, ch_ht, bnd, len);
    }
    else{
        //one line -> sort by x position
        SortSingleLine(cur_bbox_vec, ch_wd, ch_ht, 0, len);
    }
}
//----------------------------------------------------------------------------------------
float TOCR::OCR(cv::Mat& frame,std::string &PlateOCR)
{
    size_t i,t;
    char text[32];
    float d,f,Prob=0.0;
    vector<bbox_t> result_ocr;
    size_t Len=OCRnames.size();

    PlateOCR=" ?? "; //default, in case no license is recognized.

    if(!frame.empty()){
        //detect plates
        result_ocr = OcrNet->detect(frame,Js.ThresOCR);
        if(result_ocr.size()>0){
            //heuristics
            if(Js.HeuristicsOn){
                SortPlate(result_ocr);
            }
            //transfer to std::string
            for(f=1.0, i=0; (i<result_ocr.size() && i<30);i++){ //i<30 so i++ (now i=31) text[i]=0; is still valid
                t=result_ocr[i].obj_id;
                if(t<Len) text[i]=OCRnames[result_ocr[i].obj_id][0];
                else      text[i]='?';
                //get the lowest probability
                if(result_ocr[i].prob<f) f=result_ocr[i].prob;
            }
            text[i]=0; //closing (0=endl);
            PlateOCR=text;
            //reduce probability when characters are missing.
            if((int)result_ocr.size() < Js.PlateNrChars){
                if(Js.PlateNrChars>0){
                    d=result_ocr.size();
                    f*=(0.6*d/Js.PlateNrChars);
                }
            }
            Prob=f;
        }
    }

    return Prob;
}
//----------------------------------------------------------------------------------
// TOcrPoll
//----------------------------------------------------------------------------------
TOcrPoll::TOcrPoll()
{
    Clear();
}
//----------------------------------------------------------------------------------
TOcrPoll::~TOcrPoll()
{
    //dtor
}
//----------------------------------------------------------------------------------
void TOcrPoll::Init(int NrPlateChars)
{
    CharCnt = NrPlateChars+1; //ending zero
    if(CharCnt>32) CharCnt=32;
}
//----------------------------------------------------------------------------------
void TOcrPoll::Add(std::string PlateOCR, float Prob)
{
    float p;
    int i,n,t,r;
    TOcrObject NewPl;

    for(i=0;i<CharCnt;i++) NewPl.PlateOCR[i]=PlateOCR.c_str()[i];
    NewPl.Prob=Prob; NewPl.Hits=1;

    //check if used
    for(r=i=0;i<Len;i++){
        for(t=n=0;n<CharCnt;n++){
            if(NewPl.PlateOCR[n]==PlateList[i].PlateOCR[n]) t++;
        }
        if(t==CharCnt){
            //found match
            PlateList[i].Hits++;
            if(Prob>PlateList[i].Prob) PlateList[i].Prob=Prob;
            r=1; break;
        }
    }
    if(r==0){
        //new one
        if(Len<PLATE_LIST_SIZE) PlateList[Len++]=NewPl;
        else{
            //list full, replace the worse
            //find the one with the least Hits.
            t=PlateList[0].Hits; p=PlateList[0].Prob; n=0;
            for(i=1;i<Len;i++){
                if(PlateList[i].Hits<t){ t=PlateList[i].Hits; p=PlateList[i].Prob; n=i; }
                else{
                    //equal Hits, lets replace the worse probability.
                    if(PlateList[i].Hits==t && PlateList[i].Prob<p){ t=PlateList[i].Hits; p=PlateList[i].Prob; n=i; }
                }
            }
            //result of the scan
            t=PlateList[n].Hits; p=PlateList[n].Prob;
            if(p<NewPl.Prob){
                if(t<3){
                    //replace, less than three hits and a worse probability.
                    PlateList[n]=NewPl;
                }
                else{
                    //replace only when the found prob is very poor (a few chars) compare the the new one.
                    if(p<(NewPl.Prob*0.5)) PlateList[n]=NewPl;
                }
            }
        }
    }
}
//----------------------------------------------------------------------------------
float TOcrPoll::GetBestOcr(std::string &PlateOCR)
{
    float q,p,Pr=0.0;
    int i,n;

    if(Len>0){
        p=PlateList[0].Prob*pow(PlateList[0].Hits,0.15); n=0;
        for(i=1;i<Len;i++){
            q=PlateList[i].Prob*pow(PlateList[i].Hits,0.15);
            if(q>p) { p=q; n=i; }
        }
        //result of the scan
        PlateOCR=PlateList[n].PlateOCR; Pr=PlateList[n].Prob;
    }

    return Pr;
}
//----------------------------------------------------------------------------------------
void TOcrPoll::Clear(void)
{
    Len=0; CharCnt=10;
}
//----------------------------------------------------------------------------------------
