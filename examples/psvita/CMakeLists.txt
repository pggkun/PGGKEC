cmake_minimum_required(VERSION 3.2)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(pggk_crossplay_vita_example)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "PGGKEC Vita Example")
set(VITA_TITLEID  "PGGK00001")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(
  common
)

include_directories(
  ../../include
)

add_executable(${PROJECT_NAME}
  source/client.cpp
  common/debugScreen.c
)

target_link_libraries(${PROJECT_NAME}
  pthread
  SceDisplay_stub
  SceCtrl_stub
  SceNet_stub
  SceSysmodule_stub
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
)
