#include "General.h"
#include "Settings.h"

using namespace std;
//---------------------------------------------------------------------------
const char* class_names[] = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
    "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"
};
//---------------------------------------------------------------------------
// TSettings
//---------------------------------------------------------------------------
TSettings::TSettings(const char *FileName) : TFileName(NULL),
                                             CCTV1(""),
                                             CCTV2(""),
                                             CCTV3(""),
                                             CCTV4(""),
                                             PrintOutput(true),
                                             ShowOutput(true),
                                             MjpegPort(0)
{
    TFileName=FileName;
    Loaded=false;
    LoadFromFile();
    if(Loaded) SaveToFile();  //new items not written in the file are added now
}
//---------------------------------------------------------------------------
void TSettings::LoadFromFile(void)
{
    int p,t,n;
    ifstream myfile;
    string line,str_value;

    myfile.open(TFileName);
    if (myfile.is_open()) {
        n=0;
        while( getline(myfile,line) ){
            p=line.find('=');
            if(p>0){
                str_value=trim_copy(line.substr(p+1));     // get from '='+1 to the end and trim spaces
                t=line.find("CCTV1");           if(t>=0) CCTV1=str_value;
                t=line.find("CCTV2");           if(t>=0) CCTV2=str_value;
                t=line.find("CCTV3");           if(t>=0) CCTV3=str_value;
                t=line.find("CCTV4");           if(t>=0) CCTV4=str_value;
                t=line.find("print_out");       if(t>=0) PrintOutput=stoi(str_value);
                t=line.find("show_out");        if(t>=0) ShowOutput=stoi(str_value);
                t=line.find("mjpeg_port");      if(t>=0) MjpegPort=stof(str_value);
                n++;
            }
        }
        myfile.close();
        Loaded=true;
    }
    else cout<< "Error : Reading settings file : " << TFileName <<endl;
}
//---------------------------------------------------------------------------
void TSettings::SaveToFile(void)
{
    ofstream myfile;

    myfile.open(TFileName);
    if(myfile.is_open()){
        myfile << "[Section1]" << '\n';
        myfile << "CCTV1 = " << CCTV1 << '\n';
        myfile << "CCTV2 = " << CCTV2 << '\n';
        myfile << "CCTV3 = " << CCTV3 << '\n';
        myfile << "CCTV4 = " << CCTV4 << '\n';
        myfile << "print_out = " << PrintOutput << '\n';
        myfile << "show_out = " << ShowOutput << '\n';
        myfile << "mjpeg_port = " << MjpegPort << '\n';

        myfile.close();
    }
    else cout<< "Error : Writing settings file : " << TFileName <<endl;
}
//---------------------------------------------------------------------------
