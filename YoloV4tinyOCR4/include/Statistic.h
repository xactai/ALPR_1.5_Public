//---------------------------------------------------------------------------
#ifndef StatisticH
#define StatisticH

#define AVERAGE 8  //depth of array
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
//---------------------------------------------------------------------------
#endif

