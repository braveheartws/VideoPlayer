# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

#搜索当前目录下的所有cpp文件
aux_source_directory(. SOURCE)
#生成动态库
add_library(
        native-lib
        SHARED
        ${SOURCE} )


#设置一个变量
set(libs ${CMAKE_SOURCE_DIR}/${ANDROID_ABI})
#-L：引入库查找路径
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${libs}")


#引入头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


target_link_libraries(
        native-lib
        avfilter avformat  avcodec avutil swresample swscale rtmp android z
        OpenSLES
        log )