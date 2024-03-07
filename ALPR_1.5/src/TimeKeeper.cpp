#include "TimeKeeper.h"
//---------------------------------------------------------------------------
TTimeKeeper::TTimeKeeper()
{
}
//---------------------------------------------------------------------------
void TTimeKeeper::Start(int Frate, int Fstart)
{
    Tbegin     = std::chrono::steady_clock::now();
    FrameRate  = Frate;
    FirstFrame = Fstart;
    OneTick    =(FrameRate>0)? 1.000/FrameRate : 0.0;
}
//---------------------------------------------------------------------------
slong64 TTimeKeeper::GetTime(int Fcnt)
{
    slong64 t;

    if(FrameRate > 0){
        t=(1000*(Fcnt-FirstFrame))/FrameRate;
    }
    else{
        std::chrono::steady_clock::time_point Tend = std::chrono::steady_clock::now();
        t=static_cast<slong64>(std::chrono::duration_cast<std::chrono::milliseconds> (Tend - Tbegin).count());
    }

    return t;
}
//---------------------------------------------------------------------------
std::string TTimeKeeper::NiceTime_Sec(const slong64 Start, const slong64 End)
{
    slong64 t=(End-Start)/1000;
    slong64 m=t/60;
    slong64 s=t%60;
    std::string Tst="";

    if(t<=0) Tst="00:00";
    else{
        if(m<=9) Tst="0";
        Tst+=std::to_string(m) + ":";

        if(s<=9) Tst+="0";
        Tst+=std::to_string(s);
    }

    return Tst;
}
//---------------------------------------------------------------------------
std::string TTimeKeeper::NiceTime_mSec(const slong64 Start, const slong64 End)
{
    slong64 t=(End-Start)/100;
    slong64 m=t/10;
    slong64 s=t%10;
    std::string Tst="";

    Tst+=std::to_string(m) + ".";
    Tst+=std::to_string(s);

    return Tst;
}
//---------------------------------------------------------------------------
std::string TTimeKeeper::NiceTime_mSec(const slong64 Sec)
{
    slong64 t=Sec*10;
    slong64 m=t/10;
    slong64 s=t%10;
    std::string Tst="";

    Tst+=std::to_string(m) + ".";
    Tst+=std::to_string(s);

    return Tst;
}
//---------------------------------------------------------------------------
std::string TTimeKeeper::NiceTimeUTC(const slong64 t)
{
    std::string Tst;

    std::time_t tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()
                 + std::chrono::duration_cast<std::chrono::system_clock::duration>(Tbegin - std::chrono::steady_clock::now()));
    std::tm tm = std::tm{0};

    tt+=(t/1000);
    gmtime_r(&tt, &tm);

    if (tm.tm_hour <= 9)    Tst  = "0" + std::to_string(tm.tm_hour) + ":";
    else                    Tst  =       std::to_string(tm.tm_hour) + ":";
    if (tm.tm_min <= 9)     Tst += "0" + std::to_string(tm.tm_min ) + ":";
    else                    Tst +=       std::to_string(tm.tm_min ) + ":";
    if (tm.tm_sec <= 9)     Tst += "0" + std::to_string(tm.tm_sec ) + " ";
    else                    Tst +=       std::to_string(tm.tm_sec ) + " ";
    Tst += std::to_string(tm.tm_year+1900) +"-";
    if ((tm.tm_mon+1) <= 9) Tst += "0" + std::to_string(tm.tm_mon+1) + "-";
    else                    Tst +=       std::to_string(tm.tm_mon+1) + "-";
    if ((tm.tm_mday ) <= 9) Tst += "0" + std::to_string(tm.tm_mday);
    else                    Tst +=       std::to_string(tm.tm_mday);

    return Tst;
}
//---------------------------------------------------------------------------

