cmake_minimum_required(VERSION 3.12)

include(FetchContent)

if(LIBSNDFILE_ROOT_DIR)
  # instructs FetchContent to not download or update but use the location instead
  set(FETCHCONTENT_SOURCE_DIR_LIBSNDFILE ${LIBSNDFILE_ROOT_DIR})
else()
  set(FETCHCONTENT_SOURCE_DIR_LIBSNDFILE "")
endif()

set(LIBSNDFILE_GIT_REPO "https://github.com/libsndfile/libsndfile" CACHE STRING "libsndfile git repository url" FORCE)
# Release 1.1.0 / 2022/03/27
set(LIBSNDFILE_GIT_TAG 1.1.0 CACHE STRING "libsndfile git tag" FORCE)
set(LIBSNDFILE_DOWNLOAD_URL "${LIBSNDFILE_GIT_REPO}/archive/refs/tags/${LIBSNDFILE_GIT_TAG}.zip" CACHE STRING "libsndfile download url" FORCE)

FetchContent_Declare(libsndfile
      URL                        "${LIBSNDFILE_DOWNLOAD_URL}"
      SOURCE_DIR                 "${CMAKE_BINARY_DIR}/libsndfile"
      BINARY_DIR                 "${CMAKE_BINARY_DIR}/libsndfile-build"
      DOWNLOAD_EXTRACT_TIMESTAMP true
      CONFIGURE_COMMAND          ""
      BUILD_COMMAND              ""
      INSTALL_COMMAND            ""
      TEST_COMMAND               ""
      )

FetchContent_GetProperties(libsndfile)

if(NOT libsndfile_POPULATED)

  if(FETCHCONTENT_SOURCE_DIR_LIBSNDFILE)
    message(STATUS "Using libsndfile from local ${FETCHCONTENT_SOURCE_DIR_LIBSNDFILE}")
  else()
    message(STATUS "Fetching libsndfile from ${LIBSNDFILE_DOWNLOAD_URL}")
  endif()

  FetchContent_Populate(libsndfile)

endif()

set(LIBSNDFILE_ROOT_DIR ${libsndfile_SOURCE_DIR})
set(LIBSNDFILE_INCLUDE_DIR "${libsndfile_BINARY_DIR}/src")

function(libsndfile_build)
  option(BUILD_PROGRAMS "Build programs" OFF)
  option(BUILD_EXAMPLES "Build examples" OFF)
  option(BUILD_TESTING "Build examples" OFF)
  option(ENABLE_CPACK "Enable CPack support" OFF)
  option(ENABLE_PACKAGE_CONFIG "Generate and install package config file" OFF)
  option(BUILD_REGTEST "Build regtest" OFF)
  # finally we include libsndfile itself
  add_subdirectory(${libsndfile_SOURCE_DIR} ${libsndfile_BINARY_DIR} EXCLUDE_FROM_ALL)
endfunction()

libsndfile_build()

include_directories(${LIBSNDFILE_INCLUDE_DIR})
