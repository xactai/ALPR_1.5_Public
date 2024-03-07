#ifndef TFPS_H
#define TFPS_H

#include "General.h"
//---------------------------------------------------------------------------
class TFPS
{
private:
protected:
    int Cnt;
    int Fcnt;
    bool FirstInit;
    bool FirstFull;
    float Far[16];      //every entry holds average frame rate of 32 frames
    std::chrono::steady_clock::time_point Tbegin, Tend;
public:
    TFPS();

    float FrameRate;

    void Init(void);
    void Update(void);
};
//---------------------------------------------------------------------------
#endif // TFPS_H
