cmake_minimum_required(VERSION 3.6)

INCLUDE (CheckIncludeFile)
INCLUDE (CheckIncludeFiles)
INCLUDE (CheckSymbolExists)
INCLUDE (CheckFunctionExists)
INCLUDE (FeatureSummary)

project(cwordle C)

# To find config.h
include_directories("${PROJECT_BINARY_DIR}")

# set path to additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

set(CMAKE_COLOR_MAKEFILE ON)
#set(CMAKE_C_STANDARD 18)
if ( NOT( DEFINED DATA_DIR ) )
	set(DATA_DIR ${CMAKE_INSTALL_PREFIX}/share/cwordle)
endif()

configure_file(config.h.in config.h)

add_executable(cwordle cwordle.c)

SET(CMAKE_C_FLAGS		"${CMAKE_C_FLAGS} -std=c18")
SET(CMAKE_C_FLAGS		"${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

install (TARGETS cwordle DESTINATION bin)
install (FILES dict.txt possible.txt DESTINATION share/cwordle) 

FEATURE_SUMMARY(WHAT ALL)
