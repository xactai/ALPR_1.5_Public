#include "TPortal.h"

using namespace std;

extern Tjson    Js;

//----------------------------------------------------------------------------------------
TPortal::TPortal()
{
    //ctor
}
//----------------------------------------------------------------------------------------
TPortal::~TPortal()
{
    //dtor
}
//----------------------------------------------------------------------------------------
void TPortal::Init(const std::string CamName)
{
    ifstream of;
    float Aavg;

    CamID = CamName;
    Move = (Js.MovingPortal)? 0 : 12;       //forced move (no portal file found) gives 12 entries, then it blocks

    //load from the location
    of.open(Js.Pstr+CamID);
    if(of.is_open()){
        of >> Border.A.X;
        of >> Border.A.Y;
        of >> Border.B.X;
        of >> Border.B.Y;
        of >> Aavg;
        of.close();
        if(Border.B.X>0 && Border.B.X<Js.WorkWidth && Border.B.Y>0 && Border.B.Y<Js.WorkHeight){
            //fill the empty moving average
            //now a single add is weighted averaged.
            for(int i=0;i<8;i++){
                Xo.Add(Border.B.X);
                Yo.Add(Border.B.Y);
                An.Add(Aavg);
            }
        }
        else{
            //corrupted file, we must move and override Js.MovingPortal
            Move = 0;
        }
    }
    else{
        //no portal file found, we must move and override Js.MovingPortal
        Move = 0;
    }
}
//----------------------------------------------------------------------------------------
void TPortal::Update(TPredict2D<int> &Vec)
{
    float Ax,Bx,Ay,By;

    Vec.Predict(Ax,Bx,Ay,By);

    //get previous location (32 - 12 ticks = 20)
    if(Js.MovingPortal){
        Xo.Add(20*Ax+Bx);
        Yo.Add(20*Ay+By);
    }
    else{
        if(Move < 12){
            Xo.Add(20*Ax+Bx);
            Yo.Add(20*Ay+By);
            Move++;
        }
    }

    if(Ax==0.0 && Ay==0.0) An.Add(0.0);
    else An.Add(atan2(Ay,Ax));
}
//----------------------------------------------------------------------------------------
TOrinVec TPortal::Location(void)
{
    TOrinVec Vec;

    Vec.X     = Xo.Aver();
    Vec.Y     = Yo.Aver();
    Vec.Angle = An.Aver();

    return Vec;
}
//----------------------------------------------------------------------------------------
void TPortal::CalcBorder(void)
{
    ofstream of;
    float Aavg = An.Aver();

    Border.A.X = cos(Aavg-M_PI/2.0);     //aver angle rotate -90°
    Border.A.Y = sin(Aavg-M_PI/2.0);
    Border.B.X = Xo.Aver();
    Border.B.Y = Yo.Aver();

    //save the location
    of.open(Js.Pstr+CamID);
    if(of.is_open()){
        of << Border.A.X << endl;
        of << Border.A.Y << endl;
        of << Border.B.X << endl;
        of << Border.B.Y << endl;
        of << Aavg << endl;
        of.close();
    }
}
//----------------------------------------------------------------------------------------
DPoint TPortal::Predict(TPredict2D<int> &Vec,float &Dt,int &Tm,bool &In)
{
    int x,y;
    float Ax,Bx,Ay,By;
    TParaLine L1;
    DPoint P1, P2, Pnt;
    DPoint A, B, C;

    Vec.Predict(Ax,Bx,Ay,By);

    P1.X=Bx;     P1.Y=By;
    P2.X=Ax+Bx;  P2.Y=Ay+By;
    L1.MakeLine(P1,P2);

    Border.Intersection(L1,Pnt);

    if((Pnt.X<1E5) && (Pnt.X>-1E5) && (Pnt.Y<1E5) && (Pnt.Y>-1E5)){
        Dt=Pnt.Length(Border.B);     //Border.B holds average (Xo.Aver(), Yo.Aver())
        Tm=L1.GetT(Pnt.X);
    }
    else Dt=1E5;

    //get some points on the border line
    Border.Get(-10,A); Border.Get( 10,B);

    //get the current location of the vehicle
    Vec.Predict(x,y);

    //get the cross product
    In = ((B.X - A.X)*(y - A.Y) - (B.Y - A.Y)*(x - A.X) <= 0.0);

    return Pnt;
}
//----------------------------------------------------------------------------------------
