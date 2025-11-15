cmake_minimum_required(VERSION 3.10)
project(cuems-videocomposer VERSION 0.1.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -O3 -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O3 -pthread")

# Get git version
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always --tags HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(GIT_VERSION "unknown")
endif()

add_definitions(-DSUBVERSION="${GIT_VERSION}")

# Platform detection
if(APPLE)
    set(PLATFORM_OSX 1)
    add_definitions(-DPLATFORM_OSX)
    set(PLATFORM_NAME "OSX")
elseif(WIN32)
    set(PLATFORM_WINDOWS 1)
    add_definitions(-DPLATFORM_WINDOWS)
    set(PLATFORM_NAME "Windows")
elseif(UNIX)
    set(PLATFORM_LINUX 1)
    add_definitions(-DPLATFORM_LINUX)
    set(PLATFORM_NAME "Linux")
endif()

# Required dependencies
find_package(PkgConfig REQUIRED)
include(CheckIncludeFiles)

# FFmpeg (required)
pkg_check_modules(FFMPEG REQUIRED 
    libavformat 
    libavcodec 
    libavutil 
    libswscale
)
add_definitions(${FFMPEG_CFLAGS})

# Optional dependencies with feature flags
option(ENABLE_XV "Enable Xvideo extension" ON)
option(ENABLE_OPENGL "Enable OpenGL display" ON)
option(ENABLE_IMLIB2 "Enable imlib2 display" ON)
option(ENABLE_SDL "Enable SDL display" ON)
option(ENABLE_FREETYPE "Enable freetype OSD support" ON)
option(ENABLE_MQ "Enable POSIX message queue remote control" ON)
option(ENABLE_IPC "Enable IPC message queue remote control" ON)
option(ENABLE_OSC "Enable OSC remote control" ON)
option(ENABLE_LTC "Enable LTC sync support" ON)
option(ENABLE_MIDI "Enable MIDI support" ON)
option(ENABLE_PORTMIDI "Enable portmidi support" ON)
option(ENABLE_ALSAMIDI "Enable ALSA MIDI support" ON)
option(ENABLE_MIDI_TEST "Enable MIDI testing with libmtcmaster" OFF)
option(ENABLE_XINERAMA "Enable Xinerama support" OFF)
option(ENABLE_CUEMS "Enable CUEMS mode" OFF)
option(ENABLE_WARP "Enable projection warping" OFF)
option(ENABLE_TIMESCALE "Enable time scaling" OFF)
option(ENABLE_FRAMECROP "Enable frame cropping" OFF)
option(ENABLE_EMBED_FONT "Embed font in binary" OFF)

# X11 (required on Linux/Unix)
if(PLATFORM_LINUX)
    find_package(X11 REQUIRED)
    if(X11_FOUND)
        include_directories(${X11_INCLUDE_DIR})
        add_definitions(-DHAVE_X11)
    endif()
    
    pkg_check_modules(XPM REQUIRED xpm)
    add_definitions(${XPM_CFLAGS})
    set(XPM_LIBS ${XPM_LIBRARIES})
endif()

# OpenGL
if(ENABLE_OPENGL)
    find_package(OpenGL)
    if(OpenGL_FOUND)
        add_definitions(-DHAVE_GL)
        if(PLATFORM_OSX)
            set(GL_LIBS "-framework Cocoa -framework OpenGL")
        elseif(PLATFORM_WINDOWS)
            set(GL_LIBS "-lopengl32 -lglu32 -lgdi32")
        else()
            find_package(GLU)
            if(GLU_FOUND)
                set(GL_LIBS "${GL_LIBS} -lGLU -lGL")
            endif()
        endif()
    endif()
endif()

# Xinerama
if(ENABLE_XINERAMA OR ENABLE_CUEMS)
    if(PLATFORM_LINUX)
        pkg_check_modules(DPY_XINERAMA xinerama)
        if(DPY_XINERAMA_FOUND)
            add_definitions(-DHAVE_LIBXINERAMA)
        endif()
    endif()
endif()

# Xv
if(ENABLE_XV AND PLATFORM_LINUX)
    find_library(XV_LIB Xv)
    if(XV_LIB)
        add_definitions(-DHAVE_LIBXV)
        set(XV_LIBS ${XV_LIB})
    endif()
endif()

# SDL
if(ENABLE_SDL)
    pkg_check_modules(SDL sdl)
    if(SDL_FOUND)
        add_definitions(-DHAVE_SDL)
    endif()
endif()

# imlib2
if(ENABLE_IMLIB2 AND PLATFORM_LINUX)
    pkg_check_modules(IMLIB2 imlib2)
    if(IMLIB2_FOUND)
        add_definitions(-DHAVE_IMLIB2)
        add_definitions(-DIMLIB2RGBA)
    endif()
endif()

# Freetype
if(ENABLE_FREETYPE)
    pkg_check_modules(FREETYPE freetype2)
    if(FREETYPE_FOUND)
        add_definitions(-DHAVE_FT)
        add_definitions(${FREETYPE_CFLAGS})
        set(FREETYPE_LIBS ${FREETYPE_LIBRARIES})
    else()
        # Fallback: try to find freetype directly
        find_library(FREETYPE_LIB freetype)
        if(FREETYPE_LIB)
            set(FREETYPE_LIBS ${FREETYPE_LIB})
        endif()
    endif()
endif()


# Message Queue
if(ENABLE_MQ)
    find_library(RT_LIB rt)
    if(RT_LIB)
        add_definitions(-DHAVE_MQ)
        if(NOT DEFINED MQ_LIBS)
            set(MQ_LIBS "")
        endif()
        list(APPEND MQ_LIBS ${RT_LIB})
    endif()
endif()

# IPC Message Queue
if(ENABLE_IPC AND NOT ENABLE_MQ)
    check_include_files("sys/ipc.h;sys/msg.h" HAVE_IPCMSG)
    if(HAVE_IPCMSG)
        add_definitions(-DHAVE_IPCMSG)
    endif()
endif()

# OSC (liblo)
if(ENABLE_OSC)
    pkg_check_modules(LIBLO liblo)
    if(LIBLO_FOUND)
        add_definitions(-DHAVE_LIBLO)
        set(LIBLO_LIBS ${LIBLO_LIBRARIES})
    else()
        # Fallback: try to find liblo directly
        find_library(LIBLO_LIB lo)
        if(LIBLO_LIB)
            set(LIBLO_LIBS ${LIBLO_LIB})
        endif()
    endif()
endif()

# LTC
if(ENABLE_LTC)
    pkg_check_modules(LTC libltc)
    if(LTC_FOUND)
        add_definitions(-DHAVE_LTC)
    endif()
endif()

# MIDI
if(ENABLE_MIDI)
    add_definitions(-DHAVE_MIDI)
    
    
    # PortMidi
    if(ENABLE_PORTMIDI)
        find_library(PORTMIDI_LIB portmidi)
        if(PORTMIDI_LIB)
            add_definitions(-DHAVE_PORTMIDI)
        endif()
    endif()
    
    # ALSA MIDI (Sequencer only)
    if(ENABLE_ALSAMIDI AND PLATFORM_LINUX)
        pkg_check_modules(ALSA asound)
        if(ALSA_FOUND)
            add_definitions(-DALSA_SEQ_MIDI)
            # Note: ALSA Raw MIDI not being implemented
            set(ALSA_LIBS ${ALSA_LIBRARIES})
        else()
            # Fallback: try to find ALSA library directly
            find_library(ALSA_LIB asound PATHS /usr/lib /usr/local/lib)
            if(ALSA_LIB)
                set(ALSA_FOUND TRUE)
                set(ALSA_LIBS ${ALSA_LIB})
                add_definitions(-DALSA_SEQ_MIDI)
                message(STATUS "Found ALSA: ${ALSA_LIB}")
            endif()
        endif()
    endif()
    
    # RtMidi (required for mtcreceiver)
    if(PLATFORM_LINUX)
        # Try to find RtMidi
        find_path(RTMIDI_INCLUDE_DIR rtmidi/RtMidi.h
            PATHS
                /usr/include
                /usr/local/include
                /opt/local/include
        )
        find_library(RTMIDI_LIBRARY
            NAMES rtmidi librtmidi
            PATHS
                /usr/lib
                /usr/local/lib
                /opt/local/lib
        )
        if(RTMIDI_INCLUDE_DIR AND RTMIDI_LIBRARY)
            set(RTMIDI_FOUND TRUE)
            set(RTMIDI_INCLUDE_DIRS ${RTMIDI_INCLUDE_DIR})
            set(RTMIDI_LIBRARIES ${RTMIDI_LIBRARY})
            message(STATUS "Found RtMidi: ${RTMIDI_LIBRARY}")
        else()
            # Try pkg-config as fallback
            pkg_check_modules(RTMIDI rtmidi)
            if(RTMIDI_FOUND)
                set(RTMIDI_INCLUDE_DIRS ${RTMIDI_INCLUDE_DIRS})
                set(RTMIDI_LIBRARIES ${RTMIDI_LIBRARIES})
            endif()
        endif()
    endif()
    
    # libmtcmaster for MTC testing
    if(ENABLE_ALSAMIDI AND PLATFORM_LINUX)
        # Try pkg-config first
        pkg_check_modules(MTCMASTER libmtcmaster)
        if(MTCMASTER_FOUND)
            add_definitions(-DHAVE_LIBMTCMASTER)
            set(MTCMASTER_LIBS ${MTCMASTER_LIBRARIES})
            if(MTCMASTER_INCLUDE_DIRS)
                include_directories(${MTCMASTER_INCLUDE_DIRS})
            endif()
        else()
            # Fallback: check if library exists directly
            # Try different library name variations
            find_library(MTCMASTER_LIB 
                NAMES mtcmaster libmtcmaster.so.0 libmtcmaster.so.0.1
                PATHS /usr/local/lib /usr/lib
            )
            if(MTCMASTER_LIB)
                # Find header directory (libmtcmaster repository location)
                # Check common locations: repository sibling, /usr/local/include, /usr/include
                find_path(MTCMASTER_INCLUDE_DIR interface.h 
                    PATHS 
                    ${CMAKE_SOURCE_DIR}/../libmtcmaster
                    ${CMAKE_SOURCE_DIR}/../../libmtcmaster
                    /home/ion/src/cuems/libmtcmaster
                    /usr/local/include
                    /usr/include
                )
                if(MTCMASTER_INCLUDE_DIR)
                    add_definitions(-DHAVE_LIBMTCMASTER)
                    set(MTCMASTER_LIBS ${MTCMASTER_LIB})
                    include_directories(${MTCMASTER_INCLUDE_DIR})
                    # Mark as found if both library and headers are available
                    set(MTCMASTER_FOUND TRUE)
                    message(STATUS "Found libmtcmaster: ${MTCMASTER_LIB}")
                    message(STATUS "  Headers: ${MTCMASTER_INCLUDE_DIR}")
                else()
                    message(WARNING "libmtcmaster library found but headers not found. Install headers or set MTCMASTER_INCLUDE_DIR")
                endif()
            endif()
        endif()
    endif()
endif()

# CUEMS mode
if(ENABLE_CUEMS)
    add_definitions(-DCUEMS)
    add_definitions(-DWARP)
    add_definitions(-DDISABLE_OSD)
    if(NOT ENABLE_XINERAMA)
        set(ENABLE_XINERAMA ON)
    endif()
endif()

# WARP
if(ENABLE_WARP)
    add_definitions(-DWARP)
endif()

# TIMESCALE
if(ENABLE_TIMESCALE)
    add_definitions(-DTIMEMAP)
endif()

# FRAMECROP
if(ENABLE_FRAMECROP)
    add_definitions(-DCROPIMG)
endif()

# Standard definitions
add_definitions(-DCFG_WARN_ONLY)
# add_definitions(-DDND)  # Drag and drop removed
add_definitions(-DWINMENU)
# add_definitions(-DXDLG)  # Menu removed
add_definitions(-DXFIB)
add_definitions(-DCOLOREQ)

if(NOT ENABLE_CUEMS)
    add_definitions(-DDRAW_CROSS)
endif()

# Data directory
set(DATADIR "${CMAKE_INSTALL_PREFIX}/share")
add_definitions(-DSHAREDIR="${DATADIR}")

# Font file path
if(ENABLE_EMBED_FONT)
    add_definitions(-DWITH_EMBEDDED_FONT)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/cuems_videocomposer)
