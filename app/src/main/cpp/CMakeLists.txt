# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

#下面这两行是剔除编译过程中生成二进制码
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(INC_DIR ${CMAKE_SOURCE_DIR})

include_directories(boost)
add_subdirectory(boost)

include_directories(${INC_DIR})

#add_library(boost SHARED IMPORTED)
#set_target_properties(boost PROPERTIES IMPORTED_LOCATION
#        ${INC_DIR}/lib/libmars-boost.a)

add_library( # Sets the name of the library.
        xlogger

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        #${INC_DIR}/logger/util/time_utils.c
        #${INC_DIR}/logger/util/tickcount.cc

        ${INC_DIR}/logger/util/formater.cc
        ${INC_DIR}/logger/util/file_operator.cc
        ${INC_DIR}/logger/util/mmap_util.cc
        ${INC_DIR}/logger/util/scoped_jstring.cc
        ${INC_DIR}/logger/util/autobuffer.cc
        ${INC_DIR}/logger/util/ptrbuffer.cc
        ${INC_DIR}/logger/log/loginfo_extract.c
        ${INC_DIR}/logger/log/log_crypt.cc
        ${INC_DIR}/logger/log/log_buffer.cc
        ${INC_DIR}/logger/log/xloggerbase.cc
        ${INC_DIR}/logger/log/appender.cc
        ${INC_DIR}/logger/jni/java2C_Xlog.cc
        )

find_library( # Sets the name of the path variable.
        log-lib
        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

target_link_libraries( # Specifies the target library.
        xlogger
#        boost
        mars-boost
        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})