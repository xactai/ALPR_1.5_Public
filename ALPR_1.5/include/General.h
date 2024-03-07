#ifndef GENERAL_H_INCLUDED
#define GENERAL_H_INCLUDED

#include <cstdint>
#include <cmath>
#include <deque>
#include <list>
#include <array>
#include <memory>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <string.h>
#include <istream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <ctime>
#include <cctype>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <thread>
#include <atomic>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include "yolo_v2_class.hpp"
#include "Tjson.h"
#include "Numbers.h"
#include "Locate.h"
#include <plog/Log.h>
#include <plog/Initializers/ConsoleInitializer.h>
//---------------------------------------------------------------------------
/**
    Global used parameters, structures and macros
*/
//---------------------------------------------------------------------------
// Global semi-hardcoded parameters
//
// The values below are default, used when not given in the config.json
//---------------------------------------------------------------------------

#define WORK_WIDTH  640     ///< Default WORK_HEIGHT when not defined in the JSON settings
#define WORK_HEIGHT 480     ///< Default WORK_HEIGHT when not defined in the JSON settings
#define THUMB_WIDTH  640    ///< Default THUMB_WIDTH when not defined in the JSON settings
#define THUMB_HEIGHT 480    ///< Default THUMB_HEIGHT when not defined in the JSON settings

#define ALBUM_SIZE 8        ///< Max different vehicles per frame to analyze
#define WORKER_SIZE 8       ///< Max vehicle pictures to analyze for one license

//---------------------------------------------------------------------------
// Global hardcoded parameters
//---------------------------------------------------------------------------
// LERP(a,b,c) = linear interpolation macro, is 'a' when c == 0.0 and 'b' when c == 1.0 */
#define MIN(a,b)  ((a) > (b) ? (b) : (a))               ///< Returns minimum value of a or b
#define MAX(a,b)  ((a) < (b) ? (b) : (a))               ///< Returns maximum value of a or b
#define LIM(a,b,c) (((a)>(c))?(c):((a)<(b))?(b):(a))    ///< Limits the value of a between b and c
#define LERP(a,b,c) (((b) - (a)) * (c) + (a))           ///< Linear interpolation returns 'a' when c == 0.0 and 'b' when c == 1.0
#define ROUND(a) (static_cast<int>((a)+0.5))            ///< Round a float to the nearest integer value
#define EUCLIDEAN(x1,y1,x2,y2) sqrt(((x1)-(x2))*((x1)-(x2))+((y1)-(y2))*((y1)-(y2)))    ///< Returns the Euclidean distance between two points.

// labels IDs
#define PERSON 0                ///< ID of a person
#define BIKE 1                  ///< ID of a two-wheeler
#define CAR 2                   ///< ID of a four-wheeler

//trigger IDs
#define TRIG_PORTAL 0x10        ///< Portal triggering is used
#define TRIG_LEFT   0x08        ///< Wall triggering left is active
#define TRIG_TOP    0x04        ///< Wall triggering top is active
#define TRIG_RIGHT  0x02        ///< Wall triggering right is active
#define TRIG_BOTTOM 0x01        ///< Wall triggering bottom is active
#define WALL_MARGE  30          ///< Fires a trigger when object is within this marge of a active wall

typedef signed long long int slong64;       ///< Larges possible integer.<br> At least 64 bits
//----------------------------------------------------------------------------------
enum TPipeSource {
    USE_NONE = -1,
    USE_PICTURE,
    USE_FOLDER,
    USE_CAMERA,
    USE_VIDEO
};
//----------------------------------------------------------------------------------
struct TCamInfo                         //camera info
{
public:
    int         CamWidth {0};
    int         CamHeight {0};
    int64_t     CamFrames {0};
    int         CamPixels;              //misusing int for enum AVPixelFormat (otherwise you need always the complete FFmpeg lib)
    double      CamFPS {0.0};           //stream frame rate
    bool        CamSync {true};         //synchronize (movie)
    bool        CamLoop {false};        //loop mode (picture and folder)
    int         CamIndex {0};           //index number in the ProcessPipe loop (and JSON settings file)
    TPipeSource CamSource;              //type of source
    std::string CamCodec;               //stream codec
    std::string Device;                 //the device (can be a pipeline)
    std::string PipeLine;               //the actual used pipeline
    std::string CamName;                //readable name for the pipeline
    std::string FileName;               //static: used FileName | dynamic: used FrameCnt

