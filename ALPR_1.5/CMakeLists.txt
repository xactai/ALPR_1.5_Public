cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(ALPR_1_5)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexceptions -pthread -O2")

find_package(OpenCV REQUIRED)   
find_package(gflags REQUIRED)

list(APPEND CMAKE_LIBRARY_PATH "/usr/local/lib/")

set(SOURCES src/FPS.cpp src/main.cpp src/MJPG_sender.cpp src/MJPGthread.cpp src/MQTT.cpp src/ProcessPipe.cpp src/Regression.cpp src/ThreadCam.cpp src/Tjson.cpp src/TMapper.cpp src/TOCR.cpp src/TPortal.cpp src/TAlbum.cpp src/TWorker.cpp src/Kmain.cpp src/TimeKeeper.cpp src/ByteTrack/BYTETracker.cpp src/ByteTrack/STrack.cpp src/ByteTrack/kalmanFilter.cpp src/ByteTrack/lapjv.cpp src/ByteTrack/utils.cpp src/TFFmpeg.cpp src/TGstreamer.cpp src/TOstreamer.cpp)


set(EXTRA_LIBS ${OpenCV_LIBS} libdarknet.so libpaho-mqtt3c.so libpaho-mqtt3a.so /usr/local/cuda/lib64/libcudart.so)

include_directories(include ${OpenCV_INCLUDE_DIRS} ./include ./include/ByteTrack /usr/local/include /usr/include/eigen3 )

set(LINK_OPT -fopenmp -s -ldl -lpthread -pthread -O3 -lgomp -DNDEBUG -rdynamic -s)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
	message(STATUS "Detect x86_64")
	set(GFLAGS_LIBS /usr/lib/x86_64-linux-gnu/libgflags.so)
	set(FFMPEG_LIBS /usr/local/lib/libavcodec.so /usr/local/lib/libavdevice.so /usr/local/lib/libavfilter.so /usr/local/lib/libavformat.so /usr/local/lib/libavutil.so /usr/local/lib/libswresample.so /usr/local/lib/libswscale.so)
else()
	message(STATUS "Detect aarch64")
	set(GFLAGS_LIBS /usr/lib/aarch64-linux-gnu/libgflags.so)
	set(FFMPEG_LIBS /usr/lib/aarch64-linux-gnu/libgflags.so /usr/lib/aarch64-linux-gnu/libavcodec.so /usr/lib/aarch64-linux-gnu/libavformat.so /usr/lib/aarch64-linux-gnu/libavutil.so /usr/lib/aarch64-linux-gnu/libswresample.so /usr/lib/aarch64-linux-gnu/libswscale.so)
	set(LINK_OPT ${LINK_OPT} -lstdc++fs)
endif()

add_executable(ALPR_1_5 ${SOURCES})

set(EXECUTABLE_OUTPUT_PATH "../")

target_link_libraries(ALPR_1_5 ${EXTRA_LIBS} ${GFLAGS_LIBS} ${LINK_OPT} ${FFMPEG_LIBS})

