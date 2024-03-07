#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using std::cout;

char TrackName1[50];
char TrackName2[50];
const int gain_slider_max = 100;
int gain_slider;
const int warp_slider_max = 100;
int warp_slider;
float gain;
float warp;
Mat src;
Mat dst;
Mat wrp;

#define WIDTH 960
#define LIM(a,b,c) (((a)>(c))?(c):((a)<(b))?(b):(a))

//----------------------------------------------------------------------------------------
float InvWarp(float X,float Gn,float Wp,int Ht)
{
    float Exp = Wp;
    float Mx2 = Ht*Gn;
    float D1  = pow(Ht,Exp);
    float E1  = D1/Ht;
    float In  = LIM(X, 0, Ht);

    float Out = Mx2-pow((D1-In*E1),(1.0/Exp));

    if(Out<0)   Out=0;
    if(Out>=Ht) Out=Ht-1;

    return Out;
}
//----------------------------------------------------------------------------------------
static void on_trackbar(int, void*)
{
    int y,h;
    int Ht;

    gain = (float) 1.25*gain_slider/gain_slider_max ; gain+=1.0;
    warp = (float) 5.0*warp_slider/warp_slider_max ; warp+=1.0;

    std::cout << "warp : " << warp << "   gain : " << gain << std::endl;

    wrp = dst.clone();
    Ht = wrp.rows;
    for(h=0;h<Ht;h++){
        y=InvWarp(h,gain,warp,Ht);
        dst.row(y).copyTo(wrp.row(h));
    }

    imshow( "Warp", wrp);
}
//----------------------------------------------------------------------------------------
int main(void)
{
    std::string selectFile = "";
    FILE *in=NULL;

    in = popen("zenity  --title=\"Select an image\" --file-selection","r");

    if(in==NULL) return 1;
    else{
        char buff[512];
        while(fgets(buff, sizeof(buff), in) != NULL) { selectFile += buff; }
        pclose(in);
    }

    //remove the "\n"
    selectFile.erase(std::remove(selectFile.begin(), selectFile.end(), '\n'),selectFile.end());

    if(selectFile==""){ cout << "No file given \n"; return -1; }

    src = imread(selectFile);
    if(src.empty()) { cout << "Error loading src \n"; return -1; }

    int Hn=(src.rows*WIDTH)/src.cols;
    resize(src, dst, cv::Size(WIDTH,Hn), 0, 0, cv::INTER_LINEAR);


    warp_slider = 0;
    gain_slider = 0;
    namedWindow("Warp", WINDOW_AUTOSIZE); // Create Window

    snprintf( TrackName1, sizeof(TrackName1), "Gain" );
    createTrackbar(TrackName1, "Warp", &gain_slider, gain_slider_max, on_trackbar);

    snprintf( TrackName2, sizeof(TrackName2), "Warp" );
    createTrackbar(TrackName2, "Warp", &warp_slider, warp_slider_max, on_trackbar);

    on_trackbar(gain_slider, 0);
    waitKey(0);

    return 0;
}
//----------------------------------------------------------------------------------------

/*
#include <iostream>
#include <opencv2/highgui.hpp>

using namespace std;

void myButtonName_callback(int state, void*userData) {
    // do something
    printf("Button pressed\r\n");
}

int main()
{

createButton("myButtonName",myButtonName_callback,NULL,cv::PUSH_BUTTON,1);
//

//char buff[512];
//string selectFile = "";
//while (fgets(buff, sizeof(buff), in) != NULL) {
//    selectFile += buff;
//}
//pclose(in);
//
////remove the "\n"
//selectFile.erase(std::remove(selectFile.begin(), selectFile.end(), '\n'),
//            selectFile.end());
//
//// path + filename + format
//Mat image = imread(selectFile);

    cout << "Hello world!" << endl;
    return 0;
}
*/
