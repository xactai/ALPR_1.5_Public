## Multithreaded Simultaneous License Plate location. 

### Tracking and recognising license plates.
👉 please read the explanation at [YoloV4tinyMulti3](https://github.com/xactai/SCALPR-01.5/tree/main/YoloV4tinyMulti3), [YoloMultiDusty](https://github.com/xactai/SCALPR-01.5/tree/main/YoloMultiDusty) and [YoloMultiDusty2](https://github.com/xactai/SCALPR-01.5/tree/main/YoloMultiDusty2) first. This repo is the sequel.<br><br>
The first study, YoloMultiDusty, showed that multithreading is the best solution for connecting multiple inputs to a Jeston board.<br>
The second study, YoloMultiDusty2, adds json configuration files and deep learning preparations.<br><br>
The third study, YoloV4tinyMulti3, uses the YoloV4-tiny model and detects license plates in CCTV images.<br>
This study is all about tracking objects in time and space.<br>

------------

## How it works.
![image](https://github.com/xactai/SCALPR-01.5/assets/44409029/3aff2b06-8637-46c8-9432-f672bab9d2f5)

#### Work width and height
All input streams are first resized to `WORK_WIDTH, WORK_HEIGHT` to ensure uniform dimensions of all frames. It means you don't have to declare the model parameters for each stream; these now apply to all cameras simultaneously.<br>
It may be smaller than the RTSP streams received. Smaller resolutions require fewer resources, which increases processing speed. Or more cameras can be connected.<br>
On the other hand, the images cannot be chosen too small because the number plate will still have to be legible. Remember, the darknet model for the license plates works with [352x128](https://github.com/xactai/SCALPR-01.5/tree/main/YoloMultiDusty#this-repo-examines-rtsp-streams-with-nvidia-boards-like-the-jetson-nano) pixels.<br>
Frames larger than the camera resolution are processed, but do not add anything to the accuracy. They will only slow down the performance.

#### DNN_Rect
In the first phase, all objects in the `DNN_Rect` are detected by the YoloV4-tiny model.<br>
Most models, such as YoloV4, work best with square images. `DNN_Rect` defines an area of interest that is more or less square.<br>
In the screenshot above, the `DNN_Rect` is marked with a light blue rectangle. Square is not mandatory; it only improves the recognition rate.<br>
Because vehicles avoid the right wall, we can crop out a large part of the image and we're left with a square-like region.<br>
All detected object in `DNN_Rect` are marked with a thin blue rectangle (chair, monitor, guard outside).
In the `config.json` `DNN_Rect` is defined in pixels.

#### Foreground
The next phase selects only persons, two and four-wheelers, in the foreground.<br>
YoloV4 recognize a lot of objects. Most of them are in the street in the background; pedestrians, traffic like cars, tuk-tuks, etc.<br>
The most simple and effective way to filter those objects out is to look at their size and class (person, car, truck, tv-monitor, chair).<br>
In `config.json`, parameters `VEHICLE_MIN_WIDTH` and `VEHICLE_MIN_HEIGHT` define the size of the vehicles in the foreground.<br>
The same applies to the parameters `PERSON_MIN_WIDTH` and `PERSON_MIN_HEIGHT`. People smaller than the given numbers are considered in the background, like the guard in its blue uniform.<br>
Taller persons, like the man next to the car, are located in the foreground.
The numbers in the 'config.json' are relative to the frame resolution. For instance, an IMOU camera with 1920x1080 frames and a `VEHICLE_MIN_WIDTH` of 0.18, results in 0.18 x 1920 = 346. Only vehicles larger than 346 pixels are processed further. Every other vehicle is ignored.<br>
Using relative numbers is convenient because you don't have to change them every time the camera's resolution differs. After all, the dimensions remain the same.

#### Tracking
Once an object is situated in the foreground it will be tracked. A tracking routine tries to follow the object's movements.<br>
Darknet has a simple tracking routine incorporated into its framework. It is based on [Optical Flow](https://docs.opencv.org/3.4/d4/dee/tutorial_optical_flow.html).<br>
Objects who are tracked are marked with a thick rectangle. The tracking number is placed at the top right corner. Note, every class has its own tracking numbers. That's why the person and the car have the same tracking number 2.<br>
To highlight possible tracking errors, the color of the rectangle depends on the tracking number.

#### Route
At this stage, the static frame-by-frame domain transitions into the tractory domain. In other words, only movements are observed.<br> 
Once an object has a tracking number, a structure is created and added to the Rbusy array (Route Busy).<br>
Note, in addition to vehicles, people are tracked once they are detected also.<br>
The structure contains all essential information about the object.<br>
```cpp
// bbox_t
    unsigned int x, y, w, h;                        //(x,y) - top-left corner, (w, h) - width & height of bounded box
    float prob;                                     //confidence - probability that the object was found correctly
    unsigned int obj_id;                            //class of object - from range [0, classes-1]
    unsigned int track_id;                          //tracking id for video (0 - untracked, 1 - inf - tracked object)
    unsigned int frames_counter;                    //counter of frames on which the object was detected
    float x_3d, y_3d, z_3d;                         //center of object (in Meters) if ZED 3D Camera is used
// TObject
    bool     Used {false};                          //this object (vehicle or person) is further investigated.
    bool     Done {false};                          //this object has crossed the vehicle rect border and is considered processed.
    bool     PlateFound {false};                    //license plate found
    cv::Rect PlateRect;                             //location of the plate rectangle
    float    PlateMedge;                            //ratio of edges
    float    PlateFuzzy {0.0};                      //output weighted calcus for better plate
    cv::Mat  Plate;                                 //image of the plate
// TRoute    
    size_t  FirstFrame;                             //used for stitching two routes to one
    size_t  LastFrame;                              //when not updated, you know the object has left the scene
    std::chrono::steady_clock::time_point Tstart;   //time stamp first seen
    std::chrono::steady_clock::time_point Tend;     //time stamp last seen
    std::chrono::steady_clock::time_point TFinp;    //time vehicle is first inspected
    std::chrono::steady_clock::time_point TLinp;    //time vehicle is last inspected
    int Person_ID {-1};                             //tracking number of person inspecting the vehicle
    bool Inspected {false};                         //is the vehicle inspected?
    int Xc {0};                                     //ROI center (x,y)
    int Yc {0};                                     //ROI center (x,y)
    int Xt, Yt;                                     //previous center (x,y)
    float Velocity;                                 //pixels/100mSec (0.1Sec to increase readability)
    TMoveAver<float> Speed;                         //average speed over the last 5 observations
    float PlateSpeed;                               //average speed when taken snapshot of license
    float PlateEdge;                                //edge ratio in plate
    float PlateRatio {0.0};                         //plate width/height
    std::string PlateOCR;                           //OCR best result of the plate (if found)
```
It's a long list because C++ inheritance is used here.<br>
TRoute inherits all elements from TObject, which in turn has already inherited the structure bbox_t from Darknet.<br><br>

When an object gets a tracking number `track_id`, a new structure is allocated in memory.<br>
`Used` is set, `Tstart` is filled with the current time and date, and `FirstFrame` with the current frame number. Using the frame counter eases time-related calculations.
`Xc` and `Yc` are holding the object's centre. The values are calculated from bbox_t `x`, `y`, `w` and `h`.<br>
When the object is identified the next time, `LastFrame` and `Tend` are filled. `Xc, Yc` are transferred to `Xt, Yt` before the new ROI centre is updated.
`Velocity` is calculated from the Euclidean distance between the `Xc, Yc` and `Xt, Yt`, divided by the elapsed time. `Speed` gives a moving average of the last 5 velocities.<br><br>
When a license plate is detected, `PlateFound` is true, 'PlateSpeed' is filled with the average speed and `PlateEdge` with the variable `PlateMedge`. See the pictures at [YoloV4tinyMulti3](https://github.com/xactai/SCALPR-01.5/tree/main/YoloV4tinyMulti3#license-plate-location-detector). The last parameter, `PlateRatio`, is filled with the license plate width/height ratio. Subsequence frames will update all these parameters.<br><br>
The goal now is to get the best frame for the OCR detection of the license. The wider the plate, the better. However, images may be blurry. This phenomenon is caused by large shutter speeds and the H265 compression. The latter reduces the bandwidth of the video signal by removing redundant regions between consecutive frames. Details, like tiny characters, only become visible when larger movements have been processed. Hence license plates become clearer when the vehicle is stationary in front of the camera.
#### Fuzzy weights.
The best frame is a weighted sum of the width, edges and speed. The variables are multiplied by `WEIGHT_WIDTH, WEIGHT_EDGE` and `WEIGHT_SPEED` from the Config.json. In the image above, you can see this calculation in the terminal window. Please note that the numbers are not normalized. In other words, the 114 plate width is typical of this camera setup. The same applies to the edges and speed. Other situations can give a number a different size.<br>
If the Fuzzy number exceeds the previous one, the license plate is considered 'better'. `PlateFuzzy` is loaded with this higher value, and a copy of the license plate image (`cv::Mat  Plate`) is created. Over time, you see the license plate image become wider and clearer (as long as the vehicle is moving slowly).
Play a bit with the weights to get the best result. A lot of emphasis on width can result in larger plates, but with vague characters, the OCR engine will eventually generate more errors than a smaller plate with sharp letters.
#### Rdone.
After a frame update, all elements in the Rbusy are scanned for their `LastFrame`. If a `LastFrame` does not match the current frame number, you know that the object has not been updated. If a license plate is found, and the image `Plate` isn't empty, the OCR engine is invoked to read the license plate. This engine is currently the same as found in https://github.com/xactai/qengineering-01. The result is stored in `PlateOCR`, a readable std::string.<br>
Time to move the structure to the Rdone (Route Done) array. At the same time, it is removed from the Rbusy array.<br><br>
Most of the time, this mechanism works fine. However, there are situations where it fails. It has to do with the imperfection of the Darknet tracking engine. When `track_id` doesn't occur any more, it doesn't always mean it left the scene. The large object in front may be hiding it. Think of a tuk-tuk moving near the camera and blocking the view to the entrance. Once moved away, the original object re-appears.<br> 
The Darknet tracking engine may respond to this event by generating the same `track-id` or issuing a new tracking number.<br><br>
Without special measures, a new Rbusy object will be created. As a result, the vehicle is processed twice. In order to prevent this happening, the Rdone list is checked to see if it has not been added last. The first thing to look at is the tracking number. The second check is the time difference. With a few frames between the two, you know vehicle is the same. After all, in practice, it is impossible to find another car in the same spot within a second.
If it turn out that the vehicle is identical, it is been pushed back to the Rbusy list. 
#### VEHICLE_Rect
As already explained, the most legible license plate. is found through a weighted mix of width, edges and speed.
After checking in, the vehicle increases speed and passes under the camera. It can also make a turn to the next floor. During this manoeuvre, the number plate widens. However, the readability decreases because not only does the speed increase but also because the image is increasingly skewed.
The easiest way to filter this is to define a region where readability is likely guaranteed; the VEHICLE_rect, in the image above, is marked by a thin blue line. In the Config.JSON it is defined by relative numbers. For example, "left": 0.234 stands for 0.234*DNN_Rect.width + DNN_Rect.x = 400 pixels left from the input frame.
Once a vehicle crosses this boundary, the processing is stopped by setting the `Done` flag and resetting `Used`.<br>
Please note, it only applies to four-wheelers. Two-wheelers are best recognized by YoloV4 from their side view. In order words, let them take a turn in front of the camera. They have their `Done` flag set as soon as they crossed the DNN_Rect.
#### Inspection
The 'Inspected' flag is set when a person and a vehicle are nearby for a specified time period.<br>
It is a rudimentary setup. The Euclidean distance from the vehicle to the person is determined. If it is small enough, a timer starts running.
The timer continues as long as a person remains within the defined distance from the vehicle. Accidental passers-by are taken into account also.
Once the vehicle leaves the crime scene, the `Inspected` flag is set or reset, depending on the time measured. Two-wheelers are not processed.
#### OCR
Once the `Done` flag is set, the OCR engine starts. If a license plate is found, the OCR routines try to read the number plate.
As shown [earlier](https://github.com/xactai/qengineering-01), the result becomes more reliable when the heuristic algorithms work. You can enable it with `"Heuristic": true` in the config.json.
The routine will display " ?? " if an image is unreadable.

------------

## Dependencies.
To run the application, you need to have:
- Darknet ([the Alexey version](https://github.com/AlexeyAB/darknet)) installed.
- OpenCV 64-bit installed.
- ncnn framework installed.
- JSON for C++ installed.
- MQTT installed.
- plog installed.
- Code::Blocks installed, if you like to work with the C++ source. 

### Installing the dependencies.
Start with the usual 
```
$ sudo apt-get update 
$ sudo apt-get upgrade
$ sudo apt-get install curl libcurl3
$ sudo apt-get install cmake wget
```
#### JSON for C++
written by [Niels Lohmann](https://github.com/nlohmann).
```
$ git clone https://github.com/nlohmann/json.git
$ cd json
$ mkdir build
$ cd build
$ cmake ..
$ make -j4
$ sudo make install
$ sudo ldconfig
```
#### plog (Logger)
```
$ git clone --depth=1 https://github.com/SergiusTheBest/plog.git
$ cd plog
$ mkdir build
$ cd build
$ cmake ..
$ make -j4
$ sudo make install
$ sudo ldconfig
```
#### paho.mqtt (MQTT client)
```
$ git clone --depth=1 https://github.com/eclipse/paho.mqtt.c.git
$ cd paho.mqtt.c
$ mkdir build
$ cd build
$ cmake ..
$ make -j4
$ sudo make install
$ sudo ldconfig
```
#### Mosquitto (MQTT broker)
```
$ sudo apt-get install mosquitto
```
#### Code::Blocks
```
$ sudo apt-get install codeblocks
$ sudo apt-get install codeblocks-contrib
```
If you want to run the app from within the IDE, you need to set the environment variables in Code::Blocks also.<br>
You can find them in menu option Settings->Environment. On the left side scroll down to 'Environment variables' and add these two lines.<br>
```
MQTT_SERVER=localhost:1883
MQTT_CLIENT_ID=Xinthe_parking
```
#### OpenCV
Follow this [guide](https://qengineering.eu/install-opencv-4.5-on-jetson-nano.html).
#### Darknet
Follow this [guide](https://qengineering.eu/install-darknet-on-jetson-nano.html).

------------

## Config.json.
All required settings are listed in the `config.json` file. Without this file, the app will not work.
```json
{
    "VERSION": "1.1.0",

    "STREAMS_NR":1,

    "STREAM_1": {
        "CAM_NAME": "Cam 1",
        "MJPEG_PORT": 8091,
        "VIDEO_INPUT": "video",
        "VIDEO_INPUTS_PARAMS": {
            "image": "./images/car.jpg",
            "folder": "./inputs/images",
            "video": "./Cars.mp4",
            "usbcam": "v4l2src device=/dev/video0 ! video/x-raw(memory:NVMM),width=640, height=360, framerate=30/1 ! videoconvert ! appsink",
            "CSI1": "nvarguscamerasrc sensor_id=0 ! video/x-raw(memory:NVMM),width=640, height=480, framerate=15/1, format=NV12 ! nvvidconv ! video/x-raw, format=BGRx, width=640, height=480 ! videoconvert ! video/x-raw, format=BGR ! appsink",
            "CSI2": "nvarguscamerasrc sensor_id=1 ! video/x-raw(memory:NVMM),width=640, height=480, framerate=15/1, format=NV12 ! nvvidconv ! video/x-raw, format=BGRx, width=640, height=480 ! videoconvert ! video/x-raw, format=BGR ! appsink",
            "CCTV": "rtsp://192.168.178.20:8554/test/",
            "IMOU": "rtsp://admin:L230BF4A@192.168.178.40:554/cam/realmonitor?channel=1&subtype=0",
            "remote_hls_gstreamer": "souphttpsrc location=http://YOUR_HLSSTREAM_URL_HERE.m3u8 ! hlsdemux ! decodebin ! videoconvert ! videoscale ! appsink"
        },
        "DNN_Rect": {
            "x_offset": 100,
            "y_offset": 0,
            "width":  1280,
            "height": 1080
        },
        "VEHICLE_Rect": {
            "left": 0.234,
            "top": 0.0,
            "right":  0.0,
            "bottom": 0.185
        }
    },

    "STREAM_2": {
        "CAM_NAME": "Cam 2",
        "MJPEG_PORT": 8092,
        "VIDEO_INPUT": "CCTV",
        "VIDEO_INPUTS_PARAMS": {
            "CCTV": "rtsp://192.168.178.26:8554/test/"
        },
        "DNN_Rect": {
            "x_offset": 0,
            "y_offset": 0,
            "width":  640,
            "height": 480
        },
        "VEHICLE_Rect": {
            "left": 0.234,
            "top": 0.0,
            "right":  0.0,
            "bottom": 0.185
        }
    },

    "WORK_WIDTH": 1920,
    "WORK_HEIGHT": 1080,
    "THUMB_WIDTH": 1280,
    "THUMB_HEIGHT": 720,
    "VEHICLE_MIN_WIDTH" : 0.18,
    "VEHICLE_MIN_HEIGHT" : 0.15,

    "PERSON_MIN_WIDTH" : 0.04,
    "PERSON_MIN_HEIGHT" : 0.07,
   
    "WEIGHT_WIDTH": 0.25,
    "WEIGHT_EDGE": 4.0,
    "WEIGHT_SPEED": 1.5,
  
   
```
Most of the entries are well-known from the previous versions. Check out the READMEs for more information.<br>
#### DNN_Rect
Now every CCTV has its unique DNN_Rect. It defines the region that will use the DNN model.
#### MJPEG_PORT
Also new is the entry MJPEG_PORT per CCTV. Now, each processed image can be viewed individually.
#### VEHICLE_MIN_WIDTH, VEHICLE_MIN_HEIGHT
Only a vehicle entering the parking lot is of interest. Traffic on the road in the background should be ignored.<br>
The parameters determine the minimum dimensions of a vehicle. The numbers are ratios of the WORK_WIDTH and WORK_HEIGHT.<br>