    inline void operator=(const TCamInfo &i)
    {
        CamWidth =i.CamWidth;
        CamHeight=i.CamHeight;
        CamFrames=i.CamFrames;          //0=folder/movie/CCTV 1=one picture (e.g. ~/Pictures/MyCar.jpg)
        CamPixels=i.CamPixels;
        CamFPS   =i.CamFPS;
        CamSync  =i.CamSync;
        CamLoop  =i.CamLoop;
        CamSource=i.CamSource;
        CamCodec =i.CamCodec;
        Device   =i.Device;
        PipeLine =i.PipeLine;
        CamName  =i.CamName;
        FileName =i.FileName;
    }
};
//---------------------------------------------------------------------------
//do not add dynamic objects to the list!! like cv::mat or std::string
//---------------------------------------------------------------------------
struct TMapObject : public bbox_t
{
    //administration
    char     CamName[256];                          ///< Camera name used when saving pictures. (Identical to TCamInfo::CamName)
    char     CarName[256];                          ///< File name of the saved vehicle picture
    char     PlateName[256];                        ///< File name of the saved plate picture
    char     PlateOCR[32];                          ///< Found OCR.
    int      ColIndex {9};                          ///< Color index of the four wheeler (9 = no color found)
    //status
    bool     Valid {false};                         ///< object has the correct size (investigate further)
    int      ValidTicks {0};                        ///< duration of valid=true status (not only a single frame valid)
    //location
    TOrinVec Loc;                                   ///< current location, speed, angle and size
    int      Size {0};                              ///< ROI size
    TMover   Mov;                                   ///< Average last leg
    //leaving the scene
    bool     InsideBoI {false};                     ///< set when object has been inside BoI
    bool     Inside {true};                         ///< inside or outside portal?
    bool     Inside_t1 {true};                      ///< inside or outside portal a tick-1?
    int      Bounce {0};                            ///< hitting the wall?
    int      Bounce_t1 {0};                         ///< hitting the wall a tick-1?
    bool     MQTTdone {false};                      ///< MQTT string is sent
    //time
    slong64  Tick {0};                              ///< Tick equals FrameCnt (if not, object is vanished)
    slong64  Tstart {0};                            ///< time stamp first seen
    slong64  Tvalid {0};                            ///< time stamp first valid
    slong64  Tend {0};                              ///< time stamp last seen
    slong64  Tcolor {0};                            ///< time stamp color measurement
    slong64  Tplate {0};                            ///< time stamp license plate was stored for OCR
    //inspection
    slong64  TFinp {0};                             ///< time vehicle is first inspected
    slong64  TLinp {0};                             ///< time vehicle is last inspected
    int      InspectState {0};                      ///< state of inspection state machine
    bool     Inspected {false};                     ///< is the vehicle inspected?

    inline void operator=(const bbox_t T1)
    {
        x=T1.x;  y=T1.y; w=T1.w; h=T1.h; prob=T1.prob; obj_id=T1.obj_id;
        track_id=T1.track_id; frames_counter=T1.frames_counter;
    }

    inline void operator=(const TMapObject T1)
    {
        x=T1.x;  y=T1.y; w=T1.w; h=T1.h; prob=T1.prob; obj_id=T1.obj_id;
        track_id=T1.track_id; frames_counter=T1.frames_counter;

        Loc=T1.Loc; Mov=T1.Mov; Tick=T1.Tick; Tstart=T1.Tstart; Inside=T1.Inside;
        Tend=T1.Tend; Size=T1.Size; Valid=T1.Valid; Bounce=T1.Bounce;
        ValidTicks=T1.ValidTicks; ColIndex=T1.ColIndex; /*License=T1.License;*/
        Inside_t1=T1.Inside_t1; Bounce_t1=T1.Bounce_t1; InsideBoI=T1.InsideBoI;
        memcpy(CarName,T1.CarName,256);
        memcpy(CamName,T1.CamName,256);
        memcpy(PlateName,T1.PlateName,256);
        memcpy(PlateOCR,T1.PlateOCR,32);
        TFinp=T1.TFinp; TLinp=T1.TLinp; Tcolor=T1.Tcolor; Tplate=T1.Tplate;
        InspectState=T1.InspectState; Inspected=T1.Inspected; MQTTdone=T1.MQTTdone; Tvalid=T1.Tvalid;
    }

