# Copyright (c) 2021 pongasoft
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

# The purpose of this file is to be able to work on re-mock... include re-mock.cmake in your project instead!

cmake_minimum_required(VERSION 3.17)

# Setting up project (note that the version is defined in lua.info)
project(re-mock LANGUAGES CXX)

# Using C++17
set(CMAKE_CXX_STANDARD 17)

# Determines whether we are working on re-mock or using it
if(${PROJECT_NAME} STREQUAL ${CMAKE_PROJECT_NAME})
  set(RE_MOCK_DEV_BUILD TRUE)
else()
  set(RE_MOCK_DEV_BUILD FALSE)
endif()

if(RE_MOCK_DEV_BUILD)
  enable_testing()

  # Using RE SDK version 4.2.0
  set(RE_SDK_VERSION 4.2.0)

  # Location of RE SDK: can be set when invoking cmake => cmake -D "RE_SDK_ROOT:PATH=/path/to/re_sdk"
  # or via -p option in configure.py script or in cmake-gui
  if(APPLE)
    set(RE_SDK_ROOT "/Users/Shared/ReasonStudios/JukeboxSDK_${RE_SDK_VERSION}/SDK" CACHE PATH "Location of RE SDK")
  else()
    set(RE_SDK_ROOT "C:/Users/Public/Documents/ReasonStudios/JukeboxSDK_${RE_SDK_VERSION}/SDK" CACHE PATH "Location of RE SDK")
  endif()
else()
  if(NOT RE_SDK_ROOT)
    message(ERROR "RE_SDK_ROOT must be defined")
  endif()
endif()

set(target "re-mock")

# Defines the location of the sources
set(RE_MOCK_CPP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src/cpp")

# Defines the headers if you want to include them in your project (optional)
set(re-mock_BUILD_HEADERS
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/Config.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/DeviceTesters.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/Errors.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/LuaJBox.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/MockDevices.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/Motherboard.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/MotherboardImpl.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/Rack.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/fmt.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/stl.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/lua/LuaState.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/lua/MockJBox.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/lua/MotherboardDef.h
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/lua/RealtimeController.h
    )

# Defines the sources
set(re-mock_BUILD_SOURCES
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/DeviceTesters.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/Rack.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/Jukebox.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/LuaJBox.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/MockDevices.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/Motherboard.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/lua/LuaState.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/lua/MockJBox.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/lua/MotherboardDef.cpp
    ${RE_MOCK_CPP_SRC_DIR}/re/mock/lua/RealtimeController.cpp
    )

# Defines the include directories
set(re-mock_INCLUDE_DIRECTORIES "${RE_MOCK_CPP_SRC_DIR}")

# Including lua lib/headers
add_subdirectory(external/lua-cmake)

add_library(${target} STATIC "${re-mock_BUILD_HEADERS}" "${re-mock_BUILD_SOURCES}")
target_compile_definitions(${target} PUBLIC LOCAL_NATIVE_BUILD=1 JUKEBOX_SDK=0 DEBUG=1)
target_include_directories(${target} PUBLIC "${RE_SDK_ROOT}/API" "${re-mock_INCLUDE_DIRECTORIES}") # exporting SDK API to plugin
target_link_libraries(${target} PUBLIC lua::lib lua::header)

if(RE_MOCK_DEV_BUILD)

  # Generate the re_cmake_build.h file
  set(GENERATED_FILES_DIR "${CMAKE_BINARY_DIR}/generated")
  file(TO_NATIVE_PATH "${CMAKE_CURRENT_LIST_DIR}" PROJECT_DIR_NATIVE_PATH)
  file(TO_NATIVE_PATH "${RE_SDK_ROOT}" RE_SDK_ROOT_NATIVE_PATH)
  configure_file("${CMAKE_CURRENT_LIST_DIR}/re_mock_build.h.in" "${GENERATED_FILES_DIR}/re_mock_build.h")

  #######################################################
  # Testing
  #######################################################
  # Download and unpack googletest at configure time
  include(fetch-GoogleTest.cmake)
  include(GoogleTest)

  set(target_test "${target}_test")

  set(RE_MOCK_CPP_TST_DIR "${CMAKE_CURRENT_LIST_DIR}/test/cpp")

  set(TEST_CASE_SOURCES
      "${RE_MOCK_CPP_TST_DIR}/re/mock/TestDeviceTesters.cpp"
      "${RE_MOCK_CPP_TST_DIR}/re/mock/TestJukebox.cpp"
      "${RE_MOCK_CPP_TST_DIR}/re/mock/TestRack.cpp"
      "${RE_MOCK_CPP_TST_DIR}/re/mock/TestRackExtension.cpp"
      "${RE_MOCK_CPP_TST_DIR}/re/mock/lua/TestLuaState.cpp"
      "${RE_MOCK_CPP_TST_DIR}/re/mock/lua/TestMotherboardDef.cpp"
      "${RE_MOCK_CPP_TST_DIR}/re/mock/lua/TestRealtimeController.cpp"
      )

  add_executable("${target_test}" "${TEST_CASE_SOURCES}")
  target_link_libraries("${target_test}" gtest_main gmock ${target})
  target_include_directories("${target_test}" PUBLIC "${PROJECT_SOURCE_DIR}" "${GENERATED_FILES_DIR}")

  gtest_discover_tests("${target_test}")
endif()
