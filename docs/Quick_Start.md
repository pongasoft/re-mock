Quick Starting Guide
--------------------

### Hello World

After [installing](Install.md) re-mock, here is the most basic example (aka "hello world" of re-mock) demonstrating how to write a test:

```cpp
#include <gtest/gtest.h>
#include <re_cmake_build.h>
#include <re/mock/re-mock.h>
#include <Device.h>

using namespace re::mock;

// Device - Init
TEST(Device, Init)
{
  auto c = DeviceConfig<Device>::fromJBoxExport(RE_CMAKE_PROJECT_DIR);
  auto tester = InstrumentTester<Device>(c);
  tester.nextBatch();
}
```

#### Includes

* `gtest/gtest.h` is the Google Test framework (and you can replace with your own testing framework if you do not use Google Test).
* `re_cmake_build.h` is a file generated by `re-cmake` that contains very useful "defines" like `RE_CMAKE_PROJECT_DIR` pointing to the root of your plugin, thus giving access to `info.lua`, etc...
* `re/mock/re-mock.h` is the primary include for the framework giving access to the various device testers
* `Device.h` is whatever header file that contains the main class of your plugin

> #### Note
> `re_cmake_build.h` is only available if you use `re-cmake`. If you do not use `re-cmake` the `RE_CMAKE_PROJECT_DIR` define will not be available, so you need to provide the path pointing to the root of the plugin.

> #### Note
> The "main" class of your plugin is the "instance" that you instantiate in the `JBox_Export_CreateNativeObject` call. For example, in the RE example `VerySimplySampler`, this would be `CVerySimpleSampler`. In this example, the main instance is of type `Device`.

#### Namespace

All classes in `re-mock` are under the `re::mock` namespace, so we simply use it to avoid repeating the namespace over and over.

#### Test

1. We instantiate a typed `DeviceConfig` for our main class `Device` by providing the location of where `info.lua`, `motherboard_def.lua` and `relatime_controller.lua` are located. Thanks to `re_cmake_build.h`, this is trivial.
2. We instantiate a tester for our device. Each tester is specialized for the kind of rack extension under test and must match the `device_type` entry from `info.lua`. Note that the tester is also itself typed (allowing to access the main class directly).
3. We call `nextBatch` which advances the engine by one batch and as a result will instantiate the device under test and call `JBox_Export_RenderRealtime` one time, thus effectively initializing the device.

Although this code is small and seems to test nothing, it actually does a lot of work:

* it parses `info.lua`, `motherboard_def.lua` and `relatime_controller.lua` to build an in-memory model of the rack extension
* it runs the `rtc_bindings` (lua code), thus effectively calling `JBox_Export_CreateNativeObject` (c++ code) and instantiates the `Device`
* it calls `JBox_Export_RenderRealtime` with all the proper set of `iPropertyDiffs` as defined in the `rt_input_setup/notify` section of `realtime_controller.lua`

A lot can go wrong in this initialization and so it is always a good thing to have such a small/concise test run prior to deploying to Recon, to catch some typos for example, some of them triggering a hard crash of Recon!

### The main event loop

Recon/Reason has a main event loop, calling `JBox_Export_RenderRealtime` over and over as fast as necessary. For example, at a sample rate of 48000, it calls this function 750 times per second which can make logging and/or debugging quite challenging.

`re-mock` puts you in charge of the main event loop, so you decide when this API is called. For example:

* invoking `tester.nextBach()` runs through exactly one iteration of this loop.
* invoking `tester.nextBatches(time::Duration{1000})` runs through 750 iterations (assuming 48000 sample rate which can be chosen when instantiating the tester).

By controlling precisely the main event loop, it becomes easy to set the device under test in a precise configuration to be able to run some tests.

### A more elaborate example

```cpp
TEST(Device, test1)
{
  auto c = DeviceConfig<Device>::fromJBoxExport(RE_CMAKE_PROJECT_DIR);
  auto tester = InstrumentTester<Device>(c, 48000);

  // wiring main out
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
  
  // tester.saveSample(*sample, resource::File{"/tmp/result.wav"});
}
```

* Like the "Hello World" example, we instantiate an instrument tester for the device this time with a sample rate of 48000 (the default being 44100).

* Instantiating an `InstrumentTester` actually instantiates another device: a "destination" device (`MAUDst`) so that the sound generated by the device under test can be captured. Calling `tester.wireMainOut("OutLeft", "OutRight")` is wiring the main device under test to this destination device and `OutLeft` and `OutRight` are the name of the sockets defined in `motherboard_def.lua`

* The test runs 1 batch to initialize everything.
    > #### Note
    > If the device under test defines a `default_patch` in `info.lua`, all the values of this patch will be part of the first `iPropertyDiffs` exactly like Recon/Reason (if they are defined in `rt_input_setup`).

* The test then loads a different patch by calling the `loadPatch` method on the device itself. Note how it uses a built-in patch simply referenced by its path (`/Public/Sync Bell.repatch`).
  > #### Note
  > It is possible to load a patch that is not built-in by using the overloaded api `tester.device().loadPatch(resource::File const &)`, particularly useful for example, if a user reports some issue, he/she can simply saves the patch and be directly loaded.

* Applying the patch will provide all the values of the patch to the device (either via `iPropertyDiffs` if the device uses `rt_input_setup` or via `JBox_LoadMOMProperty` if the device uses this technique instead).

* The test then imports a Midi file (which is located in the source tree). The Midi file can have been generated by any means (like for example "Exporting Midi" in Reason). Each device has a sequencer track and the notes from the Midi file populates the sequencer track.

