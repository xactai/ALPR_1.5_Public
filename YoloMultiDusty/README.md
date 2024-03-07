## Multithread camera test 

### This repo examines RTSP streams with NVIDIA boards like the Jetson Nano.
As with all edge-based deep learning software, implementation is like a jigsaw puzzle.<br>
Due to limited resources, you need to split the workload between the CPU and GPU processors.<br>
With a carefully balanced design, it is possible to get good inference times.<br><br>
An important part is the handling of RTSP streams.<br>
These CCTV images must be processed and converted to the OpenCV standard (8-bit BGR).<br>
Obviously, the fewer resources required, the better.<br><br>
The captured images must be of a certain quality.<br>
Detailed, high-resolution images give much better results than small, noisy images.<br>
On the other hand, processing large images requires more resources. A trade-off is needed.<br><br>
Deep learning models have predefined formats. In the case of ALPR:
| Model | size |
| :---: | :---:|
| Vehicle Detection | 603x416x3 |
| License Detection | 416x416x3 |
| OCR Model | 352x128x3 |

One could use 640 x 480 CCTV to detect vehicles. However, the license plates in these images are too small to be properly recognized.<br>
The IMOU camera used has a resolution of 1920 x 1080, which produces license plates of approximately 147 x 61. These license plates are processed without many errors.<br>
Using larger images is likely to result in H265 artefacts considering Ethernet bandwidth.<br><br>
To get a more realistic impression, the tests are all done while a deep learning model is running.<br>
The model is a YoloFastestV2 employed on an ncnn framework. The ncnn framework has Vulkan support, hence working with the GPU.<br>
The streams must be near real-time. Latency times should be as short as possible.<br>
Several strategies are possible. The tables below show the results.<br>

------------
## Benchmark.

### RTSP polling
The [first ALPR](https://github.com/xactai/qengineering-01) application uses the polling technique.<br>
It calculates missed frames and skips them before grabbing a new image.<br>
##### H264 encoded, 25 frames per second RTSP streams.
| 1280 x 960 | FPS | CPU | Mem (MB) | Comment |
| :---: | :---: | :---: | :---:| :--- |
| 1 cam | 8.7 | 29% |  77 | |
| 2 cam | 4.1 | 46% | 108 | |
| 3 cam |  |  |  | Crashed |
| 4 cam |  |  |  | Crashed |

### Multithreading
The technique works by grabbing the frames in a separate thread.<br>
##### H264 encoded, 25 frames per second RTSP streams.
| 1280 x 960 | FPS | CPU | Mem (MB) | Comment |
| :---: | :---: | :---: | :---:| :--- |
| 1 cam | 24 | 41% | 102 | |
| 2 cam | 10.6 | 65% | 155 | |
| 3 cam | 4.6 | 85% | 180 | Unstable  |
| 4 cam | 2.7 | 95% | 210 | Highly unstable |

Use pipeline:
```
cap.open("rtsp://192.168.178.20:8554/test/"); 
```

### NVIDIA Multithreading
The technique is identical to the one above. The only difference is the used GStreamer pipeline.<br>
It is designed for NVIDIA boards and works with the DMA memory. As can see, it is superior to the others.<br>
##### H264 encoded, 25 frames per second RTSP streams.
| 1280 x 960 | FPS | CPU | Mem (MB) | Comment |
| :---: | :---: | :---: | :---:| :--- |
| 1 cam | 25 | **17%** | 89 | |
| 2 cam | 20 | **23%** | 108 | |
| 3 cam | 11.5 | **26%** | 120 | |
| 4 cam | 8.8 | **29%** | 143 | |

| 1920 x 1080 | FPS | CPU | Mem (MB) | Comment |
| :---: | :---: | :---: | :---:| :--- |
| 1 cam | 25 | **17%** | 93 | |
| 2 cam | 18.3 | **22%** | 110 | |
| 3 cam | 11.3 | **27%** | 135 | |
| 4 cam | 8.8 | **30%** | 153 | |

Use pipeline:
```
cap.open("rtspsrc location=rtsp://192.168.178.20:8554/test/ latency=300 ! rtph264depay ! \
          h264parse ! queue max-size-buffers=10 leaky=2 ! nvv4l2decoder ! video/x-raw(memory:NVMM) , format=(string)NV12 ! \
          nvvidconv ! video/x-raw , width=1280 , height=960, format=(string)BGRx ! videoconvert ! appsink drop=1 sync=0"); 
```

------------

## Running the app.

To run the application, load the project file `YoloMultiDusty.cbp` in Code::Blocks.<br/> 
Give the RTSP streams in the `settings.txt`.<br> 
Give only the RTSP address, not the full pipeline. The app will generate to correct string for you.

