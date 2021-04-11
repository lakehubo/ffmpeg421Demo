# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions -Wall")

#获取cmake父目录
get_filename_component(PARENT_DIR ${CMAKE_SOURCE_DIR} DIRECTORY)

#ffmpeg so文件路径
set(lib_src_DIR ${PARENT_DIR}/jniLibs/${ANDROID_ABI})

#配置加载native依赖
include_directories(${CMAKE_SOURCE_DIR}/include
        utils
        render
        sample)

#动态方式加载ffmepg的so文件 第三方库加载方式
add_library(ffmpeg SHARED IMPORTED)

#引入libffmpeg.so文件
set_target_properties(
        ffmpeg
        PROPERTIES
        IMPORTED_LOCATION
        ${lib_src_DIR}/libffmpeg.so)

#opengl render
set(src_render_files
        render/GLRenderCall.cpp
        render/LGLRenderContext.cpp
        utils/GLUtils.cpp
        sample/YUV420PTexture.cpp
        sample/NV12Texture.cpp
        sample/NV21Texture.cpp)

#ffmpeg ffplay
set(ffmpeg_files
        L_Callback.c
        L_Audio.c
        L_Clock.c
        L_Decoder.c
        L_FrameQueue.c
        L_PacketQueue.c
        L_Thread.c
        L_Cond.c
        L_Mutex.c)

add_library(lplay SHARED
        lplay.cpp
        ${ffmpeg_files}
        ${src_render_files})

find_library(
        log-lib
        log)

target_link_libraries( # Specifies the target library.
        lplay
        android
        ffmpeg
        EGL
        GLESv3
        OpenSLES
        ${log-lib}
        m)

