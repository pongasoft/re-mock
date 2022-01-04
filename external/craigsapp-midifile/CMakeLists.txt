cmake_minimum_required(VERSION 3.17)

project(midifile C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(midifile_CPP_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

message(STATUS "midifile_CPP_INCLUDE_DIR=${midifile_CPP_INCLUDE_DIR}")

###############################
###
### Operating-system specific settings:
###
#
#if(MSVC)
#  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
#
#  option(STATIC_CRT "Use static CRT libraries" OFF)
#
#  # Rewrite command line flags to use /MT if necessary
#  if(STATIC_CRT)
#    foreach(flag_var
#        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
#        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
#      if(${flag_var} MATCHES "/MD")
#        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
#      endif(${flag_var} MATCHES "/MD")
#    endforeach(flag_var)
#  endif()
#endif()
#

##############################
##
## Library:
##

set(SRCS
    src/Binasc.cpp
    src/MidiEvent.cpp
    src/MidiEventList.cpp
    src/MidiFile.cpp
    src/MidiMessage.cpp
    )

set(HDRS
    include/Binasc.h
    include/MidiEvent.h
    include/MidiEventList.h
    include/MidiFile.h
    include/MidiMessage.h
    )

add_library(midifile STATIC ${SRCS} ${HDRS})
target_include_directories(midifile PUBLIC "${midifile_CPP_INCLUDE_DIR}")

