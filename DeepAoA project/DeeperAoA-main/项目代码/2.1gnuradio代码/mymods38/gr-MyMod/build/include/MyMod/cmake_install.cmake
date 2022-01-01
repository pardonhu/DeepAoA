# Install script for directory: /mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/MyMod" TYPE FILE FILES
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/api.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/avg_ff.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/avg_v.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/avg_v_ff.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/cal_csi.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/cal_multi_usrp.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/cor_inf.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/cor_pac_aoa.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/cp2offs.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/extract_lts.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/lts_cor.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/msg2cv.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/smooth_pi.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/unwrap.h"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/include/MyMod/wifi_samples_align.h"
    )
endif()

