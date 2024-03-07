#ifndef TMAPPER_H
#define TMAPPER_H

#include <opencv2/opencv.hpp>
#include "General.h"
#include "TOCR.h"
#include "TPortal.h"
//---------------------------------------------------------------------------
/// We need extra information about an object, hence we inherit bbox_t.\n
/// TObject 'lives' only one frame.

/// ******************************************************************************
/// DO NOT INCLUDE CV::MATs TO THE TMapObject STRUCTURE !!!
/// A std::vector holds the TMapObject.
/// Dynamic reallocating memory can corrupt the cv::Mat.
/// It has to do with the unknown size of the cv::Mat at initialization.
/// At that time you just allocate a pointer with an empty picture (size=0).
/// ******************************************************************************

//do not add dynamic objects to the list!!
//like cv::mat or std::string
struct TMapObject : public bbox_t
{
    //administration
    char     CarChr;                                ///< Used in file name to indicate the car
    char     Name[256];                             ///< File name of the saved vehicle picture
    char     PlateName[256];                        ///< File name of the saved plate picture
    char     PlateOCR[256];                         ///< Found OCR (in json "FRAME_OCR_ON" must be true)
    //status
    bool     Valid {false};                         ///< object has the correct size (investigate further)
    int      ValidTicks {0};                        ///< duration of valid=true status (not only a single frame valid)
    //location
    TOrinVec Loc;                                   ///< current location, speed and angle
    int      Size {0};                              ///< ROI size
    TPredict2D<int>  Predict;                       ///< predict the movement
    //leaving the scene
    float    ArrivalDist;                           ///< predicted distance between center border and crossing point
    int      ArrivalTime;                           ///< expected time to arrive (in ticks)
    bool     Inside {true};                         ///< inside or outside portal?
    bool     Inside_t1 {true};                      ///< inside or outside portal a tick -1?
    //time
    size_t   Tick {0};                              ///< time ticks
    std::chrono::steady_clock::time_point Tstart;   ///< time stamp first seen
    std::chrono::steady_clock::time_point Tend;     ///< time stamp last seen
    //inspection
    std::chrono::steady_clock::time_point TFinp;    ///< time vehicle is first inspected
    std::chrono::steady_clock::time_point TLinp;    ///< time vehicle is last inspected
    int Person_ID {-1};                             ///< tracking number of person inspecting the vehicle
    bool Inspected {false};                         ///< is the vehicle inspected?
    //license
    TOcrPoll PlatePoll;
    float    PlateFuzzy {0.0};                      ///< output weighted calcus better plate (0.0 indicates no plate found)
    float    PlateEdge;                             ///< ratio of edges
    cv::Rect PlatePlot;                             ///< location of a possible plate (used for drawing)
    cv::Rect PlateRect;                             ///< location of the best plate

    inline void operator=(const bbox_t T1){
        x=T1.x;  y=T1.y; w=T1.w; h=T1.h; prob=T1.prob; obj_id=T1.obj_id;
        track_id=T1.track_id; frames_counter=T1.frames_counter;
    }
    inline void operator=(const TMapObject T1){
        x=T1.x;  y=T1.y; w=T1.w; h=T1.h; prob=T1.prob; obj_id=T1.obj_id;
        track_id=T1.track_id; frames_counter=T1.frames_counter;

        Loc=T1.Loc;  Tick=T1.Tick; Tstart=T1.Tstart; Inside=T1.Inside;
        Tend=T1.Tend; Size=T1.Size; Predict=T1.Predict; Valid=T1.Valid;
        ValidTicks=T1.ValidTicks; ArrivalDist=T1.ArrivalDist; ArrivalTime=T1.ArrivalTime;
        PlateRect=T1.PlateRect; PlateEdge=T1.PlateEdge; Inside_t1=T1.Inside_t1;
        PlateFuzzy=T1.PlateFuzzy; PlatePlot=T1.PlatePlot;
        memcpy(Name,T1.Name,256); CarChr=T1.CarChr;
        memcpy(PlateName,T1.PlateName,256);
        memcpy(PlateOCR,T1.PlateOCR,256);
        PlatePoll=T1.PlatePoll; TFinp=T1.TFinp; TLinp=T1.TLinp;
        Person_ID=T1.Person_ID; Inspected=T1.Inspected;
    }
};
//----------------------------------------------------------------------------------------
class TMapper
{
friend class TSnapShot;
friend class TProcessPipe;
public:
    TMapper();
    virtual ~TMapper();

    void Init(const int ID,const int Width,const int Height,const double Warp,
              const double Gain,const std::string CamName,const cv::Rect DnnRect);
    void Update(cv::Mat& frame, std::vector<bbox_t>& box);
    bool GetInfo(const bbox_t box,bool &Valid,cv::Rect& rt);
    TPortal Portal;
protected:
    void CheckLicense(cv::Mat& frame, TMapObject& Mo);
    void PlotDebug(void);
    void CleanList(void);
    void CheckInspection(void);
    std::vector<TMapObject> MapList;        ///< Objects currently in the scene
private:
    int Wd, Ht, ID;
    double Wp, Gn;
    std::string CamID;
    cv::Rect DnnRect;
    float Warp(float Y);
    float InvWarp(float X);
    cv::Mat MapMat;
    //please note, only one plate at the time
    unsigned int PlateObjectID;             ///< object number of the plate
    unsigned int PlateTrackID;              ///< tracking number of the plate
    float   PlateQuality;                   ///< readability of the plate
    cv::Mat Plate;                          ///< image of the plate
    std::string PlateOCR;                   ///< OCR best result of the plate (if found)
};
//----------------------------------------------------------------------------------------
#endif // TMAPPER_H
