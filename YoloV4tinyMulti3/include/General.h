#ifndef GENERAL_H_INCLUDED
#define GENERAL_H_INCLUDED

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
#include <thread>
#include <vector>
#include <ctime>
#include <cctype>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include "yolo_v2_class.hpp"

//---------------------------------------------------------------------------
// Global hardcoded parameters
//
// The values below are default, used when not given in the config.json
//---------------------------------------------------------------------------
#define WORK_WIDTH  640
#define WORK_HEIGHT 480
#define THUMB_WIDTH  640
#define THUMB_HEIGHT 480

//---------------------------------------------------------------------------
// Global hardcoded parameters
//---------------------------------------------------------------------------
// LERP(a,b,c) = linear interpolation macro, is 'a' when c == 0.0 and 'b' when c == 1.0 */
#define MIN(a,b)  ((a) > (b) ? (b) : (a))
#define MAX(a,b)  ((a) < (b) ? (b) : (a))
#define LIM(a,b,c) (((a)>(c))?(c):((a)<(b))?(b):(a))
#define LERP(a,b,c) (((b) - (a)) * (c) + (a))
#define ROUND(a) (static_cast<int>((a)+0.5))
//---------------------------------------------------------------------------
struct TObject : public bbox_t
{
    bool     PlateFound {false};
    cv::Rect PlateRect;
    inline void operator=(const bbox_t T1){
        x=T1.x;  y=T1.y; w=T1.w; h=T1.h; prob=T1.prob; obj_id=T1.obj_id;
        track_id=T1.track_id; frames_counter=T1.frames_counter;
    }
};
//----------------------------------------------------------------------------------------
inline int MakeDir(const std::string& folder)
{
    int Success=-1;

    if(folder!="none"){
        std::string Str = "mkdir -p "+folder;
        Success=system(Str.c_str());
    }
    return Success;
}
//---------------------------------------------------------------------------
inline bool FileExists(const std::string& name)
{
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}
//---------------------------------------------------------------------------
inline void FileCopy(const std::string& Src,const std::string& Dst)
{
    std::ifstream  src(Src, std::ios::binary);
    std::ofstream  dst(Dst, std::ios::binary);

    dst << src.rdbuf();
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
