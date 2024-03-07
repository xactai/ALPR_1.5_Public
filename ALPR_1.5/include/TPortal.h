#ifndef TPORTAL_H
#define TPORTAL_H
#include "General.h"
#include "Locate.h"
//----------------------------------------------------------------------------------------
class TPortal
{
friend class TMapper;
public:
    TPortal();
    virtual ~TPortal();

    void Init(const std::string CamName,const int Widthconst);      ///< Load portal
    void Update(TMover &Mov);                                       ///< Update the moving average
    DPoint Predict(TMover &Mov,bool &In);                           ///< Predict where and when the vehicle crosses the border

    int Length;             ///< 0.5*Width on both side of center
    TParaLine Border;
protected:
    int  Freeze;
    void Load(void);
    void Save(void);

    std::string FileName;
    std::string CamStr;
    TMoveAver<int> Xo;      ///< X position storage
    TMoveAver<int> Yo;      ///< Y position storage
    TMoveAver<float> An;    ///< angle storage
    TMoveAver<int> Sz;      ///< average size bouncing car
private:
};
//----------------------------------------------------------------------------------------
#endif // TPORTAL_H
