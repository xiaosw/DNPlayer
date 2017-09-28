cmake_minimum_required(VERSION 3.4.1)

#添加动态库
file(GLOB my_source src/main/cpp/*.cpp src/main/cpp/*.c)
add_library(native-lib SHARED ${my_source})

#设置变量 指向ffmpeg静态库目录
set(my_lib_path ${CMAKE_SOURCE_DIR}/src/main/cpp/lib/${ANDROID_ABI})
#添加静态库 并设置库路径
add_library(avcodec STATIC IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${my_lib_path}/libavcodec.a)
add_library(avfilter STATIC IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${my_lib_path}/libavfilter.a)
add_library(avformat STATIC IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${my_lib_path}/libavformat.a)
add_library(avutil STATIC IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${my_lib_path}/libavutil.a)
add_library(postproc STATIC IMPORTED)
set_target_properties(postproc PROPERTIES IMPORTED_LOCATION ${my_lib_path}/libpostproc.a)
add_library(swresample STATIC IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${my_lib_path}/libswresample.a)
add_library(swscale STATIC IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${my_lib_path}/libswscale.a)
#设置头文件查找目录
include_directories(src/main/cpp/include)

#编译链接
target_link_libraries(native-lib avfilter avformat    avcodec  avutil postproc swresample swscale OpenSLES  z log android)