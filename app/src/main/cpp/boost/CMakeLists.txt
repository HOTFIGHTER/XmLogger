cmake_minimum_required(VERSION 3.4.1)
#下面这两行是剔除编译过程中生成二进制码
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

set(INC_DIR ${CMAKE_SOURCE_DIR})
set(LINK_DIR ${CMAKE_SOURCE_DIR}/boost)

project(mars-boost)

include_directories(.)
include_directories(..)

set(SELF_SRC_FILES
        ${LINK_DIR}/libs/iostreams/src/mapped_file.cpp)


#if(MSVC)
#    include_directories(../comm/windows)
#    list(APPEND SELF_SRC_FILES
#            ${LINK_DIR}/libs/thread/src/win32/thread.cpp
#            ${LINK_DIR}/libs/thread/src/win32/tss_dll.cpp
#            ${LINK_DIR}/libs/thread/src/win32/tss_pe.cpp)
#
#endif()

#if (ANDROID)
#    file(GLOB SELF_ANDROID_SRC_FILE
#            ${LINK_DIR}/libs/coroutine/src/*.cpp
#            ${LINK_DIR}/libs/coroutine/src/detail/*.cpp
#            ${LINK_DIR}/libs/coroutine/src/posix/*.cpp
#            ${LINK_DIR}/libs/context/src/*.cpp
#            ${LINK_DIR}/libs/context/src/posix/*.cpp)
#
#    list(APPEND SELF_SRC_FILES ${SELF_ANDROID_SRC_FILE})
#    enable_language(ASM)
#
#    if (ANDROID_ABI MATCHES "^armeabi(-v7a)?$")
#        list(APPEND SELF_SRC_FILES
#                ${LINK_DIR}/libs/context/src/asm/jump_arm_aapcs_elf_gas.S
#                ${LINK_DIR}/libs/context/src/asm/make_arm_aapcs_elf_gas.S)
#    elseif (ANDROID_ABI STREQUAL arm64-v8a)
#        list(APPEND SELF_SRC_FILES
#                ${LINK_DIR}/libs/context/src/asm/jump_arm64_aapcs_elf_gas.S
#                ${LINK_DIR}/libs/context/src/asm/make_arm64_aapcs_elf_gas.S)
#    elseif (ANDROID_ABI STREQUAL x86)
#        list(APPEND SELF_SRC_FILES
#                ${LINK_DIR}/libs/context/src/asm/jump_i386_sysv_elf_gas.S
#                ${LINK_DIR}/libs/context/src/asm/make_i386_sysv_elf_gas.S)
#    elseif (ANDROID_ABI STREQUAL x86_64)
#        list(APPEND SELF_SRC_FILES
#                ${LINK_DIR}/libs/context/src/asm/jump_x86_64_sysv_elf_gas.S
#                ${LINK_DIR}/libs/context/src/asm/make_x86_64_sysv_elf_gas.S)
#    endif ()
#
#endif ()

#set_source_files_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
add_library(${PROJECT_NAME} STATIC ${SELF_SRC_FILES})
target_link_libraries(${PROJECT_NAME} log)

#install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION ${CMAKE_SYSTEM_NAME}.out)
    
    

