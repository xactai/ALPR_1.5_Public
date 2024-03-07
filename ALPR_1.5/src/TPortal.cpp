#include "TPortal.h"

using namespace std;

extern Tjson    Js;

//----------------------------------------------------------------------------------------
TPortal::TPortal():FileName("")
{
    //ctor
    Freeze=0;
    FileName="";
    for(int i=0;i<8;i++){
        Xo.Add(0);
        Yo.Add(0);
        An.Add(0.0);
        Sz.Add(0);
    }
    An.Len=0;
}
//----------------------------------------------------------------------------------------
TPortal::~TPortal()
{
    Save();
}
//----------------------------------------------------------------------------------------
void TPortal::Init(const std::string CamName,const int Width)
{
    Length      = Width/2;
    CamStr      = CamName;
    FileName    = Js.Pstr+"_"+CamName;

    Load();
}
//----------------------------------------------------------------------------------------
void TPortal::Load(void)
{
    ifstream of;
    float Aavg;
    int Size;

    if(FileName!=""){
        of.open(FileName);
        if(of.is_open()){
            try{
                of >> Border.A.X;
                of >> Border.A.Y;
                of >> Border.B.X;
                of >> Border.B.Y;
                of >> Aavg;
                of >> Size;
                of >> Freeze;
                of.close();
                if(Border.B.X>0 && Border.B.X<Js.WorkWidth && Border.B.Y>0 && Border.B.Y<Js.WorkHeight){
                    //fill the empty moving average
                    //now a single add is weighted averaged.
                    for(int i=0;i<8;i++){
                        Xo.Add(Border.B.X);
                        Yo.Add(Border.B.Y);
                        An.Add(Aavg);
                        Sz.Add(Size);
                    }
                }
                else{
                    //corrupted file, we must move and override Js.MovingPortal
                    PLOG_ERROR << "Portal file: "<< FileName << " Borders corrupted, loading defaults";
                    Freeze = 0;
                }
            }
            catch( ... ){
                //corrupted file, we must move and override Js.MovingPortal
                PLOG_ERROR << "Portal file: "<< FileName << " corrupted, loading defaults";
                Freeze = 0;
            }
        }
        else{
            //no portal file found, we must move and override Js.MovingPortal
            PLOG_ERROR << "Portal file: "<< FileName << " missing, loading defaults";
            Freeze = 0;
        }
    }

    if(Js.PortalFreeze==0){
        cout << "Camera "<< CamStr << " : Portal is flexible" << endl << endl;
    }
    else{
        if(Js.PortalFreeze<=16){
            if(Freeze <= 16){
                cout << "Camera "<< CamStr << " : Portal is closing. Still " << 16-Freeze << " cars to go"<< endl << endl;
            }
            else{
                cout << "Camera "<< CamStr << " : Portal is fixed" << endl << endl;
            }
        }
        if(Js.PortalFreeze>16){
            if(Js.PortalFreeze<=Freeze) cout << "Camera "<< CamStr << " : Portal is fixed" << endl;
            else cout << "Camera "<< CamStr << " : Portal is closing. Still " << Js.PortalFreeze-Freeze << " cars to go"<< endl << endl;
        }
    }
}
//----------------------------------------------------------------------------------------
void TPortal::Save(void)
{
    ofstream of;
    float Aavg = An.Aver();
    int Size = Sz.Aver();

    if(Js.PortalFreeze==0) Freeze=0;

    if(FileName!=""){
        of.open(FileName);
        if(of.is_open()){
            of << Border.A.X << endl;
            of << Border.A.Y << endl;
            of << Border.B.X << endl;
            of << Border.B.Y << endl;
            of << Aavg << endl;
            of << Size << endl;
            of << Freeze << endl;
            of.close();
        }
        else PLOG_ERROR << "Can't save portal file: " << FileName;
    }
    else PLOG_ERROR << "Can't save portal file, missing name";
}
//----------------------------------------------------------------------------------------
void TPortal::Update(TMover &Mov)
{
    float Af,Fz,Ax,Bx,Ay,By,Aavg,AnD;

//    if((100*Mov.Sz)<(60*Sz.Aver()) || !Mov.Bounced ) cout << "  - too small" << endl;
//    else cout << "  - ok" <<endl;

    if(!Mov.Bounced || ((100*Mov.Sz)<(60*Sz.Aver()))) return;

    if(An.Len>5){                                   //after 5 cars we have a solid indication of the angle
        AnD=Mov.Angle-An.Aver();
        if(AnD<-0.7854 || AnD>0.7854) return;       //Angle must be between -45 and 45
    }

    Sz.Add(Mov.Sz);

    Mov.PredictPortal(Ax,Bx,Ay,By,1); //here you use the location and velocity just before bouncing

//    cout << "Bounced : " << Mov.Bounced << endl;
//    cout << "Located : " << Mov.X     << " - " << Mov.Y     << " - An : " << (180*Mov.Angle)/M_PI << " - Sz : "<< Mov.Sz << endl;
//    cout << "Average : " << Xo.Aver() << " - " << Yo.Aver() << " - An : " << (180*An.Aver())/M_PI << " - Sz : " << Sz.Aver() << endl;
//    cout << "Predict : " << Bx << " - " << By << endl;

    if(Ax==0.0 && Ay==0.0) Af=0.0;
    else Af=atan2(Ay,Ax);

    if(Js.PortalFreeze==0){
        Xo.Add(Bx); Yo.Add(By); An.Add(Af); Freeze = 0; Fz=1.0;
    }
    if(Js.PortalFreeze<=16){
        if(Freeze <= 16){
            Xo.Add(Bx); Yo.Add(By); An.Add(Af); Freeze++; Fz=1.0;
        }
        else Fz=0.0;
    }
    if(Js.PortalFreeze>16){
        if(Freeze <= 16){
            Xo.Add(Bx); Yo.Add(By); An.Add(Af); Freeze++; Fz=1.0;
        }
        else{
            Fz=1.0-(((float)Freeze-16)/(Js.PortalFreeze-16));
            if(Fz>0.0 && Fz<=1.0){
                Xo.Add(Fz*Bx+(1.0-Fz)*Xo.Aver());
                Yo.Add(Fz*By+(1.0-Fz)*Yo.Aver());
                An.Add(Fz*Af+(1.0-Fz)*An.Aver());
            }
        }
        if(Fz> 0.0) Freeze++;
    }

    PLOG_DEBUG << "Update portal : " << Js.PortalFreeze << " | count : " << Freeze << " | ratio : " << Fz << endl;

    //update location
    Aavg = An.Aver();

    Border.A.X = cos(Aavg-M_PI/2.0);     //aver angle rotate -90°
    Border.A.Y = sin(Aavg-M_PI/2.0);
    Border.B.X = Xo.Aver();
    Border.B.Y = Yo.Aver();

    Save();
}
//----------------------------------------------------------------------------------------
DPoint TPortal::Predict(TMover &Mov,bool &In)
{
    int x,y;
    float Ln,Ax,Bx,Ay,By;
    TParaLine L1;
    DPoint P1, P2, Pnt;
    DPoint A, B, C;

    Mov.Predict(Ax,Bx,Ay,By);

    P1.X=Bx;     P1.Y=By;
    P2.X=Ax+Bx;  P2.Y=Ay+By;
    L1.MakeLine(P1,P2);

    Border.Intersection(L1,Pnt);

    //get some points on the border line
    Border.Get(-10,A); Border.Get( 10,B);

    //get the current location of the vehicle
    Mov.Predict(x,y);

    Ln=Border.GetT(Pnt.X);
    if(fabs(Ln)<=Length){
        //get the cross product
        In = ((B.X - A.X)*(y - A.Y) - (B.Y - A.Y)*(x - A.X) <= 0.0);
    }
    else In=true;

    return Pnt;
}
//----------------------------------------------------------------------------------------
