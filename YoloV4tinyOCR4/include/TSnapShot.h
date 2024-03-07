#ifndef TSNAPSHOT_H
#define TSNAPSHOT_H
#include <opencv2/opencv.hpp>
#include "TMapper.h"
/// TSnapShot holds the license plate image inserted in the CCTV footage.\n
/// It will show for 10 secondes after the vehicle is gone.\n
/// Or when a new license plate is detected.
//---------------------------------------------------------------------------
class TSnapShot
{
public:
    TSnapShot();
    virtual ~TSnapShot();
    void Update(TMapper& Map);                      ///< update with new info
    void Show(cv::Mat& frame);                      ///< insert the snapshot in frame
protected:

private:
    unsigned int obj_id;
    unsigned int track_id;
    cv::Mat Snap;
    float Sfuzzy;                                   ///< fuzzy number of plate (update only when greater than previous)
    std::string Mocr;                               ///< OCR result (update only when differs)
    std::chrono::steady_clock::time_point TLast;    ///< the last time an update occurred
    void Refresh(cv::Mat& frame);                   ///< Refresh the Snap image
};
//---------------------------------------------------------------------------
#endif // TSNAPSHOT_H
