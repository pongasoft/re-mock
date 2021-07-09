# Copyright (c) 2020-2021 pongasoft
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# @author Yan Pujante

# The purpose of this file is to be able to work on re-common... include re-common.cmake in your project instead!

cmake_minimum_required(VERSION 3.13)

# Setting up project (note that the version is defined in lua.info)
project(re-common LANGUAGES CXX)

# Using C++17
set(CMAKE_CXX_STANDARD 17)

include(re-common.cmake)

# Using RE SDK version 4.2.0
set(RE_SDK_VERSION 4.2.0)

# Location of RE SDK: can be set when invoking cmake => cmake -D "RE_SDK_ROOT:PATH=/path/to/re_sdk"
# or via -p option in configure.py script or in cmake-gui
if(APPLE)
  set(RE_SDK_ROOT "/Users/Shared/ReasonStudios/JukeboxSDK_${RE_SDK_VERSION}/SDK" CACHE PATH "Location of RE SDK")
else()
  set(RE_SDK_ROOT "C:/Users/Public/Documents/ReasonStudios/JukeboxSDK_${RE_SDK_VERSION}/SDK" CACHE PATH "Location of RE SDK")
endif()

set(target "re-common")

add_library(${target} STATIC "${RE_COMMON_CPP_SRC_DIR}/jbox.cpp" ${re-common_BUILD_HEADERS} ${re-common_NATIVE_BUILD_SOURCES} ${re-common_EXPERIMENTAL_BUILD_HEADERS} ${re-common_EXPERIMENTAL_BUILD_SOURCES} "${re-common_MOCK_BUILD_SOURCES}")
target_compile_definitions(${target} PUBLIC LOCAL_NATIVE_BUILD=1 JUKEBOX_SDK=0 DEBUG=1)
target_include_directories(${target} PUBLIC "${RE_SDK_ROOT}/API" ${re-common_INCLUDE_DIRECTORIES}) # exporting SDK API to plugin

#######################################################
# Testing
#######################################################
# Download and unpack googletest at configure time
include(fetch-GoogleTest.cmake)
include(GoogleTest)

set(target_test "${target}_test")

set(RE_COMMON_CPP_TST_DIR "${CMAKE_CURRENT_LIST_DIR}/test/cpp")

set(TEST_CASE_SOURCES
    "${RE_COMMON_CPP_TST_DIR}/TestStaticString.cpp"
    "${RE_COMMON_CPP_TST_DIR}/re/TestProperty.cpp"
    )

add_executable("${target_test}" "${TEST_CASE_SOURCES}")
target_link_libraries("${target_test}" gtest_main ${target})
target_include_directories("${target_test}" PUBLIC "${PROJECT_SOURCE_DIR}" "${GTEST_INCLUDE_DIRS}")