include_directories(${CMAKE_SOURCE_DIR}/src/cuems_videocomposer/cpp)

# Source files for existing C code (will be gradually replaced)
# Note: main.c removed - using cpp/main.cpp instead
set(C_SOURCES
    # src/cuems_videocomposer/video_globals.c - Consolidated into common.c
    # src/cuems_videocomposer/xjadeo.c - Replaced by VideoFileInput, VideoComposerApplication, VideoLayer
    # src/cuems_videocomposer/remote.c - Replaced by RemoteControl/OSCRemoteControl
    # src/cuems_videocomposer/mqueue.c - Message queue remote control (not yet migrated)
    # src/cuems_videocomposer/xjosc.c - Replaced by OSCRemoteControl
    # src/cuems_videocomposer/configfile.c - Replaced by ConfigurationManager
    src/cuems_videocomposer/common.c
    # src/cuems_videocomposer/ltc-jack.c - Removed (JACK support removed, not used by C++ codebase)
    # src/cuems_videocomposer/midi.c - Removed (C++ version complete and tested, not used by C++ codebase)
    src/cuems_videocomposer/freetype.c
    # src/cuems_videocomposer/smpte.c - Replaced by cpp/utils/SMPTEUtils.cpp + SMPTEWrapper.cpp
    # smpte.c removed - C code now uses C++ implementation via SMPTEWrapper
    src/cuems_videocomposer/display.c
    # src/cuems_videocomposer/display_x_dnd.c - Removed (drag and drop functionality removed)
    # src/cuems_videocomposer/display_x_dialog.c - Removed (menu functionality removed)
    src/cuems_videocomposer/libsofd.c
    # src/cuems_videocomposer/display_mac.c - Removed (Linux-only project, macOS not supported)
    src/cuems_videocomposer/display_x11.c
    # src/cuems_videocomposer/display_sdl.c - Removed (SDL display not being migrated, Linux-only project)
    # src/cuems_videocomposer/gtime.c - Replaced by cpp/utils/TimeUtils.cpp
)

