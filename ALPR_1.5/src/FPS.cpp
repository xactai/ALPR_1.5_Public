#include "FPS.h"
//---------------------------------------------------------------------------
TFPS::TFPS()
{
    Fcnt=0;
    FrameRate=0.0;
    Latency  =0.0;
    for(int i=0;i<32;i++) Far[i]=0.0;
}
//---------------------------------------------------------------------------
void TFPS::Init(void)
{
    Fcnt=0;
    for(int i=0;i<32;i++) Far[i]=0.0;
    Tbegin = std::chrono::steady_clock::now();
}
//---------------------------------------------------------------------------
void TFPS::Update(void)
{
    Stop();
    Tbegin = std::chrono::steady_clock::now();
}
//---------------------------------------------------------------------------
void TFPS::Start(void)
{
    Tbegin = std::chrono::steady_clock::now();
}
//---------------------------------------------------------------------------
void TFPS::Stop(void)
{
    int i;
    float f;

    Tend = std::chrono::steady_clock::now();
    f = std::chrono::duration_cast<std::chrono::milliseconds> (Tend - Tbegin).count();

    Far[Fcnt]=f;
    Fcnt++; Fcnt&=0x01F;                        //Fnct=(Fcnt+1)%32

    for(f=0.0, i=0;i<32;i++) f+=Far[i];
    f/=32.0;
    if(f>0.0){
        Latency  = f;
        FrameRate=1000.0/f;                      //f in milliseconds
    }
}
//---------------------------------------------------------------------------
