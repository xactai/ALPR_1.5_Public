#ifndef YOLOIPCAMLIB_H
#define YOLOIPCAMLIB_H

#include "General.h"
#include <opencv2/opencv.hpp>
//---------------------------------------------------------------------------
bool InitYOLO(void);
int RunYOLO(const int ID, const cv::Mat& bgr, std::vector<TObject>& objects, const bool Fast=false);   //nr objects. -1=error see /dev/shm/key.txt
//---------------------------------------------------------------------------
#endif //YOLOIPCAMLIB
