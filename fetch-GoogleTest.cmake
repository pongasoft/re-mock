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

if(GOOGLETEST_ROOT_DIR)
  # instructs FetchContent to not download or update but use the location instead
  set(FETCHCONTENT_SOURCE_DIR_GOOGLETEST ${GOOGLETEST_ROOT_DIR})
else()
  set(FETCHCONTENT_SOURCE_DIR_GOOGLETEST "")
endif()

#------------------------------------------------------------------------
# The git respository to fetch googletest from
#------------------------------------------------------------------------
set(googletest_GIT_REPO "https://github.com/google/googletest" CACHE STRING "googletest git repository URL")

#------------------------------------------------------------------------
# The git tag for googletest
# v1.13.0 (2023/01/17/30)
#------------------------------------------------------------------------
set(googletest_GIT_TAG "v1.13.0" CACHE STRING "googletest git tag")

#------------------------------------------------------------------------
# The download URL for googletest
#------------------------------------------------------------------------
set(googletest_DOWNLOAD_URL "${googletest_GIT_REPO}/archive/refs/tags/${googletest_GIT_TAG}.zip" CACHE STRING "googletest download url" FORCE)
set(googletest_DOWNLOAD_URL_HASH "SHA256=ffa17fbc5953900994e2deec164bb8949879ea09b411e07f215bfbb1f87f4632" CACHE STRING "googletest download url hash" FORCE)

FetchContent_Declare(googletest
    EXCLUDE_FROM_ALL
    URL                        "${googletest_DOWNLOAD_URL}"
    URL_HASH                   "${googletest_DOWNLOAD_URL_HASH}"
    DOWNLOAD_EXTRACT_TIMESTAMP true
    SOURCE_DIR                 "${CMAKE_BINARY_DIR}/googletest-src"
    BINARY_DIR                 "${CMAKE_BINARY_DIR}/googletest-build"
    CONFIGURE_COMMAND          ""
    BUILD_COMMAND              ""
    INSTALL_COMMAND            ""
    TEST_COMMAND               ""
    )

FetchContent_GetProperties(googletest)

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "Set by re-cmake" FORCE)

# Do not install GoogleTest!
option(INSTALL_GTEST "Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)" OFF)

if(FETCHCONTENT_SOURCE_DIR_GOOGLETEST)
  message(STATUS "Using googletest from local ${FETCHCONTENT_SOURCE_DIR_GOOGLETEST}")
else()
  message(STATUS "Fetching googletest from ${googletest_DOWNLOAD_URL}")
endif()

# Add googletest directly to our build. This defines the gtest and gtest_main targets.
FetchContent_MakeAvailable(googletest)
