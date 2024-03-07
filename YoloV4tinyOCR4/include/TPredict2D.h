#ifndef TPREDICT2D_H
#define TPREDICT2D_H
//----------------------------------------------------------------------------------------
struct TOrinVec                                     ///< Vector with free origin
{
    int      X {0};                                 ///< X location origin
    int      Y {0};                                 ///< Y location origin
    float    Velocity {0.0};                        ///< speed of movement
    float    Angle {0.0};                           ///< direction of movement

    inline void operator=(const TOrinVec T1){
        X=T1.X;  Y=T1.Y; Velocity=T1.Velocity; Angle=T1.Angle;
    }

    float Euclidean(TOrinVec Vec)
    {
        return sqrt(((X)-(Vec.X))*((X)-(Vec.X))+((Y)-(Vec.Y))*((Y)-(Vec.Y)));
    }
};
//----------------------------------------------------------------------------------------
template<class Tp> struct TPredict2D
{
public:
    TPredict2D()
    {
        Reset();
    }

    virtual ~TPredict2D(){ ; }

    void operator=(const TPredict2D<Tp> P1)
    {
        for(int i=0;i<32;i++){ Xarr[i]=P1.Xarr[i]; Yarr[i]=P1.Yarr[i]; }
        Len=P1.Len; Cnt=P1.Cnt;
    }

    TOrinVec Reset(const Tp X=0.0, const Tp Y=0.0)
    {
        TOrinVec Vec;

        Vec.X=X;  Vec.Y=Y;
        for(int i=0;i<32;i++){ Xarr[i]=X; Yarr[i]=Y; }
        Len=Cnt=0;

        return Vec;                 //returns the new prediction
    }

    TOrinVec Feed(const Tp X, const Tp Y)
    {
        TOrinVec Vec;
        float Ax,Bx,Ay,By;

        Vec.X    =X;  Vec.Y    =Y;
        Xarr[Len]=X;  Yarr[Len]=Y;
        Len++; Len&=0x01F;           //Len MOD 32
        Cnt++; if(Cnt>32) Cnt=32;

        Predict(Ax,Bx,Ay,By);

        Vec.Velocity = sqrt(Ax*Ax+Ay*Ay);
        if(Ax==0.0 && Ay==0.0) Vec.Angle = 0.0;
        else                   Vec.Angle = 180*atan2(Ay,Ax)/M_PI;

        return Vec;                 //returns the new prediction
    }

    void Predict(float &Ax,float &Bx, float &Ay, float &By)
    {
        int i,p;
        float ysum,xysum,xsum,x2sum;

        if(Cnt>1){
            for(xsum=x2sum=ysum=xysum=0.0, i=0;i<Cnt;i++){
                p = (Len-Cnt+i) & 0x1F;
                xsum  += i;
                x2sum += i*i;
                ysum  += Xarr[p];
                xysum += i*Xarr[p];
            }
            Ax=(Cnt*xysum-xsum*ysum)/(Cnt*x2sum-xsum*xsum);
            Bx=(ysum-Ax*xsum)/Cnt;
        }
        else{ Ax=0; Bx=Xarr[0]; }

        if(Cnt>1){
            for(xsum=x2sum=ysum=xysum=0.0, i=0;i<Cnt;i++){
                p = (Len-Cnt+i) & 0x1F;
                xsum  += i;
                x2sum += i*i;
                ysum  += Yarr[p];
                xysum += i*Yarr[p];
            }
            Ay=(Cnt*xysum-xsum*ysum)/(Cnt*x2sum-xsum*xsum);
            By=(ysum-Ay*xsum)/Cnt;
        }
        else{ Ay=0; By=Xarr[0]; }
    }

    void Predict(Tp &X, Tp &Y, int Pos=0)
    {
        float Ax,Bx,Ay,By;

        Predict(Ax,Bx,Ay,By);

        X=static_cast<Tp>((Cnt+Pos)*Ax+Bx);
        Y=static_cast<Tp>((Cnt+Pos)*Ay+By);
    }

    float Vector(void)
    {
        float Ax,Bx,Ay,By;

        Predict(Ax,Bx,Ay,By);

        return sqrt(Ax*Ax+Ay*Ay);
    }

    float Angle(void)
    {
        float Ax,Bx,Ay,By;

        Predict(Ax,Bx,Ay,By);
        if(Ax==0.0 && Ay==0.0) return 0.0;

        return atan2(Ay,Ax);
    }

    Tp Euclidean(const Tp X, const Tp Y)
    {
        Tp Px, Py;

        Predict(Px,Py);
        return static_cast<Tp>(sqrt((X-Px)*(X-Px)+(Y-Py)*(Y-Py)));
    }
protected:
private:
    Tp Xarr[32];
    Tp Yarr[32];
    int Len, Cnt;
};
//----------------------------------------------------------------------------------------
#endif // TPREDICT2D_H
