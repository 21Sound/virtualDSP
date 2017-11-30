cmake_minimum_required(VERSION 3.0)
project(virtualDSP)

#New policy, is a good idea to perform the qt search with nice behaviour
if(POLICY CMP0020)
    cmake_policy(SET CMP0020 NEW)
endif()

#Using c++11 standard
set (CMAKE_CXX_STANDARD 11)

#Include the current directory because of MOC-compilation (Qt related code wrapper)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#Activate MOC compilation for Qt signal and slots (plus other Qt related things)
set(CMAKE_AUTOMOC ON)

#Audio dependencies include directory
find_path(PORTAUDIO_PATH "portaudio" HINTS "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
set(PORTAUDIO_PATH "${PORTAUDIO_PATH}/portaudio")

message(STATUS "Qt qmake executable lays in: ${QT_QMAKE_EXECUTABLE} (not needed by this project)")
message(STATUS "Compiler identification: ${CMAKE_CXX_COMPILER_ID}")

#OS dependent library searches / includes
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        find_library(STDCPP_LIB libstdc++-6.dll libstdc++)
        find_library(WINPTHREAD_LIB libwinpthread-1.dll libwinpthread)
		find_library(QT5CORE_LIB Qt5Core.dll)
		find_library(QT5GUI_LIB Qt5Gui.dll)
		find_library(QT5WIDGETS_LIB Qt5Widgets.dll)
		find_library(QT5PRINT_LIB Qt5PrintSupport.dll)
	endif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")

    if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        message(STATUS "Target is Windows, 64 bit toolchain")

        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            find_library(SEH_LIB libgcc_s_seh-1.dll)
        endif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")

        find_library(PORTAUDIO_LIB portaudio HINTS "${PORTAUDIO_PATH}/win64")

    else("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        message(STATUS "Target is Windows, 32 bit toolchain")

        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            find_library(DW2_LIB libgcc_s_dw2-1)
        endif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")

        find_library(PORTAUDIO_LIB portaudio HINTS "${PORTAUDIO_PATH}/win32")
    endif("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message(STATUS "Target is Linux, 64 bit toolchain")
    find_library(PORTAUDIO_LIB portaudio)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(STATUS "Target is Darwin (Max OS X), 64 bit toolchain")
    find_library(PORTAUDIO_LIB portaudio)
else(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    message(SEND_ERROR "Unknown system name (does not equal Windows, Linux or Darwin [Mac OS X])")

endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

#Find qt related things (libraries and include directories)
find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)
find_package(Qt5PrintSupport)

#Include all the defined and resolved include paths
include_directories("${PORTAUDIO_PATH}/include" ${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${Qt5PrintSupport_INCLUDE_PATH})

#Set list of source files (.h) gets included automatically
set(SOURCES
    main.cpp
    CppDSP.cpp
    CppDSP.h
    CppRTA.cpp
    CppRTA.h
    complex_float32.h
    complex_float64.h
    fft.cpp
    fft.h
    mainwindow.cpp
    mainwindow.h
    paramWidget.cpp
    paramWidget.h
    qcustomplot.cpp
    qcustomplot.h
    )

#Add the executable
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
else(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_executable(${PROJECT_NAME} ${SOURCES})
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_library(WINMM_LIB winmm)
    find_library(OLE32_LIB ole32)
    find_library(UUID_LIB uuid)
    find_library(SETUPAPI_LIB setupapi)
    target_link_libraries(${PROJECT_NAME} ${PORTAUDIO_LIB} -lwinmm -lole32 -luuid -lsetupapi)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_library(ASOUND_LIB asound)
    find_library(PTHREAD_LIB pthread)
    find_package(Jack)
    if(JACK_FOUND MATCHES TRUE)
        message(STATUS "Supporting Jack audio library")
        target_link_libraries(${PROJECT_NAME} ${PORTAUDIO_LIB} ${JACK_LIBRARIES} ${ASOUND_LIB} ${PTHREAD_LIB})
    else(JACK_FOUND MATCHES TRUE)
        message(STATUS "Jack not found. Using ALSA directly")
        option(PA_USE_JACK "Enable support for Jack" OFF)
        target_link_libraries(${PROJECT_NAME} ${PORTAUDIO_LIB} ${ASOUND_LIB} ${PTHREAD_LIB})
    endif(JACK_FOUND MATCHES TRUE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(COREAUDIO_LIB CoreAudio)
    find_library(AUDIOTOOLBOX_LIB AudioToolbox)
    find_library(AUDIOUNIT_LIB AudioUnit)
    find_library(CARBON_LIB Carbon)
    target_link_libraries(${PROJECT_NAME} ${PORTAUDIO_LIB} ${COREAUDIO_LIB} ${AUDIOTOOLBOX_LIB} ${AUDIOUNIT_LIB} ${CARBON_LIB})
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

target_link_libraries(${PROJECT_NAME} ${Qt5Core_LIBRARIES} ${Qt5Gui_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5PrintSupport_LIBRARIES})

#Copy all related dynamic libraries to the binary folder if we are on windows with GNU compliler
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            file(COPY ${SEH_LIB} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
        else("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
            file(COPY ${DW2_LIB} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
        endif("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
		file(COPY ${STDCPP_LIB} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
		file(COPY ${WINPTHREAD_LIB} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
		file(COPY ${PORTAUDIO_LIB} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

		#Copy shared qt bibs with the executable, so that it can run standalone
		find_library(QWINDOWS qwindows.dll qwindows HINTS "${QT_QMAKE_EXECUTABLE}/../../plugins/platforms")
		file(COPY ${QWINDOWS} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/platforms")
		file(COPY ${QT5PRINT_LIB} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
		file(COPY ${QT5CORE_LIB} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
		file(COPY ${QT5GUI_LIB} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
		file(COPY ${QT5WIDGETS_LIB} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
	endif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

#copy symbols and testing data to the binary directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/symbols" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#End of file
