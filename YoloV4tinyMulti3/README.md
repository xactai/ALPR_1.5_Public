## Multithreaded Simultaneous License Plate location. 

### A dynamic array of multiple video inputs on an NVIDIA board like the Jetson Nano.
👉 please read the explanation at [YoloMultiDusty](https://github.com/xactai/SCALPR-01.5/tree/main/YoloMultiDusty) and [YoloMultiDusty2](https://github.com/xactai/SCALPR-01.5/tree/main/YoloMultiDusty2) first. This repo is the sequel.<br><br>
The first study, YoloMultiDusty, showed that multithreading is the best solution for connecting multiple inputs to a Jeston board.<br>
The second study, YoloMultiDusty2, adds json configuration files and deep learning preparations.<br><br>
This study is about detecting the position of license plates in CCTV images.<br>
YoloV4-tiny replaces the YoloFastestV2 deep learning model because it runs on Darknet. Darknet is chosen due to its incorporated tracking algorithm. When you look closely, every recognized object has a label starting with a tracking number.<br>
Also new are the discrete OpenCV algorithms to detect the location of the license plate in the frames. It finds license plates in 9 mSec, whereas its Darknet brother takes 67 mSec.<br>

https://github.com/xactai/SCALPR-01.5/assets/44409029/0e18f4e7-f6c7-4995-a898-17313e9c4eea

------------

## Dependencies.
The dependencies are the same as in previous versions.

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
            "remote_hls_gstreamer": "souphttpsrc location=http://YOUR_HLSSTREAM_URL_HERE.m3u8 ! hlsdemux ! decodebin ! videoconvert ! videoscale ! appsink"
        },
        "DNN_Rect": {
            "x_offset": 100,
            "y_offset": 0,
            "width":  1280,
            "height": 1080
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
        }
    },

    "WORK_WIDTH": 1920,
    "WORK_HEIGHT": 1080,
    "THUMB_WIDTH": 1280,
    "THUMB_HEIGHT": 720,
    "VEHICLE_MIN_WIDTH" : 0.18,
    "VEHICLE_MIN_HEIGHT" : 0.15,
   
  
   
```
Most of the entries are well-known from the previous versions. Check out the READMEs for more information.<br>
#### DNN_Rect
Now every CCTV has its unique DNN_Rect. It defines the region that will use the DNN model.
#### MJPEG_PORT
Also new is the entry MJPEG_PORT per CCTV. Now, each processed image can be viewed individually.
#### VEHICLE_MIN_WIDTH, VEHICLE_MIN_HEIGHT
Only a vehicle entering the parking lot is of interest. Traffic on the road in the background should be ignored.<br>
The parameters determine the minimum dimensions of a vehicle. The numbers are ratios of the WORK_WIDTH and WORK_HEIGHT.<br>

------------

## License plate location detector.

![Car2](https://github.com/xactai/SCALPR-01.5/assets/44409029/013d008b-da7c-4652-a7f9-538f2f6a288d)<br>
_gray input_<br><br>
![out_m](https://github.com/xactai/SCALPR-01.5/assets/44409029/3d0460ab-1c88-4006-b6f6-5bfbff555671)<br>
_Morphology "black hat" transform_<br><br>
![out_e](https://github.com/xactai/SCALPR-01.5/assets/44409029/08805adc-6d05-4208-9f1a-facfdf4682a9)<br>
_Vertical edges detected by a Sobel kernal_<br><br>
![out_t](https://github.com/xactai/SCALPR-01.5/assets/44409029/4c5e541d-ca73-4d92-a12f-124e63a0d10a)<br>
_Morphology image converted to a binary image with the OTSU operator_<br><br>
![out_d](https://github.com/xactai/SCALPR-01.5/assets/44409029/f061a30f-bb69-4b94-9507-636b73b61731)<br>
_Dilate image by a 5x5 kernal_<br><br>
![out_c](https://github.com/xactai/SCALPR-01.5/assets/44409029/867059d5-3477-4ecf-9ab8-3bbcccd0e58b)<br>
_Mark the found contours with a rectangle_<br><br>
![output](https://github.com/xactai/SCALPR-01.5/assets/44409029/6d5a7e2c-d470-4fec-b633-ce3ed9a1dcdb)<br>
_After filtering the license plate is located_<br><br>



