INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_MYMOD MyMod)

FIND_PATH(
    MYMOD_INCLUDE_DIRS
    NAMES MyMod/api.h
    HINTS $ENV{MYMOD_DIR}/include
        ${PC_MYMOD_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    MYMOD_LIBRARIES
    NAMES gnuradio-MyMod
    HINTS $ENV{MYMOD_DIR}/lib
        ${PC_MYMOD_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/MyModTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MYMOD DEFAULT_MSG MYMOD_LIBRARIES MYMOD_INCLUDE_DIRS)
MARK_AS_ADVANCED(MYMOD_LIBRARIES MYMOD_INCLUDE_DIRS)
