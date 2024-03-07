#ifndef TMAPPER_H
#define TMAPPER_H

#include "General.h"
#include "TOCR.h"
#include "TPortal.h"
#include "ProcessPipe.h"
#include "TimeKeeper.h"
#include "TAlbum.h"
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
//----------------------------------------------------------------------------------------
class TSnapShot;  // Forward declaration
//----------------------------------------------------------------------------------------
class TMapper : public TProcessPipe
{
friend class TSnapShot;
private:
    double  Warp;                       ///< Warp coefficient (birds view)
    double  Gain;                       ///< Gain coefficient (birds view)
    int     PortalWidth;                ///< Width of the portal (+/- 0.5*width around center)
    int     Trigger;                    ///< Trigger type bits | portal (0x16) | left (0x8) | top (0x4) | right (0x2) | bottom (0x1) |
    double  MinTwoSec;                  ///< Minimal valid time of 2 wheeler to be investigated
    double  MinFourSec;                 ///< Minimal valid time of 4 wheeler to be investigated
    int     MinSizePic;                 ///< Minimal size of a four-wheeler in static mode (picture or folder)
    cv::Rect BoIRect;                   ///< BandOfInterest roi used to trigger LPD and OCR algoritms
    double   BoISpeed;                  ///< Max velocity inside BoI
private:
    bool  CheckBoI(TMapObject& Mo);
    float DoWarp(float Y);
    float InvWarp(float X);
    void  FillMQTTevent(TMapObject& Mo);
    void  FillColor(TMapObject& Mo);
    bool  SendJsonHTTP(std::string send_json,int port = 8070, int timeout = 400000);
protected:
    TAlbum      PlateAlbum;             ///< Plate album can hold up to ALBUM_SIZE vehicles with possible license plates
    TTimeKeeper StreamTime;             ///< Keep track of the local stream time
    std::vector<TMapObject> MapList;    ///< Objects currently in the scene
    std::string MQTTevent;              ///< MQTT event string
    std::string TopicName;
    int         MJpegPort;
protected:
    void  MapStatic(void);
    void  MapDynamic(void);
    void  CleanList(void);
    void  CheckInspection(void);
    void  UpdateAlbum(TMapObject& Mo);
    void  CheckBounced(TMapObject& Mo);
    void  SetInpectionState(TMapObject& Mo);
    float GetBoxDistance(TMapObject& Mo,const cv::Point& Pt);
    void  PlotBirdView(void);
    void  DrawObjects(void);
    void  DrawObjectsLabels(void);
public:
    TPortal Portal;
public:
    TMapper();
    virtual ~TMapper();

    void Init(const int Nr);
    void Execute(void);
};
//----------------------------------------------------------------------------------------
#endif // TMAPPER_H
