# Install script for directory: /mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/gnuradio/grc/blocks" TYPE FILE FILES
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_avg_ff.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_avg_v.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_avg_v_ff.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_cal_csi.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_cal_multi_usrp.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_cor_inf.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_cor_pac_aoa.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_cp2offs.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_extract_lts.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_lts_cor.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_msg2cv.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_smooth_pi.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_unwrap.block.yml"
    "/mnt/hgfs/project/DeepAoA/DeeperAoA-main/项目代码/2.1gnuradio代码/mymods38/gr-MyMod/grc/MyMod_wifi_samples_align.block.yml"
    )
endif()

