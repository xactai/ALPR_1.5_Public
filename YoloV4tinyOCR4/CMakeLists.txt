cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(YoloV4tinyOCR)

macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 17)
  endif ()
endmacro(use_cxx11)

find_package(OpenCV REQUIRED)                              

set(SOURCES src/FPS.cpp src/Main.cpp src/MJPG_sender.cpp src/MJPGthread.cpp src/MQTT.cpp src/ProcessPipe.cpp src/Regression.cpp src/ThreadCam.cpp src/Tjson.cpp src/TMapper.cpp src/TOCR.cpp src/TPortal.cpp src/TSnapShot.cpp)

set(EXTRA_LIBS ${OpenCV_LIBS} /usr/local/lib/libdarknet.so /usr/local/lib/libpaho-mqtt3c.so /usr/local/lib/libpaho-mqtt3a.so)

set(EXECUTABLE_OUTPUT_PATH "../")

include_directories(include ${OpenCV_INCLUDE_DIRS} ./include /usr/local/include )

add_link_options(-fopenmp -s -ldl -lpthread -pthread -O3 -lgomp -DNDEBUG -rdynamic -s)

add_executable(YoloV4tinyOCR ${SOURCES})

target_link_libraries(YoloV4tinyOCR ${EXTRA_LIBS})
