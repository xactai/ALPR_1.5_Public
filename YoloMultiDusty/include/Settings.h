#ifndef SETTINGS_H_INCLUDED
#define SETTINGS_H_INCLUDED
#include <sys/types.h>
#include <sys/stat.h>
//CCTV1 = rtsp://192.168.178.129:8554/test/
//CCTV2 = rtsp://192.168.178.112:8554/test/
//CCTV3 = rtsp://192.168.178.96:8554/test/
//CCTV4 = udpsrc port=5200 ! application/x-rtp, media=video, clock-rate=90000, payload=96 ! rtpjpegdepay ! jpegdec ! videoconvert ! appsink
//---------------------------------------------------------------------------
// TSettings (can be altered by the user)
//---------------------------------------------------------------------------
struct TSettings
{
private:
    const char *TFileName;
public:
    //settings
    std::string CCTV1;              //GStreamer pipeline cam 1
    std::string CCTV2;              //GStreamer pipeline cam 2
    std::string CCTV3;              //GStreamer pipeline cam 3
    std::string CCTV4;              //GStreamer pipeline cam 4
    bool        PrintOutput;        //print the found objects in the terminal window
    bool        ShowOutput;         //show the output window
    int         MjpegPort;          //local port sent address (8090)

    //flag
    bool Loaded;

	TSettings(const char *FileName);

	void LoadFromFile(void);
	void SaveToFile(void);
};
//---------------------------------------------------------------------------
#endif // SETTINGS_H_INCLUDED
