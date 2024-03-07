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

//---------------------------------------------------------------------------
// Global hardcoded parameters
//---------------------------------------------------------------------------

#define OK_FILE      "/dev/shm/YoloCamStatus.txt"
#define SETTING_FILE "settings.txt"

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
struct TObject
{
    cv::Rect_<float> rect;
    int label;
    float prob {0.0};
    float area {0.0};       //percentage of total image
    float diff {0.0};       //percentage of movement
    short email {0};        //0=no mail | 1=mail prio 1 | 2=mail prio 2 (higher) | etc
    bool active {false};    //object is an event
    //used by YoloFastestV2
    int x1 {0};
    int y1 {0};
    int x2 {0};
    int y2 {0};
    float GetArea() { return (((float)(x2 - x1))*((float)(y2 - y1))); };
};
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
