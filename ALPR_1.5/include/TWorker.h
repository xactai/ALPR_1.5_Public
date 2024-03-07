#ifndef TWorker_H
#define TWorker_H

#include "General.h"
#include "TOCR.h"
//----------------------------------------------------------------------------------------
struct TCar
{
public:
    cv::Mat      Picture;           ///< image of the vehicle (not the plate!)
    cv::Rect     Prect;             ///< plate rectangle of the vehicle
    unsigned int TrackID;           ///< tracking number of the plate
    float        Fuzzy {0.0};       ///< output weighted calcus better plate (0.0 indicates no plate found)
    char         OCR[32];           ///< OCR output of the vehicle
    float        Prob {0.0};        ///< OCR Probability
    slong64      Tick {0};          ///< Get the age of the license (used to delete the oldest from the list, if needed)
};
//----------------------------------------------------------------------------------
inline bool compareByFuzzy(const TCar* a, const TCar* b) { return a->Fuzzy > b->Fuzzy; }
//----------------------------------------------------------------------------------
inline void sortCarPointersByFuzzy(TCar** array, size_t size) { std::sort(array, array + size, compareByFuzzy); }
//----------------------------------------------------------------------------------
class TWorker
{
friend class TAlbum;
private:
    TCar** Cars;
    TOCR Ocr;
protected:
    void GetLicense(TMapObject &Mo, int p);
public:
    int BestPlate;
    TWorker(void);
    virtual ~TWorker();

    void Update(TMapObject &Mo,cv::Mat &Img);
    void Execute(TMapObject &Mo);
};
//----------------------------------------------------------------------------------
#endif // TWorker_H
