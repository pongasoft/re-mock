Introduction
------------

The goal of this project is to offer mock classes of the Reason SDK APIs in order to be able to unit test (or black box test) rack extensions (for Reason DAW by Reason Studios).

Taste of the framework
----------------------

Example of unit test (using GoogleTest):

 ```c++
// Creates a config by reading motherboard_def.lua and realtime_controller.lua
auto c = DeviceConfig<Device>::fromJBoxExport(RE_CMAKE_PROJECT_DIR);

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
auto buffer = tester.nextBatch(MockAudioDevice::buffer(0.5, 0.6));

// Check that the resulting buffer is what we expected (here we assume that it is an effect with a gain knob
// set to 0 dB change => output == input).
ASSERT_EQ(MockAudioDevice::buffer(0.5, 0.6), buffer);

// Make sure that properties were changed accordingly
ASSERT_TRUE(tester.device().getBool("/custom_properties/sound_on_led");
```

Links
-----

* [Installation](docs/Install.md)
* [Quick Starting guide](docs/Quick_Start.md)
* [Documentation](docs/Documentation.md)
* [Advanced Topics](docs/Advanced_Topics.md)

Status & Limitations
--------------------

This project is currently under development and as a result, not all apis and functionalities provided by Reason 
are supported.

Here is a list of known unimplemented features:

* `/environment/master_tune` (you can change it but it has no effect currently)

If you find other unimplemented feature and/or you want to help in implementation, feel free to open a ticket. I
am looking for use cases my plugins are not covering, so do not hesitate to contact me.

Release notes
-------------

#### 0.9.3 - 2021-10-04

- Added `ui_percent` to the list of ignored lua keywords

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

- This project includes [midifile](https://github.com/craigsapp/midifile) released under a [BSD 2-Clause "Simplified" License](external/craigsapp-midifile/LICENSE.txt)

- This project (optionally) uses [libsndfile](https://github.com/libsndfile/libsndfile) relased under an [LGPL-2.1 License](https://github.com/libsndfile/libsndfile/blob/master/COPYING)
