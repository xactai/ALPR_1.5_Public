#include <iostream>
#include <ctime>
#include "General.h"
#include "Settings.h"
#include "Email.h"
#include "Pid.h"
//---------------------------------------------------------------------------
using namespace std;
//---------------------------------------------------------------------------
TEmail      MyEmail;
float       Temp;
float       FPS;
TSettings   MySettings(SETTING_FILE);       //the settings (they are loaded here for the first time)
//---------------------------------------------------------------------------
int main ()
{
    int Ret, Ypid;
    time_t t;

    if(MySettings.Alive){
        //see if YoloCamRpi still lives
        Ypid = getProcIdByName("YoloIPcam");
//        Fpid = getProcIdByName("ffmpeg");
        if(Ypid<0){
            //YoloCamRpi stopped (stream broken??) -> rebooot
            system("sudo reboot");
        }
        else{
            if(MySettings.MailAddress!="" && MySettings.MailAddress!="none"){
                //see if it's 00:00
                time(&t);
                std::tm* now = localtime(&t);
                if(now->tm_hour == 0){
                    //send alive mail
                    Ret = MyEmail.Alive("YoloIPcam");
                    if(Ret!=0){
                        //something goes wrong with the email -> rebooot
                        system("sudo reboot");
                    }
                }
            }
        }
    }
}

