#ifndef TCAMCLUSTER_H
#define TCAMCLUSTER_H

#include "ThreadCam.h"
//----------------------------------------------------------------------------------
class TCamCluster
{
friend class TProcessFrame;
private:
    cv::Mat MatEmpty;
    void InitCamArray(int NewSize);
    void DeleteCamArray();
protected:
    int  Width;                                     //widht and height of the frame we process,
    int  Height;                                    //need not be the camera resolution (smaller will speed up calcus)
    int  Count;                                     //number of cameras (size of CamGrb*[])
    ThreadCam **CamGrb;
    cv::Mat* MatAr;                                 //array of frames
public:
    TCamCluster(int Width, int Height);             //wanted Width and Height (Camera ratio will overrule width or height)
    virtual ~TCamCluster();
    void Start(std::vector< std::string > Devs);    //start running the camera threads
    bool GetFrame(const int DevNr);                 //returns false when no frame is available.
    void GetComposedView(cv::Mat& m);               //returns all frames in one large picture
};
//----------------------------------------------------------------------------------
#endif // TCAMCLUSTER_H