# Platform-specific C sources
if(PLATFORM_LINUX OR PLATFORM_NETBSD)
    list(APPEND C_SOURCES src/cuems_videocomposer/display_glx.c)
endif()

# Platform-specific files removed (Linux-only project)
# if(PLATFORM_WINDOWS)
#     list(APPEND C_SOURCES src/cuems_videocomposer/display_gl_win.c src/cuems_videocomposer/windows.rc)
# endif()
# 
# if(PLATFORM_OSX)
#     list(APPEND C_SOURCES src/cuems_videocomposer/display_gl_osx.m)
# endif()

# C++ sources
set(CPP_SOURCES
    src/cuems_videocomposer/cpp/main.cpp
    src/cuems_videocomposer/cpp/VideoComposerApplication.cpp
    src/cuems_videocomposer/cpp/config/ConfigurationManager.cpp
    src/cuems_videocomposer/cpp/input/VideoFileInput.cpp
    src/cuems_videocomposer/cpp/sync/MIDISyncSource.cpp
    src/cuems_videocomposer/cpp/sync/MIDIDriver.cpp
    src/cuems_videocomposer/cpp/sync/MTCDecoder.cpp
    src/cuems_videocomposer/cpp/sync/ALSASeqMIDIDriver.cpp
    src/cuems_videocomposer/cpp/video/FrameBuffer.cpp
    src/cuems_videocomposer/cpp/layer/VideoLayer.cpp
    src/cuems_videocomposer/cpp/layer/LayerManager.cpp
    src/cuems_videocomposer/cpp/display/OpenGLDisplay.cpp
    src/cuems_videocomposer/cpp/display/OpenGLRenderer.cpp
    src/cuems_videocomposer/cpp/display/DisplayManager.cpp
    src/cuems_videocomposer/cpp/display/XineramaHelper.cpp
    src/cuems_videocomposer/cpp/remote/OSCRemoteControl.cpp
    src/cuems_videocomposer/cpp/remote/RemoteCommandRouter.cpp
    src/cuems_videocomposer/cpp/osd/OSDManager.cpp
    src/cuems_videocomposer/cpp/osd/OSDRenderer.cpp
    src/cuems_videocomposer/cpp/utils/SMPTEUtils.cpp
    src/cuems_videocomposer/cpp/utils/SMPTEWrapper.cpp
    src/cuems_videocomposer/cpp/utils/TimeUtils.cpp
    src/cuems_videocomposer/cpp/utils/MIDIBridge.cpp
    # Note: Logger.h is header-only, no .cpp needed
)

