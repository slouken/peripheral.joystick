cmake_minimum_required(VERSION 3.1)
project(peripheral.joystick C CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

SET(CMAKE_VERBOSE_MAKEFILE ON)

include(CheckIncludeFiles)

# --- Add-on Dependencies ------------------------------------------------------

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)
find_package(p8-platform REQUIRED)
find_package(PCRE REQUIRED)

include_directories(${INCLUDES}
                    ${PROJECT_SOURCE_DIR}/src
                    ${KODI_INCLUDE_DIR}
                    ${kodiplatform_INCLUDE_DIRS}
                    ${p8-platform_INCLUDE_DIRS}
                    ${PCRE_INCLUDE_DIRS}
)

set(JOYSTICK_SOURCES src/addon.cpp
                     src/api/AnomalousTrigger.cpp
                     src/api/Joystick.cpp
                     src/api/JoystickInterfaceCallback.cpp
                     src/api/JoystickManager.cpp
                     src/api/JoystickTranslator.cpp
                     src/api/PeripheralScanner.cpp
                     src/buttonmapper/ButtonMapper.cpp
                     src/buttonmapper/ButtonMapTranslator.cpp
                     src/buttonmapper/ButtonMapUtils.cpp
                     src/buttonmapper/ControllerTransformer.cpp
                     src/buttonmapper/DriverGeometry.cpp
                     src/buttonmapper/JoystickFamily.cpp
                     src/filesystem/DirectoryCache.cpp
                     src/filesystem/DirectoryUtils.cpp
                     src/filesystem/Filesystem.cpp
                     src/filesystem/FileUtils.cpp
                     src/filesystem/generic/ReadableFile.cpp
                     src/filesystem/generic/SeekableFile.cpp
                     src/filesystem/vfs/VFSDirectoryUtils.cpp
                     src/filesystem/vfs/VFSFileUtils.cpp
                     src/log/Log.cpp
                     src/log/LogAddon.cpp
                     src/log/LogConsole.cpp
                     src/settings/Settings.cpp
                     src/storage/ButtonMap.cpp
                     src/storage/Device.cpp
                     src/storage/DeviceConfiguration.cpp
                     src/storage/JustABunchOfFiles.cpp
                     src/storage/StorageManager.cpp
                     src/storage/StorageUtils.cpp
                     src/storage/api/DatabaseJoystickAPI.cpp
                     src/storage/xml/ButtonMapXml.cpp
                     src/storage/xml/DatabaseXml.cpp
                     src/storage/xml/DeviceXml.cpp
                     src/storage/xml/JoystickFamiliesXml.cpp
                     src/utils/StringUtils.cpp)

check_include_files("syslog.h" HAVE_SYSLOG)

if(HAVE_SYSLOG)
  list(APPEND JOYSTICK_SOURCES src/log/LogSyslog.cpp)
endif()

list(APPEND DEPLIBS ${kodiplatform_LIBRARIES}
                    ${p8-platform_LIBRARIES}
                    ${PCRE_LIBRARIES})
add_definitions(${PCRE_DEFINITIONS})

# --- Cocoa --------------------------------------------------------------------

if("${CORE_SYSTEM_NAME}" STREQUAL "darwin" OR "${CORE_SYSTEM_NAME}" STREQUAL "osx")
  find_library(COCOA_LIBRARY Cocoa)

  add_definitions(-DHAVE_COCOA)

  list(APPEND JOYSTICK_SOURCES src/api/cocoa/JoystickCocoa.cpp
                               src/api/cocoa/JoystickInterfaceCocoa.cpp)

  list(APPEND DEPLIBS ${COCOA_LIBRARY})
endif()

# --- Linux Joystick API -------------------------------------------------------

check_include_files(linux/joystick.h HAVE_LINUX_JOYSTICK_H)

if(HAVE_LINUX_JOYSTICK_H)
  add_definitions(-DHAVE_LINUX_JOYSTICK)

  list(APPEND JOYSTICK_SOURCES src/api/linux/JoystickInterfaceLinux.cpp
                               src/api/linux/JoystickLinux.cpp)