* For the sake of this example, we set the transport play position starting at bar 2 (if we don't, it just starts at 1) and we assume that there are either no events prior to bar 2 in the Midi file, or we just simply want to skip them.

* The test uses the `IntrumentTester::bounce()` convenient api which does a few things:

  * it runs as many batches as necessary to cover the provided duration
  * it starts the transport playback when it starts, and stops it when it ends (so that the events that were imported in the sequencer track from the Midi file are triggered)
  * it captures the output of the device and returns it as a `Sample` (`MockAudioDevice::Sample`) which can be directly compared with some other "expected" sample.
  
* The test compares the output generated with some sample file loaded directly from the file system.
 
* Finally, as shown in the commented line, you could also save the sample generated directly in a file so that you can use any external tool to check the result. 

### A final example

This final example in this quick start section is a test written for the Very Simple Sampler example code provided with the RE SDK. 

```cpp
TEST(CVerySimpleSampler, Play)
{
  auto c = DeviceConfig<CVerySimpleSampler>::fromJBoxExport(RE_CMAKE_PROJECT_DIR).disable_trace();
  auto tester = InstrumentTester<CVerySimpleSampler>(c);

  // we wire main out to the proper sockets in the device
  tester.wireMainOut("left", "right");

  // we load the actual samples that will be played so that we can use them to check playback
  auto const bellSample = tester.loadSample("/Public/Samples/Bell.wav");
  auto const chipSample = tester.loadSample("/Public/Samples/Chip.wav");
  auto const machinaSample = tester.loadSample("/Public/Samples/Machina.wav");
  auto const mkivSample = tester.loadSample("/Public/Samples/MkIV.wav");

  // initializes the device
  tester.nextBatch();

  // Make sure we play at max volume (gain = 1.0 / preview_volume_level = 127) and set the proper root_key
  tester.device().setNum("/user_samples/0/root_key", Midi::C(3));       tester.device().setNum("/user_samples/0/preview_volume_level", 127);
  tester.device().setNum("/user_samples/1/root_key", Midi::D_sharp(3)); tester.device().setNum("/user_samples/1/preview_volume_level", 127);
  tester.device().setNum("/user_samples/2/root_key", Midi::F_sharp(3)); tester.device().setNum("/user_samples/2/preview_volume_level", 127);
  tester.device().setNum("/user_samples/3/root_key", Midi::A(3));       tester.device().setNum("/user_samples/3/preview_volume_level", 127);

  // play C3 (Bell) (gain = 1.0)
  ASSERT_EQ(*bellSample,
            *tester.bounce(sample::Duration{bellSample.getFrameCount()},
                           tester.newTimeline()
                             .note(Midi::C(3), sample::Duration{bellSample.getFrameCount()}, 127)));

  // play C3 again (note off/note on) (Bell) (gain = 0.78 (100/127))
  ASSERT_EQ(bellSample->clone().applyGain(100/127.0f),
            *tester.bounce(sample::Duration{bellSample.getFrameCount()},
                           tester.newTimeline()
                             .note(Midi::C(3), sample::Duration{bellSample.getFrameCount()}, 100)));

  // play D#3 (Chip) (gain = 1.0) ...
  // play F#3 (Machina) (gain = 1.0) ...
  // play A3 (MkIV) (gain = 1.0) ...

  // Expected result is 4 samples mixed
  auto mixedSample = bellSample->clone().mixWith(*chipSample).mixWith(*machinaSample).mixWith(*mkivSample);

  ASSERT_EQ(mixedSample,
            *tester.bounce(sample::Duration{mixedSample.getFrameCount()},
                           tester.newTimeline()
                             .note(Midi::C(3),       sample::Duration{bellSample.getFrameCount()},    127)
                             .note(Midi::D_sharp(3), sample::Duration{chipSample.getFrameCount()},    127)
                             .note(Midi::F_sharp(3), sample::Duration{machinaSample.getFrameCount()}, 127)
                             .note(Midi::A(3),       sample::Duration{mkivSample.getFrameCount()},    127)));
}
```

* Like in the previous example, we initialize the tester. Note how the class is `CVerySimpleSampler` and we call `disable_trace()` because the device generates a lot of traces otherwise (due to `JBOX_TRACE("NoteOff")` in the `VoicePool` class).

* The output sockets are wired (called `left` and `right` for this device).

* The built-in samples are pre-loaded because we will use them to test the output

* The next section demonstrates how to simulate user input by changing the value of any motherboard property. In this case we change the volume to 127 (unity gain) and set the proper root key for each of the 4 samples.
    > #### Note
    > The device does not adjust the root key (which I think is a bug/mistake): for example in order to play "chip" you have to press D#3, but that does not mean that it should sound like if its root key is C3, and it has been transposed to D#3... This is why we set the proper root key

* The previous example used a Midi file and the sequencer track to play some notes. This example uses a different technique which is equivalent to loading the plugin in Reason/Recon and simply hitting notes on a Midi keyboard (without entering them on the sequencer track). It uses the concept of a `Timeline` which describes what happens during the various `bounce`, `play` or `nextBatches` call:
  ```cpp
     // timeline describes holding the C3 note from the beginning of the bounce call for a duration of 
     // "bellSample.getFrameCount()" and a velocity of 127
     tester.bounce(sample::Duration{bellSample.getFrameCount()},
                   tester.newTimeline()
                     .note(Midi::C(3), sample::Duration{bellSample.getFrameCount()}, 127)))
  ```
* The next assert shows how to easily apply gain to a sample (`bellSample->clone().applyGain(100/127.0f)`)
* The next 3 asserts are not shown for brevity since they do the same for the other samples
* Finally, the last assert shows how to mix multiple samples together and how the timeline describes holding 4 notes with various durations

### Next

The [Documentation](Documentation.md) page describes some of those concepts in more details.