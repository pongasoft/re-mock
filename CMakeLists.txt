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

cmake_minimum_required(VERSION 3.17)

# Setting up project
project(re-mock LANGUAGES CXX)
set(re-mock_VERSION_MAJOR 1)
set(re-mock_VERSION_MINOR 8)
set(re-mock_VERSION_PATCH 0)
set(re-mock_VERSION "${re-mock_VERSION_MAJOR}.${re-mock_VERSION_MINOR}.${re-mock_VERSION_PATCH}")

# Using C++17
set(CMAKE_CXX_STANDARD 17)

# Determines whether we are working on re-mock or using it
if(${PROJECT_NAME} STREQUAL ${CMAKE_PROJECT_NAME})
  set(re-mock_DEV_BUILD TRUE)
else()
  set(re-mock_DEV_BUILD FALSE)
endif()

# The SDK (api only) is part of this project
set(RE_MOCK_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/external/ReasonStudios/JukeboxSDK_4.6.0/SDK")

# In dev build we enable testing
if(re-mock_DEV_BUILD)
  enable_testing()

  # Using RE SDK version 4.6.0
  set(RE_SDK_VERSION 4.6.0)

  # Location of RE SDK: can be set when invoking cmake => cmake -D "RE_SDK_ROOT:PATH=/path/to/re_sdk"
  # or via -p option in configure.py script or in cmake-gui
  if(APPLE)
    set(RE_LOCAL_SDK_ROOT "/Users/Shared/ReasonStudios/JukeboxSDK_${RE_SDK_VERSION}/SDK")
  else()
    set(RE_LOCAL_SDK_ROOT "C:/Users/Public/Documents/ReasonStudios/JukeboxSDK_${RE_SDK_VERSION}/SDK")
  endif()
  if(EXISTS "${RE_LOCAL_SDK_ROOT}")
    set(RE_SDK_ROOT "${RE_LOCAL_SDK_ROOT}" CACHE PATH "Location of RE SDK")
  endif()
endif()

# In some environments, stb_sprintf does not work at this time (ex: wasm)
option(RE_MOCK_USE_STB_SPRINTF "Enable/Disable using stb_sprintf" ON)

set(target "re-mock")

# Defines the location of the sources
set(re-mock_CPP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src/cpp")

# Defines the headers if you want to include them in your project (optional)
set(re-mock_BUILD_HEADERS
    ${re-mock_CPP_SRC_DIR}/re/mock/re-mock.h
    ${re-mock_CPP_SRC_DIR}/re/mock/Config.h
    ${re-mock_CPP_SRC_DIR}/re/mock/Constants.h
    ${re-mock_CPP_SRC_DIR}/re/mock/DeviceTesters.h
    ${re-mock_CPP_SRC_DIR}/re/mock/Errors.h
    ${re-mock_CPP_SRC_DIR}/re/mock/Extension.h
    ${re-mock_CPP_SRC_DIR}/re/mock/FileManager.h
    ${re-mock_CPP_SRC_DIR}/re/mock/MockDevices.h
    ${re-mock_CPP_SRC_DIR}/re/mock/Motherboard.h
    ${re-mock_CPP_SRC_DIR}/re/mock/MotherboardImpl.h
    ${re-mock_CPP_SRC_DIR}/re/mock/PatchParser.h
    ${re-mock_CPP_SRC_DIR}/re/mock/Transport.h
    ${re-mock_CPP_SRC_DIR}/re/mock/Rack.h
    ${re-mock_CPP_SRC_DIR}/re/mock/Resources.h
    ${re-mock_CPP_SRC_DIR}/re/mock/Sequencer.h
    ${re-mock_CPP_SRC_DIR}/re/mock/fmt.h
    ${re-mock_CPP_SRC_DIR}/re/mock/stl.h
    ${re-mock_CPP_SRC_DIR}/re/mock/lua/InfoLua.h
    ${re-mock_CPP_SRC_DIR}/re/mock/lua/LuaState.h
    ${re-mock_CPP_SRC_DIR}/re/mock/lua/MockJBox.h
    ${re-mock_CPP_SRC_DIR}/re/mock/lua/MotherboardDef.h
    ${re-mock_CPP_SRC_DIR}/re/mock/lua/RealtimeController.h
    )

# Defines the sources
set(re-mock_BUILD_SOURCES
    ${re-mock_CPP_SRC_DIR}/re/mock/Config.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/DeviceTesters.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/Extension.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/FileManager.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/Jukebox.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/MockDevices.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/Motherboard.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/PatchParser.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/Rack.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/Sequencer.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/Transport.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/fft.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/fmt.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/lua/InfoLua.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/lua/LuaState.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/lua/MockJBox.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/lua/MotherboardDef.cpp
    ${re-mock_CPP_SRC_DIR}/re/mock/lua/RealtimeController.cpp
    )

# Defines the include directories
set(re-mock_INCLUDE_DIRECTORIES "${re-mock_CPP_SRC_DIR}")

# Including stb
add_subdirectory(external/nothings/stb)

# Including lua lib/headers
add_subdirectory(external/lua-cmake)

# Including tinyxml2
add_subdirectory(external/tinyxml2-9.0.0)

# Including midifile
add_subdirectory(external/craigsapp-midifile)

# Including miniaudio (for file loading/saving)
add_subdirectory(external/miniaudio)

add_library(${target} STATIC "${re-mock_BUILD_HEADERS}" "${re-mock_BUILD_SOURCES}")
target_compile_definitions(${target} PUBLIC
    LOCAL_NATIVE_BUILD=1
    JUKEBOX_SDK=0
    DEBUG=1
    RE_MOCK_USE_STB_SPRINTF=$<BOOL:${RE_MOCK_USE_STB_SPRINTF}>
)
target_include_directories(${target} PUBLIC "${RE_MOCK_SDK_ROOT}/API" "${re-mock_INCLUDE_DIRECTORIES}") # exporting SDK API to plugin
target_link_libraries(${target} PUBLIC lua::lib lua::header tinyxml2::tinyxml2 midifile stb miniaudio)

if(re-mock_DEV_BUILD)
  target_compile_definitions(${target} PUBLIC ENABLE_RE_MOCK_INTERNAL_ASSERT=1)

  # Generate the re_mock_build.h file
  set(GENERATED_FILES_DIR "${CMAKE_BINARY_DIR}/generated")
  file(TO_NATIVE_PATH "${CMAKE_CURRENT_LIST_DIR}" PROJECT_DIR_NATIVE_PATH)
  if(EXISTS "${RE_SDK_ROOT}")
    file(TO_NATIVE_PATH "${RE_SDK_ROOT}" RE_SDK_ROOT_NATIVE_PATH)
  else()
    set(RE_SDK_ROOT_NATIVE_PATH "")
  endif()
  file(TO_NATIVE_PATH "${RE_MOCK_SDK_ROOT}" RE_MOCK_SDK_ROOT_NATIVE_PATH)
  configure_file("${CMAKE_CURRENT_LIST_DIR}/re_mock_build.h.in" "${GENERATED_FILES_DIR}/re_mock_build.h")

  #######################################################
  # Testing
  #######################################################
  # Download and unpack googletest at configure time
  include(fetch-GoogleTest.cmake)
  include(GoogleTest)

  set(target_test "${target}_test")

  set(re-mock_CPP_TST_DIR "${CMAKE_CURRENT_LIST_DIR}/test/cpp")

  set(TEST_CASE_SOURCES
      "${re-mock_CPP_TST_DIR}/re/mock/TestDeviceTesters.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/TestFft.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/TestFmt.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/TestJukebox.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/TestMidi.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/TestMisc.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/TestMockDevices.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/TestPatch.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/TestRack.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/TestRackExtension.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/TestSequencer.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/TestStl.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/TestTransport.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/lua/TestInfoLua.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/lua/TestLuaState.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/lua/TestMotherboardDef.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/lua/TestRealtimeController.cpp"
      "${re-mock_CPP_TST_DIR}/re/mock/lua/TestSanityCheckSDK.cpp"
      )

  add_executable("${target_test}" "${TEST_CASE_SOURCES}")
  target_link_libraries("${target_test}" gtest_main gmock ${target})
  target_include_directories("${target_test}" PUBLIC "${PROJECT_SOURCE_DIR}" "${GENERATED_FILES_DIR}")

  gtest_discover_tests("${target_test}")

  add_custom_target("run-tests"
      COMMAND ${CMAKE_COMMAND} -E echo "Running tests using $<TARGET_FILE:${target_test}>"
      COMMAND "${CMAKE_CTEST_COMMAND}" -C $<CONFIG>
      DEPENDS "${target_test}"
      )

endif()