    inline void operator<<(const TMapObject T1)     //merge
    {
        //remember T1 is very young. Just a few ticks old.
        //this (the original) can be old, with filled arrays.
        //do not copy everything from T1. Keep your historical data
        x=T1.x;  y=T1.y; w=T1.w; h=T1.h; /* prob=T1.prob; obj_id=T1.obj_id; */
        track_id=T1.track_id; frames_counter+=T1.frames_counter;

        Loc=T1.Loc;  Mov<<T1.Mov; Tick=T1.Tick; /*Tstart=T1.Tstart;*/ Inside=T1.Inside;
        Tend=T1.Tend; Size=T1.Size; Valid=T1.Valid; Bounce=T1.Bounce; /*License=T1.License;*/
        ValidTicks+=T1.ValidTicks; /*ColIndex=T1.ColIndex; InsideBoI=T1.InsideBoI; */
        Inside_t1=T1.Inside_t1; Bounce_t1=T1.Bounce_t1;
        memcpy(CarName,T1.CarName,256);
        memcpy(CamName,T1.CamName,256);
        memcpy(PlateName,T1.PlateName,256);
        /*memcpy(PlateOCR,T1.PlateOCR,32);*/
        /*TFinp=T1.TFinp;*/ TLinp=T1.TLinp; Tcolor=T1.Tcolor; Tplate=T1.Tplate;
        /*InspectState=T1.InspectState;*/ Inspected=T1.Inspected; MQTTdone=T1.MQTTdone; /*Tvalid=T1.Tvalid;*/
    }
};
//---------------------------------------------------------------------------
struct BGR
{
    uchar b {0};
    uchar g {0};
    uchar r {0};
};
//----------------------------------------------------------------------------------------
struct HSV
{
    uchar h {0};
    uchar s {0};
    uchar v {0};
};
//----------------------------------------------------------------------------------------
struct HSL
{
    uchar h {0};
    uchar s {0};
    uchar l {0};
};
//----------------------------------------------------------------------------------------
inline HSV RGB_HSV(BGR rgb)
{
    HSV hsv;
    unsigned char rgbMin, rgbMax;

    rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
    rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

    hsv.v = rgbMax;
    if(hsv.v == 0) hsv.h = hsv.s = 0; // Achromatic case
    else{
        long delta = rgbMax - rgbMin;

        hsv.s = 255 * delta / hsv.v;
        if(hsv.s == 0) hsv.h = 0;
        else{
            if (rgbMax == rgb.r)
                hsv.h = 0 + 43 * (rgb.g - rgb.b) / delta;
            else if (rgbMax == rgb.g)
                hsv.h = 85 + 43 * (rgb.b - rgb.r) / delta;
            else
                hsv.h = 171 + 43 * (rgb.r - rgb.g) / delta;
        }
    }

    return hsv;
}
//----------------------------------------------------------------------------------------
inline HSL RGB_HSL(BGR rgb)
{
    HSL hsl;
    unsigned char rgbMin, rgbMax;

    rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
    rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

    // Calculate Lightness (L)
    hsl.l = long(rgbMax + rgbMin) / 2;

    if(rgbMax == rgbMin)  hsl.h = hsl.s = 0; // Achromatic case
    else{
        long delta = rgbMax - rgbMin;

        // Calculate Saturation (S)
        if(hsl.l <= 127) hsl.s = (255*delta) / long(rgbMax + rgbMin);
        else             hsl.s = (255*delta) / long(512 - delta);

        // Calculate Hue (H)
        if (rgbMax == rgb.r)
            hsl.h = 0 + 43 * (rgb.g - rgb.b) / delta;
        else if (rgbMax == rgb.g)
            hsl.h = 85 + 43 * (rgb.b - rgb.r) / delta;
        else
            hsl.h = 171 + 43 * (rgb.r - rgb.g) / delta;
    }
    return hsl;
}
//----------------------------------------------------------------------------------------
inline float GetTemp(void)
{
    std::ifstream myfile;
    float temp=0.0;
    std::string line;

    myfile.open("/sys/class/thermal/thermal_zone0/temp");
    if (myfile.is_open()) {
        getline(myfile,line);
        temp = stoi(line)/1000.0;
        myfile.close();
    }
    return temp;
}
//----------------------------------------------------------------------------------------
inline bool MakeDir(const std::string& folder)
{
    bool Success=true;

    if(folder!="none"){
        if(!std::filesystem::exists(folder)){
            try{
                Success=std::filesystem::create_directory(folder);
            }
            catch (const std::filesystem::filesystem_error& e) {
                PLOG_ERROR << "\nError opening folder " << e.what()<< "\n";
            }
        }
    }
    return Success;
}
//---------------------------------------------------------------------------
// to lower case
static inline void lcase(std::string &s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
}
//---------------------------------------------------------------------------
// to lower case (copying)
static inline std::string lcase_copy(std::string s) {
    lcase(s);
    return s;
}
//---------------------------------------------------------------------------
// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}
//---------------------------------------------------------------------------
// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}
//---------------------------------------------------------------------------
// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}
//---------------------------------------------------------------------------
// trim from start (copying)
static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
}
//---------------------------------------------------------------------------
// trim from end (copying)
static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
}
//---------------------------------------------------------------------------
// trim from both ends (copying)
static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
}
//---------------------------------------------------------------------------
#endif // GENERAL_H_INCLUDED
