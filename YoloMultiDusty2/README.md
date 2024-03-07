## Multithreaded Simultaneous Inputs Interface 

### A dynamic array of multiple video inputs on an NVIDIA board like the Jetson Nano.
👉 please read the explanation at [YoloMultiDusty](https://github.com/xactai/SCALPR-01.5/tree/main/YoloMultiDusty) first. This repo is the sequel.<br><br>
The first study, YoloMultiDusty, showed that multithreading is the best solution for connecting multiple inputs to a Jeston board.
When a dedicated NVIDIA pipeline is used, the CPU and GPU are under minimal load. This provides the necessary space for other (computer intensive) algorithms such as deep-learning and/or object tracking.<br><br>
This repo is the follow-up to this. The settings file has been replaced by a JSON script. In addition, preparations have been made to easily apply deep-learning in the future.
The different inputs live in a dynamic array. There is no limit to the number. Only the board resources, like memory, are the limitation.<br><br>
![v0GX38JP0UFO3lXZ](https://github.com/xactai/SCALPR-01.5/assets/44409029/53c0e92a-3718-40f2-be2a-0eec245f58e0)<br>
_Example of seven inputs (in this case 7 RTSP streams) captured on a Jetson Nano._<br><rb>

------------

## Dependencies.
To run the application, you need to have:
- A member of the Jetson family, like a Jetson Nano or Xavier.<br>
- OpenCV 64-bit installed.
- ncnn framework installed.
- JSON for C++ installed.
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
```
#### Code::Blocks
```
$ sudo apt-get install codeblocks
```
#### OpenCV
Follow this [guide](https://qengineering.eu/install-opencv-4.5-on-jetson-nano.html).
#### ncnn
Follow this [guide](https://qengineering.eu/install-ncnn-on-jetson-nano.html).

------------

## Config.json.
All required settings are listed in the `config.json` file. Without this file, the app will not work.
```json
{
    "VERSION": "1.1.0",

    "STREAMS_NR":2,

    "STREAM_1": {
        "CAM_NAME": "Cam 1",
        "VIDEO_INPUT": "CCTV",
        "VIDEO_INPUTS_PARAMS": {
            "image": "./images/car.jpg",
            "folder": "./inputs/images",
            "video": "./images/demo.mp4",
            "usbcam": "v4l2src device=/dev/video0 ! video/x-raw(memory:NVMM),width=640, height=360, framerate=30/1 ! videoconvert ! appsink",
            "CSI1": "nvarguscamerasrc sensor_id=0 ! video/x-raw(memory:NVMM),width=640, height=480, framerate=15/1, format=NV12 ! nvvidconv ! video/x-raw, format=BGRx, width=640, height=480 ! videoconvert ! video/x-raw, format=BGR ! appsink",
            "CSI2": "nvarguscamerasrc sensor_id=1 ! video/x-raw(memory:NVMM),width=640, height=480, framerate=15/1, format=NV12 ! nvvidconv ! video/x-raw, format=BGRx, width=640, height=480 ! videoconvert ! video/x-raw, format=BGR ! appsink",
            "CCTV": "rtsp://192.168.178.20:8554/test/",
            "remote_hls_gstreamer": "souphttpsrc location=http://YOUR_HLSSTREAM_URL_HERE.m3u8 ! hlsdemux ! decodebin ! videoconvert ! videoscale ! appsink"
        }
    },

    "STREAM_2": {
        "CAM_NAME": "Cam 2",
        "VIDEO_INPUT": "CCTV",
        "VIDEO_INPUTS_PARAMS": {
            "CCTV": "rtsp://192.168.178.26:8554/test/"
        }
    },

    "STREAM_3": {
        "CAM_NAME": "Cam 3",
        "VIDEO_INPUT": "CCTV",
        "VIDEO_INPUTS_PARAMS": {
            "CCTV": "rtsp://192.168.178.25:8554/test/"
        }
    },

    "STREAM_4": {
        "CAM_NAME": "Cam 4",
        "VIDEO_INPUT": "CCTV",
        "VIDEO_INPUTS_PARAMS": {
            "CCTV": "rtsp://192.168.178.36:8554/test/"
        }
    },

    "WORK_WIDTH": 640,
    "WORK_HEIGHT": 480,
    "THUMB_WIDTH": 640,
    "THUMB_HEIGHT": 480,

    "MJPEG_PORT": 8090,

    "DNN_Rect": {
        "x_offset": 0,
        "y_offset": 0,
        "width":  640,
        "height": 480
    }
}
```
#### STREAMS_NR
Give the number of used streams. Can be any figure, there is no hard-coded limitation.<br> 
However, keep in mind the available resources.
#### STREAM_x
Define your input for flow x here. The number of stream_ must be at least equal to the number of streams given above.
#### CAM_NAME
For your convenience, you can give the RTSP stream a readable name.
#### VIDEO_INPUT
Select your video input. It can be one of the sources listed under `VIDEO_INPUTS_PARAMS`:<br>
`file, movie, usbcam, raspberrycam, remote_cam or remote_hls_gstreamer`.<br>
Default choice is an RTSP video stream.
#### VIDEO_INPUTS_PARAMS
| Item      | Description |
| --------- | -----|
| image  | Name and location of the picture. It must be a jpg or png file. |
| folder  | Directory containing the pictures. They must be jpg or png. |
| video | Name and location of the video file. |
| usbcam  | The GStreamer pipeline connecting the ALPR to an USB camera. |
| CSI1 | The GStreamer pipeline connecting the ALPR to an MIPI camera (port 0). |
| CSI2 | The GStreamer pipeline connecting the ALPR to an MIPI camera (port 1). |
| CCTV | The GStreamer pipeline connecting the ALPR to an RTSP source. |
| remote_hls_gstreamer | The GStreamer pipeline connecting the ALPR to an HLS source. |
#### WORK_WIDTH, WORK_HEIGHT
Define the size of the frames that the ALPR will use.<br>
It may be smaller than the RTSP streams received. Smaller resolutions require fewer resources, which increases processing speed. Or more cameras can be connected.<br>
On the other hand, the images cannot be chosen too small because the number plate will still have to be legible. Remember, the darknet model for the license plates works with [352x128](https://github.com/xactai/SCALPR-01.5/tree/main/YoloMultiDusty#this-repo-examines-rtsp-streams-with-nvidia-boards-like-the-jetson-nano) pixels.<br>
Frames larger than the camera resolution are processed, but do not add anything to the accuracy. They will only slow down the performance.
#### THUMB_WIDTH, THUMB_HEIGHT
The individual images are merged into an overview that is displayed on the screen. The individual size and height of these thumbnails are given here.
#### MJPEG_PORT
The port number of the local host to which the composed video is streamed. It is not possible to send the individual images.
#### DNN_Rect
Most deep learning models work with square images. Images from a Full-HD input have an aspect ratio of 16:9.
These have been reduced to a square. This causes distortion. Especially fast lightweight models such as YoloX or YoloFastest are sensitive to these artifacts. They recognize fewer objects.
With the parameters of DNN_Rect a section can be sent to the model.<br><br>
![image](https://github.com/xactai/SCALPR-01.5/assets/44409029/73e6814b-523a-4b24-9747-63fe0f097767)<br>
_Input image_.<br>
![image](https://github.com/xactai/SCALPR-01.5/assets/44409029/2f95ab85-77c3-467e-9df7-c40c6a4235c6)<br>
_Resized to 352x352_.<br>
![image](https://github.com/xactai/SCALPR-01.5/assets/44409029/b964a147-b930-414f-b928-4cfe3b13359c)<br>
_Using the DNN_Rect_.


------------

## Running the app.

To run the application, load the project file `YoloMultiDusty.cbp` in Code::Blocks.<br/> 
Give the RTSP streams in the `config.json`.<br>
