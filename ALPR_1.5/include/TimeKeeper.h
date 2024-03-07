#ifndef TIME_KEEPER_H
#define TIME_KEEPER_H

#include "General.h"
//---------------------------------------------------------------------------
class TTimeKeeper
{
private:
    int FirstFrame;
    int FrameRate;
    std::chrono::steady_clock::time_point Tbegin;
public:
    TTimeKeeper();

    double OneTick;                                                     ///< Ticks in SECONDS!!! 30FPS = 0.03333
    void Start(int Frate=0, int Fstart=0);                              ///< Set the timer to 0 mSec [Frate=0 use real time, Frate>0 use 1000/Frate]
    slong64 GetTime(int Fcnt);                                          ///< Get the time elapsed from Start() in mSec [Frate=0 use real time, Fcnt>0 use frame rate]
    std::string NiceTime_Sec(const slong64 Start, const slong64 End);   ///< Get a nice 00:00 string in seconds
    std::string NiceTime_mSec(const slong64 Start, const slong64 End);  ///< Get a nice 0.0 string in milliseconds
    std::string NiceTime_mSec(const slong64 Sec);                       ///< Get a nice 0.0 string in milliseconds
    std::string NiceTimeUTC(const slong64 t);                           ///< Get a nice UTC string
};
//---------------------------------------------------------------------------
#endif // TIME_KEEPER_H