# Add mtcreceiver submodule
if(ENABLE_MIDI AND RTMIDI_FOUND)
    add_subdirectory(src/mtcreceiver)
    add_definitions(-DHAVE_MTCRECEIVER)
    # Add mtcreceiver driver adapter
    list(APPEND CPP_SOURCES
        src/cuems_videocomposer/cpp/sync/MtcReceiverMIDIDriver.cpp
    )
endif()

# Create executable
add_executable(cuems-videocomposer ${C_SOURCES} ${CPP_SOURCES})

# Test executable (optional)
option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS)
    # Test source files
    set(TEST_SOURCES
        src/cuems_videocomposer/cpp/test/TestFramework.h
        src/cuems_videocomposer/cpp/test/TestMain.cpp
        src/cuems_videocomposer/cpp/test/TestLayerManager.cpp
        src/cuems_videocomposer/cpp/test/TestVideoLayer.cpp
        src/cuems_videocomposer/cpp/test/TestConfigurationManager.cpp
        src/cuems_videocomposer/cpp/test/TestIntegration.cpp
        src/cuems_videocomposer/cpp/test/TestMTCDecoder.cpp
    )
    
    # C++ implementation files needed by tests
    set(TEST_CPP_SOURCES
        src/cuems_videocomposer/cpp/config/ConfigurationManager.cpp
        src/cuems_videocomposer/cpp/layer/VideoLayer.cpp
        src/cuems_videocomposer/cpp/layer/LayerManager.cpp
        src/cuems_videocomposer/cpp/sync/MTCDecoder.cpp
        src/cuems_videocomposer/cpp/video/FrameBuffer.cpp
        src/cuems_videocomposer/cpp/input/VideoFileInput.cpp
        src/cuems_videocomposer/cpp/utils/TimeUtils.cpp
        src/cuems_videocomposer/cpp/utils/SMPTEUtils.cpp
    )
    
    add_executable(cuems_videocomposer_test ${TEST_SOURCES} ${TEST_CPP_SOURCES} src/cuems_videocomposer/common.c)
    target_include_directories(cuems_videocomposer_test PRIVATE
        src/cuems_videocomposer
        src/cuems_videocomposer/cpp
    )
    target_link_libraries(cuems_videocomposer_test
        ${FFMPEG_LIBRARIES}
        ${ALSA_LIBS}
        m
        pthread
    )
    
    # Add test to CTest
    enable_testing()
    add_test(NAME cuems_videocomposer_unit_tests COMMAND cuems_videocomposer_test)
    
    # MIDI test with libmtcmaster (if available and enabled)
    if(ENABLE_MIDI_TEST AND MTCMASTER_FOUND AND ALSA_FOUND)
        set(MIDI_TEST_SOURCES
            src/cuems_videocomposer/cpp/test/TestMIDI.cpp
        )
        
        # C++ implementation files needed by MIDI test
        set(MIDI_TEST_CPP_SOURCES
            src/cuems_videocomposer/cpp/sync/MTCDecoder.cpp
            src/cuems_videocomposer/cpp/sync/ALSASeqMIDIDriver.cpp
            src/cuems_videocomposer/cpp/sync/MIDIDriver.cpp
            src/cuems_videocomposer/cpp/utils/TimeUtils.cpp
        )
        
        add_executable(cuems_videocomposer_midi_test ${MIDI_TEST_SOURCES} ${MIDI_TEST_CPP_SOURCES})
        target_include_directories(cuems_videocomposer_midi_test PRIVATE
            src/cuems_videocomposer
            src/cuems_videocomposer/cpp
            ${MTCMASTER_INCLUDE_DIR}
        )
        target_link_libraries(cuems_videocomposer_midi_test
            ${ALSA_LIBS}
            ${MTCMASTER_LIBS}
            m
            pthread
        )
        
        # Add test to CTest
        add_test(NAME cuems_videocomposer_midi_test COMMAND cuems_videocomposer_midi_test)
    endif()
