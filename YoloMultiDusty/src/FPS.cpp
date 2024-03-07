#include "FPS.h"
#define AVERAGE 5  //number of frames per Far[] entry.
//---------------------------------------------------------------------------
TFPS::TFPS()
{
    Cnt=0;
    Fcnt=0;
    FrameRate=0.0;
    FirstInit=true;
}
//---------------------------------------------------------------------------
void TFPS::Init(void)
{
    Cnt=0;
    Fcnt=0;
    for(int i=0;i<16;i++) Far[i]=0.0;
    FirstInit=true;
    FirstFull=false;
    Tbegin = std::chrono::steady_clock::now();
}
//---------------------------------------------------------------------------
void TFPS::Update(void)
{
    int i;
    float f;

    Cnt++;
    Tend = std::chrono::steady_clock::now();
    f = std::chrono::duration_cast<std::chrono::milliseconds> (Tend - Tbegin).count();
    //new sequence?
    if(Cnt>=AVERAGE){
        Far[Fcnt]=f;

        Fcnt++; Fcnt&=0x0F;                 //Fnct=(Fcnt+1)%16
        if(Fcnt==0) FirstFull=true;

        Far[Fcnt]=0.0;
        Cnt=0; FirstInit=false;

        Tbegin = std::chrono::steady_clock::now();
    }
    //get average
    if(FirstInit){
        //less than 20 frames
        FrameRate=(Cnt*1000.0)/f;
    }
    else{
        if(FirstFull){
            //all entries are filled
            for(f=0.0, i=0;i<16;i++){
                if(i!=Fcnt) f+=Far[i];              //Fcnt is being filled at the moment, don't use
            }
            FrameRate=(AVERAGE*15*1000.0)/f;        //f in milliseconds (15 because we skipped i==Fcnt)
        }
        else{
            for(f=0.0, i=0;i<Fcnt;i++) f+=Far[i];
            FrameRate=(AVERAGE*Fcnt*1000.0)/f;      //f in milliseconds
        }
    }
}
//---------------------------------------------------------------------------
