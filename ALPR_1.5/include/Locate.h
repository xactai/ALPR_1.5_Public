//---------------------------------------------------------------------------
#ifndef LocateH
#define LocateH

#define AVERAGE 8           //depth of array
#define LEG_LENGTH 80
//---------------------------------------------------------------------------
template<class Tp> struct TMoveAver
{
    Tp Ar[AVERAGE];
    int Pnt, Len;

    TMoveAver() {Pnt=Len=0;}

    inline void Add(const Tp &a)
    {
        if(Len<AVERAGE){
            Ar[Pnt] = a;
            Pnt++; Pnt%=AVERAGE; Len++;
        }
        else{
            Ar[Pnt] = a;
            Pnt++; Pnt%=AVERAGE;
        }
    }
    inline Tp Aver(void)
    {
        if(Len>0){
            Tp Sum=0.0;
            for(int i=0;i<Len;i++) Sum+=Ar[i];
            return Sum/Len;
        }
        else{
            Tp Zero=0.0;
            return Zero;
        }
    }
};
//----------------------------------------------------------------------------------------
struct TOrinVec                                     ///< Vector with free origin
{
    int      X {0};                                 ///< X location origin
    int      Y {0};                                 ///< Y location origin
    float    Velocity {0.0};                        ///< speed of movement
    float    Angle {0.0};                           ///< angle of movement
    int      Sz {0};                                ///< size of object

    inline void operator=(const TOrinVec T1){
        X=T1.X;  Y=T1.Y; Velocity=T1.Velocity; Angle=T1.Angle; Sz=T1.Sz;
    }

    float Euclidean(TOrinVec Vec)
    {
        return sqrt(((X)-(Vec.X))*((X)-(Vec.X))+((Y)-(Vec.Y))*((Y)-(Vec.Y)));
    }
};
//----------------------------------------------------------------------------------------
struct TMover: TOrinVec
{
    TMoveAver<float> Xo;                              ///< X average position storage
    TMoveAver<float> Yo;                              ///< Y average position storage
    TMoveAver<float> Velo;                            ///< Y average speed storage
    int Xt, Yt;
    int X0, X1;
    int Y0, Y1;
    int T0, T1;                                     ///< Number of feeds before PARTS_LENGTH was traveled
    int Bounced;

    inline void operator=(const TMover M1)
    {
        TOrinVec::operator=(M1);
        for(int i=0;i<AVERAGE;i++){
            Xo.Ar[i]=M1.Xo.Ar[i];
            Yo.Ar[i]=M1.Yo.Ar[i];
            Velo.Ar[i]=Velo.Ar[i];
        }
        X0=M1.X0; X1=M1.X1;
        Y0=M1.Y0; Y1=M1.Y1;
        T0=M1.T0; T1=M1.T1;
        Xt=M1.Xt; Yt=M1.Yt;
        Bounced=M1.Bounced;
    }

    inline void operator<<(const TMover M1)         //merge
    {
        TOrinVec::operator=(M1);
        for(int i=0;i<AVERAGE;i++){
            Xo.Ar[i]  =M1.Xo.Ar[i];
            Yo.Ar[i]  =M1.Yo.Ar[i];
            Velo.Ar[i]=Velo.Ar[i];
        }
        X0=M1.X0; X1=M1.X1;
        Y0=M1.Y0; Y1=M1.Y1;
        T0=M1.T0; T1=M1.T1;
        Xt=M1.Xt; Yt=M1.Yt;
        Bounced=M1.Bounced;
    }

    TOrinVec Reset(const int X=0, const int Y=0)
    {
        TOrinVec Vec;

        for(int i=0;i<AVERAGE;i++){
            Xo.Ar[i]=X;
            Yo.Ar[i]=Y;
            Velo.Ar[i]=0.0;
        }
        X0=X; X1=X;
        Y0=Y; Y1=Y;
        T0=0; T1=0;
        Xt=0; Yt=0;
        Bounced =0;

        Vec.X=X;   Vec.Y=Y;
        Vec.Velocity = 0.0;
        Vec.Angle = 0.0;
        Vec.Sz = 0;

        return Vec;
    }

    TOrinVec Feed(const int _X, const int _Y,const int _Sz,const int Bounce, const double CamFPS)
    {
        TOrinVec Vec;
        int x,y;
        float Ax,Ay;

        if(Bounced==0) Bounced=Bounce;        //only one time bouncing

        Xt=Xo.Aver(); Yt=Yo.Aver();
        Xo.Add(_X);   Yo.Add(_Y);
        x=Xo.Aver();  y=Yo.Aver(); T0++;
        if((sqrt((x-X0)*(x-X0)+(y-Y0)*(y-Y0)))>LEG_LENGTH){
            X1=X0; X0=x;
            Y1=Y0; Y0=y;
            T1=T0; T0=0;
        }

        Vec.X=x;  Vec.Y=y; Vec.Sz=_Sz;

        if((T1+T0)>0){
            Vec.Velocity = sqrt((x-X1)*(x-X1)+(y-Y1)*(y-Y1))/(T1+T0);
            Ax=X0-X1; Ay=Y0-Y1;
            if(Ax>=0.0 && Ay==0.0) Vec.Angle=0.000001;
            else Vec.Angle=atan2(Ay,Ax);
        }
        else{
            Vec.Velocity = 0.0;
            Vec.Angle=0.0;
        }
        //adjust for other FPS
        if(CamFPS>0.0){
            Vec.Velocity*=CamFPS/30.0;
        }
        //update the location if not bounced
        if(Bounced==0){
            X=x;  Y=y; Velocity=Vec.Velocity; Angle=Vec.Angle; Sz=_Sz;
        }

        return Vec;                 //returns the new prediction
    }

    void Movement(float &Ax,float &Ay)
    {
        Ax=((float)(X0-X1))/LEG_LENGTH;
        Ay=((float)(Y0-Y1))/LEG_LENGTH;
    }

    void Predict(int &X, int &Y, int Pos=0)
    {
        int x=Xo.Aver();
        int y=Yo.Aver();

        if((T1+T0)>0){
            float velo = sqrt((x-X1)*(x-X1)+(y-Y1)*(y-Y1))/(T1+T0);
            float Ax=((float)(X0-X1))/LEG_LENGTH;
            float Ay=((float)(Y0-Y1))/LEG_LENGTH;
            X=Ax*velo*Pos+x;
            Y=Ay*velo*Pos+y;
        }
        else{
            X=x; Y=y;
        }
    }

    void Predict(float &Ax,float &Bx, float &Ay, float &By)
    {
        int x=Xo.Aver();
        int y=Yo.Aver();

        if((T1+T0)>0){
            float velo = sqrt((x-X1)*(x-X1)+(y-Y1)*(y-Y1))/(T1+T0);
            Ax=((float)(X0-X1)*velo)/LEG_LENGTH;
            Ay=((float)(Y0-Y1)*velo)/LEG_LENGTH;
        }
        else{
            Ax=0.0; Ay=0.0;
        }
        Bx=x;   By=y;
    }

    void PredictPortal(float &Ax,float &Bx, float &Ay, float &By,int Pos=0)
    {
        //use the stored location just before bouncing
        Ax=((float)(X0-X1)*Velocity)/LEG_LENGTH;
        Ay=((float)(Y0-Y1)*Velocity)/LEG_LENGTH;
        Bx=Ax*Velocity*Pos+X;   By=Ay*Velocity*Pos+Y;
    }
};
//----------------------------------------------------------------------------------------
#endif