endif()

# Initialize ALSA_LIBS if not set
if(NOT DEFINED ALSA_LIBS)
    set(ALSA_LIBS "")
endif()

# Link libraries
target_link_libraries(cuems-videocomposer
    ${FFMPEG_LIBRARIES}
    ${ALSA_LIBS}
    ${XV_LIBS}
    ${FREETYPE_LIBS}
    ${IMLIB2_LIBS}
    ${XPM_LIBS}
    ${LIBLO_LIBS}
    ${SDL_LIBS}
    ${LTC_LIBS}
    ${GL_LIBS}
    ${MQ_LIBS}
    ${DPY_XINERAMA_LIBS}
    ${RTMIDI_LIBRARIES}
    m
    pthread
)

# Link mtcreceiver if available
if(ENABLE_MIDI AND RTMIDI_FOUND)
    target_link_libraries(cuems-videocomposer mtcreceiver)
    target_include_directories(cuems-videocomposer PRIVATE
        src/mtcreceiver
        ${RTMIDI_INCLUDE_DIRS}
    )
endif()

if(PLATFORM_LINUX)
    target_link_libraries(cuems-videocomposer ${X11_LIBRARIES})
    # GLX is part of libGL, but we need to ensure it's linked
    find_library(GL_LIBRARY GL)
    if(GL_LIBRARY)
        target_link_libraries(cuems-videocomposer ${GL_LIBRARY})
    endif()
endif()

if(PLATFORM_WINDOWS)
    target_link_libraries(cuems-videocomposer ws2_32 winmm comdlg32)
endif()

if(PLATFORM_OSX)
    target_link_libraries(cuems-videocomposer dl)
endif()

# Install
install(TARGETS cuems-videocomposer DESTINATION bin)
install(FILES src/cuems_videocomposer/fonts/ArdourMono.ttf DESTINATION share/cuems-videocomposer)

# Clean targets
# Standard CMake clean: 'make clean' removes build artifacts (objects, executables) but keeps CMake cache
# Distclean: 'make distclean' removes both build artifacts and CMake cache for a completely fresh build
add_custom_target(distclean
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/Makefile
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CTestTestfile.cmake
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/Testing
    COMMENT "Removing CMake cache and all build artifacts"
)

