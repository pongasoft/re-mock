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

include(FetchContent)

function(fetch_content)
  set(oneValueArgs NAME GIT_REPO GIT_TAG DOWNLOAD_URL DOWNLOAD_URL_HASH ROOT_DIR)

  cmake_parse_arguments(
      "ARG" # prefix
      "" # options
      "${oneValueArgs}" # single values
      "" # multiple values
      ${ARGN}
  )

  if(NOT ARG_NAME)
    message(FATAL_ERROR "fetch_content requires NAME argument")
  endif()

  macro(set_default_value name default_value)
    if(NOT ${name})
      set(${name} "${default_value}")
    endif()
  endmacro()

  set_default_value(ARG_ROOT_DIR "${${ARG_NAME}_ROOT_DIR}")
  set_default_value(ARG_DOWNLOAD_URL "${${ARG_NAME}_DOWNLOAD_URL}")
  set_default_value(ARG_DOWNLOAD_URL_HASH "${${ARG_NAME}_DOWNLOAD_URL_HASH}")
  set_default_value(ARG_GIT_REPO "${${ARG_NAME}_GIT_REPO}")
  set_default_value(ARG_GIT_TAG  "${${ARG_NAME}_GIT_TAG}")
  set_default_value(ARG_GIT_TAG  "master")

  if(NOT ARG_ROOT_DIR AND NOT ARG_GIT_REPO AND NOT ARG_DOWNLOAD_URL)
    message(FATAL_ERROR "fetch_content requires either ROOT_DIR argument/${ARG_NAME}_ROOT_DIR variable or GIT_REPO/${ARG_NAME}_GIT_REPO or DOWNLOAD_URL/${ARG_NAME}_DOWNLOAD_URL to be defined ")
  endif()

  string(TOUPPER "${ARG_NAME}" UPPERCASE_NAME)

  if(ARG_ROOT_DIR)
    message(STATUS "Using ${ARG_NAME} from local ${ARG_ROOT_DIR}")
    FetchContent_Declare(${ARG_NAME}
        SOURCE_DIR       "${ARG_ROOT_DIR}"
        BINARY_DIR       "${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}-build"
        SOURCE_SUBDIR    "do_not_make_available" # invalid folder to not execute CMakeLists.txt
    )
  else()
    if(ARG_DOWNLOAD_URL)
      message(STATUS "Fetching ${ARG_NAME} from ${ARG_DOWNLOAD_URL}")
      FetchContent_Declare(          ${ARG_NAME}
          URL                        "${ARG_DOWNLOAD_URL}"
          URL_HASH                   "${ARG_DOWNLOAD_URL_HASH}"
          SOURCE_DIR                 "${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}-src"
          BINARY_DIR                 "${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}-build"
          DOWNLOAD_EXTRACT_TIMESTAMP true
          SOURCE_SUBDIR              "do_not_make_available"
      )
    else()
      message(STATUS "Fetching ${ARG_NAME} from ${ARG_GIT_REPO}/tree/${ARG_GIT_TAG}")
      FetchContent_Declare(${ARG_NAME}
          GIT_REPOSITORY   ${ARG_GIT_REPO}
          GIT_TAG          ${ARG_GIT_TAG}
          GIT_CONFIG       advice.detachedHead=false
          GIT_SHALLOW      true
          SOURCE_DIR       "${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}-src"
          BINARY_DIR       "${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}-build"
          SOURCE_SUBDIR    "do_not_make_available"
      )
    endif()
  endif()

  FetchContent_MakeAvailable(${ARG_NAME})

  set(${ARG_NAME}_ROOT_DIR "${${ARG_NAME}_SOURCE_DIR}" PARENT_SCOPE)
  set(${ARG_NAME}_SOURCE_DIR "${${ARG_NAME}_SOURCE_DIR}" PARENT_SCOPE)
  set(${ARG_NAME}_BINARY_DIR "${${ARG_NAME}_BINARY_DIR}" PARENT_SCOPE)

endfunction()

set(googletest_GIT_REPO "https://github.com/google/googletest" CACHE STRING "googletest git repository URL")
set(googletest_GIT_TAG "v1.17.0" CACHE STRING "googletest git tag")
set(googletest_DOWNLOAD_URL "${googletest_GIT_REPO}/archive/refs/tags/${googletest_GIT_TAG}.zip" CACHE STRING "googletest download url" FORCE)
set(googletest_DOWNLOAD_URL_HASH "SHA256=40d4ec942217dcc84a9ebe2a68584ada7d4a33a8ee958755763278ea1c5e18ff" CACHE STRING "googletest download url hash" FORCE)

fetch_content(NAME googletest)

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "Set by re-common" FORCE)

# Do not install GoogleTest!
option(INSTALL_GTEST "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" OFF)

# Add googletest directly to our build. This defines the gtest and gtest_main targets.
add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
