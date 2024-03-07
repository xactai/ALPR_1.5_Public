#ifndef TFPS_H
#define TFPS_H

#include "General.h"
//---------------------------------------------------------------------------
/**
    A simple class to measure the FPS (latency) of a process.
*/
//---------------------------------------------------------------------------
class TFPS
{
private:
protected:
    int Fcnt;                   ///< Points to the next free Far[]
    float Far[32];              ///< Use to average the output over the last 32 frames
    std::chrono::steady_clock::time_point Tbegin, Tend;     ///< Some timing
public:
    TFPS();                     ///< Constructor

    float FrameRate;            ///< Average output FPS over the last 32
    float Latency;              ///< Average output in mSec over the last 32

    void Init(void);            ///< Initialize the Far[] array and its calcus.
    void Update(void);          ///< Use Update() every time after cycle is ready.

    void Start(void);           ///< Sets the starting point
    void Stop(void);            ///< Sets the stop point and calculate the FPS and mSec with the previous values in Far
};
//---------------------------------------------------------------------------
#endif // TFPS_H