endif()

# --- DirectInput --------------------------------------------------------------

if("${CORE_SYSTEM_NAME}" STREQUAL "windows")
  add_definitions(-DHAVE_DIRECT_INPUT)

  list(APPEND JOYSTICK_SOURCES src/api/directinput/JoystickDirectInput.cpp
                               src/api/directinput/JoystickInterfaceDirectInput.cpp)
endif()

# --- XInput -------------------------------------------------------------------

if("${CORE_SYSTEM_NAME}" STREQUAL "windows")
  add_definitions(-DHAVE_XINPUT)

  list(APPEND JOYSTICK_SOURCES src/api/xinput/JoystickInterfaceXInput.cpp
                               src/api/xinput/JoystickXInput.cpp
                               src/api/xinput/XInputDLL.cpp)
endif()

# --- udev ---------------------------------------------------------------------

find_package(udev REQUIRED)
if(UDEV_FOUND)
  include_directories(${UDEV_INCLUDE_DIRS})

  add_definitions(-DHAVE_UDEV)

  list(APPEND JOYSTICK_SOURCES src/api/udev/JoystickInterfaceUdev.cpp
                               src/api/udev/JoystickUdev.cpp)

  list(APPEND DEPLIBS ${UDEV_LIBRARIES})
endif()

# --- SDL2 ---------------------------------------------------------------------

message(STATUS "")
message(STATUS "")
message(STATUS "")
message(STATUS "find_package:")
message(STATUS "CMAKE_FIND_ROOT_PATH: ${CMAKE_FIND_ROOT_PATH}")
message(STATUS "")

find_package(SDL2)
message(STATUS "SDL:")
message(STATUS "SDL2_FOUND: ${SDL2_FOUND}")
message(STATUS "SDL2_INCLUDE_DIR: ${SDL2_INCLUDE_DIR}")
message(STATUS "SDL2_LIBRARY: ${SDL2_LIBRARY}")
message(STATUS "CMAKE_USE_PTHREADS_INIT: ${CMAKE_USE_PTHREADS_INIT}")
message(STATUS "CMAKE_THREAD_LIBS_INIT: ${CMAKE_THREAD_LIBS_INIT}")
message(STATUS "")

if(SDL2_FOUND)
  FIND_LIBRARY(PTHREAD_LIBRARY
               NAMES pthread)
  FIND_LIBRARY(PTHREAD_NONSHARED_LIBRARY
               NAMES pthread_nonshared)

  MESSAGE(STATUS "pthread:")
  MESSAGE(STATUS "PTHREAD_LIBRARY is ${PTHREAD_LIBRARY}")
  MESSAGE(STATUS "PTHREAD_NONSHARED_LIBRARY is ${PTHREAD_NONSHARED_LIBRARY}")
  message(STATUS "")

  MESSAGE(STATUS "linker:")
  MESSAGE(STATUS "CMAKE_INSTALL_RPATH is ${CMAKE_INSTALL_RPATH}")
  message(STATUS "CMAKE_MODULE_LINKER_FLAGS: ${CMAKE_MODULE_LINKER_FLAGS}")
  message(STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")
  message(STATUS "CMAKE_STATIC_LINKER_FLAGS: ${CMAKE_STATIC_LINKER_FLAGS}")
  message(STATUS "")
  message(STATUS "")
  message(STATUS "")

  include_directories(${SDL2_INCLUDE_DIR})

  add_definitions(-DHAVE_SDL)

  list(APPEND JOYSTICK_SOURCES src/api/sdl/JoystickInterfaceSDL.cpp
                               src/api/sdl/JoystickSDL.cpp)

  list(APPEND DEPLIBS ${SDL2_LIBRARY}
                      ${PTHREAD_LIBRARY}
                      ${PTHREAD_NONSHARED_LIBRARY}
  )
endif()

# ------------------------------------------------------------------------------

build_addon(peripheral.joystick JOYSTICK DEPLIBS)

