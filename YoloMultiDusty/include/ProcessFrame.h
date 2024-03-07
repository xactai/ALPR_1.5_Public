#ifndef PROCESSFRAME_H
#define PROCESSFRAME_H

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include "yolo-fastestv2.h"
#include "CamCluster.h"
//----------------------------------------------------------------------------------
class TProcessFrame
{
private:
    int WorkingOnFrameNr;
    TCamCluster *MyCams;
    void ProcessDNN(cv::Mat& frame);
    void DrawObjects(cv::Mat& frame, const std::vector<TargetBox>& boxes);
protected:
public:
    TProcessFrame(TCamCluster *CamClst);
    virtual ~TProcessFrame(void);
    void ExecuteFrame(const int FrmNr);
};
//----------------------------------------------------------------------------------
#endif // PROCESSFRAME_H

