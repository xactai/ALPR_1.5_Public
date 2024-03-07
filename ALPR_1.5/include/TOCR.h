#ifndef TOCR_H
#define TOCR_H
#include "General.h"
#include <opencv2/opencv.hpp>

#define PLATE_LIST_SIZE 16
//----------------------------------------------------------------------------------
struct TOcrObject
{
    char  PlateOCR[32];             // Found OCR
    float Prob {0.0};               // Average Prob
    int   Hits {0};                 // Number of identical plates found in time serie

    inline void operator=(const TOcrObject R1){
        memcpy(PlateOCR, R1.PlateOCR, 32);
        Prob = R1.Prob;
        Hits  = R1.Hits;
    }
};
//----------------------------------------------------------------------------------
class TOCR
{
public:
    TOCR();
    virtual ~TOCR();
    float OCR(cv::Mat& frame,char *PlateOCR);

protected:

private:
    void SortPlate(std::vector<bbox_t>& cur_bbox_vec);
    void SortSingleLine(std::vector<bbox_t>& cur_bbox_vec, float ch_wd, float ch_ht, size_t StartPos, size_t& StopPos);
};
//----------------------------------------------------------------------------------
struct TOcrPoll
{
public:
    TOcrPoll();
    virtual ~TOcrPoll();
    void  Init(int NrPlateChars);
    void  Add(std::string PlateOCR, float Prob);
    float GetBestOcr(std::string &PlateOCR);
    void  Clear(void);
    inline void operator=(const TOcrPoll R1){
        for(int i=0;i<PLATE_LIST_SIZE; i++) PlateList[i]=R1.PlateList[i];
        Len = R1.Len;
    }
protected:
private:
    int Len;
    int CharCnt;
    TOcrObject PlateList[PLATE_LIST_SIZE];
};
//----------------------------------------------------------------------------------
#endif // TOCR_H
