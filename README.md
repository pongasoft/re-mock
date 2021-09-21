Introduction
------------

The goal of this project is to offer mock classes of the Reason SDK APIs in order to be able to unit test (or black box test) rack extensions (for Reason DAW by Reason Studios).

Taste of the framework
----------------------

Example of unit test (using GoogleTest):

 ```c++
// Creates a config by reading motherboard_def.lua and realtime_controller.lua
auto c = DeviceConfig<Device>::fromJBoxExport(RE_CMAKE_MOTHERBOARD_DEF_LUA, RE_CMAKE_REALTIME_CONTROLLER_LUA);

// Creates a tester for the device (a studio_fx device)
auto tester = StudioEffectTester<Device>(c);

// Wire the sockets
tester.wireMainIn("audioInLeft", "audioInRight");
tester.wireMainOut("audioOutLeft", "audioOutRight");

// Change properties (simulate user action)
tester.device().setNum("/custom_properties/gain", 0.7);

// Call "JBox_Export_RenderRealtime" of all registered devices
// Populate a stereo buffer (2x64 samples) with the value 0.5 (left) and 0.6 (right) 
// made available to the device under test on the [in] sockets previously wired ("audioInLeft" / "audioInRight")
// Read the [out] sockets after processing and return it (buffer)
auto buffer = tester.nextFrame(MockAudioDevice::buffer(0.5, 0.6));

// Check that the resulting buffer is what we expected (here we assume that it is an effect with a gain knob
// set to 0 dB change => output == input).
ASSERT_EQ(MockAudioDevice::buffer(0.5, 0.6), buffer);

// Make sure that properties were changed accordingly
ASSERT_TRUE(tester.device().getBool("/custom_properties/sound_on_led");
```

Installation and usage
----------------------

This CMake based project compiles into a library called `re-mock` after adding it as a subdirectory to your rack extension project (`add_subdirectory`). The only requirement is that the RE SDK (4.2.0+) be installed on your system and that the `RE_SDK_ROOT` CMake variable be set to the location of the SDK.

```cmake
# in CMakeLists.txt
set(RE_SDK_ROOT "<path_to_RE_SDK_4.2.0+>")

# add re-mock as a subdirectory
add_subdirectory("re-mock" EXCLUDE_FROM_ALL)

# add the library "re-mock" to the test target
target_link_libraries("${test_target}" "gtest_main" "re-mock")
```

It is highly recommended to use [re-cmake](https://github.com/pongasoft/re-cmake) and check [re-blank-plugin](https://github.com/pongasoft/re-blank-plugin) on how to configure it. TODO: integrate with re-quickstart!!!

```cmake
# in CMakeLists.txt (using re-cmake)
# RE_SDK_ROOT is set by re-cmake so no need to set it

# add re-mock as a subdirectory (not the recommended way to do this: check re-blank-plugin for a better way)
add_subdirectory("re-mock" EXCLUDE_FROM_ALL)

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
    TEST_LINK_LIBS           "native-test-lib" "re-mock"  # tests can link plugin classes
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

Documentation
-------------

Check the [documentation](docs/Documentation.md).

Status & Limitations
--------------------

This project is currently under development and as a result, not all apis and functionalities provided by Reason 
are supported.

Here is a list of the Jukebox API currently not implemented:

* `JBox_GetSampleInfo`
* `JBox_GetSampleMetaData`
* `JBox_GetSampleData`
* `JBox_GetBLOBInfo`
* `JBox_GetBLOBData`
* `JBox_GetOptimalFFTAlignment`
* `JBox_FFTRealForward`
* `JBox_FFTRealInverse`

Here is a list of known unimplemented features:

* the `/transport` properties (see documentation for workaround)
* `/patterns` (`patterns = {}` in `motherboard_def.lua`)
* `/environment/master_tune` (you can change it but it has no effect currently)

If you find other unimplemented feature and/or you want to help in implementation, feel free to open a ticket. I
am looking for use cases my plugins are not covering, so do not hesitate to contact me.

Release notes
-------------

#### 0.9.2 - 2021-09-21

- Fixed output buffers not being cleaned before `nextFrame`
- Fixed input buffers not being cleaned after `nextFrame`

#### 0.9.1 - 2021-09-19

- Fixed "warning: unused function 'compare'" message
- Added missing licenses

#### 0.9.0 - 2021-09-12

- First Beta/Feedback release.

License
-------

- This project is released under the terms of the [Apache 2.0 license](LICENSE.txt)

- This project uses the Rack extension SDK released under the [Rack Extension License agreement](RE_License.txt)

- This project includes [lua-cmake](https://github.com/lubgr/lua-cmake) released under an [MIT License](external/lua-cmake/LICENSE)

- This project includes [lua 5.4.1](https://www.lua.org/) released under an [MIT License](https://www.lua.org/license.html)
