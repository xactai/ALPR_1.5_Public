#ifndef TPORTAL_H
#define TPORTAL_H
#include "General.h"
#include "Statistic.h"
//----------------------------------------------------------------------------------------
class TPortal
{
public:
    TPortal();
    virtual ~TPortal();

    void Init(const std::string CamName);       ///< Load portal from file
    void Update(TPredict2D<int> &Vec);          ///< Update the moving average
    TOrinVec Location(void);
    void CalcBorder(void);
    DPoint Predict(TPredict2D<int> &Vec,float &Dt,int &Tm,bool &In); ///< Predict where and when the vehicle crosses the border

    TParaLine Border;
protected:
    int Move;
private:
    std::string CamID;
    TMoveAver<int> Xo;      ///< X position storage
    TMoveAver<int> Yo;      ///< Y position storage
    TMoveAver<float> An;    ///< angle storage
};
//----------------------------------------------------------------------------------------
#endif // TPORTAL_H
