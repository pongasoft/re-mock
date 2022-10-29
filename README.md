Introduction
------------

The goal of this project is to offer mock classes of the Reason SDK APIs in order to be able to unit test (or black box test) rack extensions (for Reason DAW by Reason Studios).

Features
--------

* Allow for testing rack extensions by fully controlling the event loop
* Supports 100% of `Jukebox.h` functions (ex: `JBox_LoadMOMProperty`)
* Supports 100% of `jbox.lua` functions (in `realtime_controller.lua`) (ex: `jbox.load_sample_async`)
* Load `info.lua`, `motherboard_def.lua` and `realtime_controller.lua` to build the rack extension exactly as Reason does
* Support loading and saving sample files (wav, aiff...)
* Support loading patch files (for example a patch file saved in Recon/Reason)
* Support for simulating slow loads and errors (when loading samples or blobs)
* Support loading midi files to populate the sequencer track
* Powerful APIs to help in common testing use cases (like "playing an instrument" or "processing a sample through an effect")
* much, much more

Taste of the framework
----------------------

Example of unit test (using GoogleTest):

 ```cpp
// Creates a config by reading info.lua, motherboard_def.lua and realtime_controller.lua
auto c = DeviceConfig<MyInstrument>::fromJBoxExport(RE_CMAKE_PROJECT_DIR);

// Creates a tester for the device (an instrument device)
auto tester = InstrumentTester<MyInstrument>(c, 48000);

// wiring main out sockets
tester.wireMainOut("OutLeft", "OutRight");

// first batch = initialization
tester.nextBatch();

// load a patch to set all the parameters of the device to a known state
tester.device().loadPatch("/Public/Sync Bell.repatch");

// apply patch
tester.nextBatch();

// import a Midi file containing note on/off events
tester.importMidi(resource::File{fmt::path(RE_CMAKE_PROJECT_DIR, "test", "midi", "test1.mid")});

// set the transport position at the start of bar 2
tester.transportPlayPos(sequencer::Time(2,1,1,0));

// play for 2 bars (so bar 2 and 3) and captures the output
auto sample = tester.bounce(sequencer::Duration::k1Beat_4x4 * 2);

// make sure that the sample is what we expect
ASSERT_EQ(*sample, *tester.loadSample(resource::File{fmt::path(RE_CMAKE_PROJECT_DIR, "test", "wav", "test1.wav")));
  
// Optionally the sample can be saved to a file (for listening to it or opening in an audio visualizer...)
// tester.saveSample(*sample, resource::File{"/tmp/result.wav"});
```

Example of command line tool: offline effect processor

```cpp
// invoke with <path to RE>, <patch>, <input file (to process)>, <output file (result)>
int main(int argc, char *argv[])
{
  // ignoring all error handling for this brief example!!!
  auto c = DeviceConfig<MyEffect>::fromJBoxExport(argv[1]);
  auto tester = StudioEffectTester<MyEffect>(c);
  tester.device().loadPatch(argv[2]); // set the device in a known state (optional of course)
  auto sample = tester.processSample(argv[3]);
  tester.saveSample(*sample, resource::File{argv[4]});
}
```

Links
-----

* [Installation](docs/Install.md)
* [Quick Starting guide](docs/Quick_Start.md)
* [Documentation](docs/Documentation.md)
* [Advanced Topics](docs/Advanced_Topics.md)

Release notes
-------------

#### 1.2.0 - 2022-10-29

- Much better error reporting (when error in lua files)
- Use of `std::filesystem` throughout the APIs
- Use `stb_sprintf` for better performance

> #### Note
> On macOS, the `std::filesystem` APIs were introduced in macOS 10.15, so as a result, `re-mock` now requires a minimum of macOS 10.15

#### 1.1.0 - 2022-09-25

- Handle discrete properties
- `instance` is now optional (although it does not make a lot of sense for a real device, some examples in the SDK do not have one!)
- added missing `/transport/muted` property
- allow string parameter in `jbox.native_object` default parameter list
- Exposes more internal APIS (like `JboxPropertyInfo`) in order to build other tools on top of `re-mock`
- Fixed various bugs

#### 1.0.1 - 2022-02-01

- Fixes for Windows 10

#### 1.0.0 - 2022-01-22

- Reached maturity to be a 1.0.0 release (70 commits since 0.9.3!)

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

- This project includes [stb_sprintf](https://github.com/nothings/stb) released under a [Public Domain License](https://github.com/nothings/stb/blob/master/LICENSE)

- This project (optionally) uses [libsndfile](https://github.com/libsndfile/libsndfile) relased under an [LGPL-2.1 License](https://github.com/libsndfile/libsndfile/blob/master/COPYING)
