Requirements
------------

* CMake 3.17+
* For Mac: tested on macOS Big Sur (11.7) / Xcode 13.2.1 (requires macOS 15+)
* For Windows: tested on Windows 10 / "Visual Studio 16 2019"
* RE SDK 4.1.0+

Installation
------------

This CMake based project compiles into a library called `re-mock` after adding it as a subdirectory to your rack extension project (`add_subdirectory`). The only requirement is that the RE SDK (4.1.0+) be installed on your system and that the `RE_SDK_ROOT` CMake variable be set to the location of the SDK.

```cmake
# in CMakeLists.txt
set(RE_SDK_ROOT "<path_to_RE_SDK_4.1.0+>")

# add re-mock as a subdirectory
add_subdirectory("re-mock" EXCLUDE_FROM_ALL)

# add the library "re-mock" to the test target
target_link_libraries("${test_target}" "gtest_main" "re-mock")
```

It is highly recommended to use [re-cmake](https://github.com/pongasoft/re-cmake) and in particular it is strongly advised to check one of the following projects:

* [Rack Extension - Quick Start](https://pongasoft.com/re-quickstart/index.html) which creates a blank plugin and uses `re-mock` for testing
* [Rack Extension - Convert](https://github.com/pongasoft/re-quickstart/blob/master/docs/convert.md) which converts an existing plugin to use `re-cmake` and `re-mock` for testing


```cmake
# in CMakeLists.txt (using re-cmake)
# RE_SDK_ROOT is set by re-cmake so no need to set it

# Initializes re-cmake and add re-mock
re_cmake_init(INCLUDES re-mock)

# add the library to "TEST_LINK_LIBS"
add_re_plugin(
    RE_SDK_VERSION       "${RE_SDK_VERSION}"
    RE_SDK_ROOT          "${RE_SDK_ROOT}"
    RE_2D_RENDER_ROOT    "${RE_2D_RENDER_ROOT}"
    BUILD_SOURCES        "${re_sources_cpp}"      # compiled for both local and jbox builds
    NATIVE_BUILD_SOURCES "${logging_sources}"     # compiled only for local builds
    RENDER_2D_SOURCES    "${re_sources_2d}"
    INCLUDE_DIRECTORIES  "${LOGGING_CPP_SRC_DIR}" # plugin uses loguru
    COMPILE_OPTIONS      -Wall
    ENABLE_DEBUG_LOGGING                          # turn on JBOX_TRACE and loguru
    # Testing
    TEST_CASE_SOURCES        "${re_test_cpp}"     # the source files containing the test cases
    TEST_INCLUDE_DIRECTORIES "${RE_CPP_SRC_DIR}"  # tests can include plugin classes
    TEST_LINK_LIBS           "native-test-lib" "${re-mock_LIBRARY_NAME}"  # tests can link plugin classes
)
```

> #### Note
> There are many ways to bring this framework into your own project and how you do it depends on your preferences
> and structure. Here are a few examples:
> 1. `re-mock` is local but "out of tree", somewhere on the user's system, for example where multiple projects share one instance of it. This would require to clone or copy this project somewhere.
> 2. `re-mock` is local, but "in tree", within the RE project directory (or somewhere below), for example where using git submodules or git subtree, or even when just copied directly into the RE project (good for people who love 100% reproducible builds).
> 3. `re-mock` is remote, and fetched by CMake during the configure phase (this is the recommended approach: check [re-blank-plugin](https://github.com/pongasoft/re-blank-plugin) for an example).

> #### Note
> This project can be opened directly to work on the project itself! It contains its own set of unit tests.

If the device under test uses samples that get loaded via patch or apis, you must enable support for sample loading by using the `RE_MOCK_SUPPORT_FOR_AUDIO_FILE` option like so:

```cmake
option(RE_MOCK_SUPPORT_FOR_AUDIO_FILE "" ON) # must be set BEFORE including re-mock
add_subdirectory("re-mock" EXCLUDE_FROM_ALL)
```
