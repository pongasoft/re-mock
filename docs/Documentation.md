Documentation
-------------

It is strongly recommended reading the [Quick Start Guide](Quick_Start.md) prior to reading this section. This guide mostly follows the concepts in the order they are encountered when writing a test.

> #### Note
> All classes are under the `re::mock` namespace, so it will be omitted from now on.

### The main event loop

Recon/Reason has a main event loop, calling `JBox_Export_RenderRealtime` over and over as fast as necessary. For example, at a sample rate of 48000, it calls this function 750 times per second which can make logging and/or debugging quite challenging.

`re-mock` puts you in charge of the main event loop, so you decide when this API is called. For example:

* invoking `tester.nextBach()` runs through exactly one iteration of this loop.
* invoking `tester.nextBatches(time::Duration{1000})` runs through 750 iterations (assuming 48000 sample rate which can be chosen when instantiating the tester).

By controlling precisely the main event loop, it becomes easy to set the device under test in a precise configuration to be able to run some tests.

### `DeviceConfig`

In order to instantiate a rack extension you need to first build a `DeviceConfig` for it. Although the configuration object can be manually created (for very simple devices) it is strongly recommended using the convenient API `DeviceConfig<T>::fromJBoxExport(std::string const &iDeviceRootFolder)`: this API automatically loads the 3 main lua files describing the rack extension (`info.lua`, `motherboard_def.lua` and `relatime_controller.lua`) and populates the configuration from it.

> #### Note
> * `T` is the cpp class representing the main instance of the device. For example `CVerySimpleSampler`.
> * When using `re-cmake`, `iDeviceRootFolder` is the convenient "define" `RE_CMAKE_PROJECT_DIR`

Once `DeviceConfig` is instantiated you can further configure it by using a builder pattern (all apis return the object itself)

```cpp
auto c = DeviceConfig<Device>::fromJBoxExport(RE_CMAKE_PROJECT_DIR)
  .disable_trace()                       // disable output from JBOX_TRACE
  .default_patch("/Public/Init.repatch") // change the default patch defined in info.lua
  ;
```

Of note in this API is the ability to inject/replace patches, blobs and samples that are bundled with the device with other ones for testing purposes. For example:

```cpp
// replaces the /Private/sample.wav file with something much simpler for testing purposes
auto c = DeviceConfig<Device>::fromJBoxExport(RE_CMAKE_PROJECT_DIR)
  .sample_data("/Private/sample.wav", resource::Sample{}.sample_rate(44100).channels(1).data({0,0.5,0.75,1}))
  ;
```

The API also offers the ability to inject errors and control partial loading (for samples and blobs) and this is covered in the [Advanced Topics](Advanced_Topics.md) guide.

### Testers

Once the device configuration is created a tester is instantiated. The tester is the main entry point to the testing framework as it does the heavy lifting of creating all the necessary pieces and wiring them together as well as offering a wealth of convenient APIs to facilitate testing.

The class `DeviceTester` represents the base class for all the testers provided by this framework:

- `HelperTester`
- `StudioEffectTester`
- `CreativeEffectTester`
- `InstrumentTester`
- `NotePlayerTester`

The generic type provided to the tester is the type of the device under test (the same type provided to `DeviceConfig`), for example `InstrumentTester<CVerySimpleSampler>`.

Each tester creates a basic test infrastructure for each type of device:

1. a rack (`Rack` concept) is created and the device to test is automatically added to it
2. the individual tester creates more devices and wire them to the device to test (see each tester for details)
3. the rack can be accessed directly via the `DeviceTester::rack()` api in case there is a (rare) need
4. each tester provides a `device()` api which gives access to the device under test thus allowing:
   - direct manipulation of the device itself (using the arrow notation) (ex: `tester.device()->xxx`)
   - manipulation of the properties of the device in the motherboard, simulating user input or automation 
     (ex: `tester.device().setBool("/custom_properties/my_bool_prop", true);`)

This base class has many methods to wire/unwire other devices and offers 2 primary set of APIs to manage the event loop:

* `nextBatches` which advances the event loop without having the transport playing (equivalent to loading a Rack Extension in Recon/Reason while not pressing "Play")
* `play` which advances the event loop while having the transport playing

> ### Note
> `play` is (conceptually) a wrapper around
>  ```cpp
>  tester.transportStart();
>  tester.nextBatches(...);
>  tester.transportStop();
>  ```

#### HelperTester

This tester is used when the device is a helper/utility (`device_type="helper"` in `info.lua`). This tester does not
create or wire any other devices as it is not really possible to determine what the helper will do. 
Refer to the section [How to test sockets?](#how-to-test-sockets) for details on how to wire devices based on the sockets exposed by the 
device.

#### StudioEffectTester and CreativeEffectTester

These testers are used when the device is an effect (`StudioEffectTester` for `device_type="studio_fx"`, 
resp. `CreativeEffectTester` for `device_type="creative_fx"` in `info.lua`). 

Because an effect is designed to process audio, these testers automatically creates:

- a source of audio of type `MAUSrc` (accessible via `tester.src()`)
- a destination of audio of type `MAUDst` (accessible via `tester.dst()`)

After instantiating the tester, you need to wire your main in/out sockets by calling `tester.wireMainIn(...)` and 
`tester.wireMainOut(...)`. This allows you to control exactly when the wiring happens (for example if you want to test
the device when nothing is connected).

Note that the `wireMainIn` (resp. `wireMainOut`) api uses `std::optional` in the event you only have one socket to 
wire (mono device).

The tester provides a shortcut to get (resp. set) the bypass state of the effect (`getBypassState` resp. `setBypassState`).

This tester provides another convenient `nextBatch` api which automatically injects the audio stereo buffer (64 samples) in the 
input (main in) and returns the audio stereo buffer (64 samples) from the output (main out). In other words:

```cpp
auto inputBuffer = ...;

// this convenient api
auto outputBuffer = tester.nextBatch(inputBuffer);

// is 100% equivalent to:
tester.src()->fBuffer = inputBuffer;
tester.rack().nextBatch();
auto outputBuffer = tester.dst()->fBuffer;
```

The `nextBatch` api is very fine-grained since it deals with only 1 (rack) frame at a time (64 samples). This tester offers a higher level api which lets you deal with an entire sample (including adding an optional tail so that the device runs past the end of the sample).

```cpp
auto sinePath = fmt::path(RE_CMAKE_PROJECT_DIR, "test", "resources", "audio", "sine.wav");
auto sine = tester.loadSample(resource::File{sinePath});
auto processedSine = tester.processSample(resource::File{sinePath}); // you can optionally add a "tail" (check API)

auto processedSinePath = fmt::path(RE_CMAKE_PROJECT_DIR, "test", "resources", "audio", "processed_sine.wav");
auto expectedProcessedSine = tester.loadSample(resource::File{processedSinePath});

ASSERT_EQ(expectedProcessedSine, processedSine); // compare the 2 samples
```

#### InstrumentTester

This tester is used when the device is an instrument (`device_type="instrument"` in `info.lua`).

Because an instrument is designed to generate sound, this tester automatically creates:

- a destination of audio of type `MAUDst` (accessible via `tester.dst()`)

After instantiating the tester, you need to wire your out sockets by calling `tester.wireMainOut(...)`. This allows you to control exactly when the wiring happens (for example if you want to test the device when nothing is connected).

Note that the `wireMainOut` api uses `std::optional` in the event you only have one socket to wire (mono device).

The tester provides an additional convenient `nextBatch` api which lets you provide note events and returns the audio stereo buffer (64 samples) from the output (main out). In other words:

```cpp
auto noteEvents = MockNotePlayer::NoteEvents{}.noteOn(Midi::A_440);

// this convenient api
auto outputBuffer = tester.nextBatch(noteEvents);

// is 100% equivalent to:
tester.setNoteEvents(noteEvents);
tester.rack().nextBatch();
auto outputBuffer = tester.dst()->fBuffer;
```

The `nextBatch` api is very fine-grained since it deals with only 1 (rack) frame at a time (64 samples).
This tester offers a higher level api which lets you bounce the output of the instrument for a given duration (it returns a sample).

```cpp
auto sample = tester.bounce(time::Duration{1000},
                            tester.newTimeline()
                              .note(Midi::A(3), sequencer::Duration(0,1,0,0))); // hold A3 for 1 beat

auto path = fmt::path(RE_CMAKE_PROJECT_DIR, "test", "resources", "audio", "instrument_A3_1beat.wav");
auto expectedSample = tester.loadSample(resource::File{path});

ASSERT_EQ(expectedSample, sample); // compare the 2 samples
```

> ### Note
> `bounce` runs the event loop without playing the transport whereas `bouncePlay` start and stop the transport

#### NotePlayerTester

This tester is used when the device is a note player (`device_type="note_player"` in `info.lua`).

Because a note player is designed to generate notes (while potentially being in a chain of note players), this tester automatically creates:

- a source of note events of type `MNPSrc` (accessible via `tester.src()`)
- a destination of note events of type `MNPDst` (accessible via `tester.dst()`)

The tester provides a shortcut to get (resp. set) the bypass state of the note player (`isBypassed` resp. `setBypassed`).

The tester provides a convenient `nextBatch` api which lets you provide note events (from a potential previous note player) and returns the note events generated by the note player under test. In other words:

```cpp
auto noteEvents = MockNotePlayer::NoteEvents{}.noteOn(69);

// this convenient api
auto events = tester.nextBatch(noteEvents);

// is 100% equivalent to:
tester.src()->fNoteEvents = noteEvents;
tester.rack().nextBatch();
auto events = tester.dst()->fNoteEvents;
```

> ### Note
> TODO: The `nextBatch` api is very fine-grained since it deals with only 1 (rack) frame at a time (64 samples). At this time no other convenient API is provided, but it should be possible to add some API that processes multiple batches and return a Midi file (`smf::MidiFile`).

### Accessing the device under test

The device under test is always accessible via `tester.device()` and you can access either the motherboard/properties side of it, or directly the device instance (the one created by `JBox_Export_CreateNativeObject`).

#### Accessing properties

Using the _dot_ notation, you have access to many convenient apis to directly read or write the device motherboard properties. For example:

```cpp
 // returns the gain property value as a number (TJBox_Float64)
 auto gain = tester.device().getNum("/custom_properties/gain");
 
// returns the my_int_prop value as an int
 auto p = tester.device().getNum<int>("/custom_properties/my_int_prop");
```

A lower level api lets you access the device via the Jukebox api (although it is far more verbose):
```cpp
tester.device().use([] {
  auto customProperties = JBox_GetMotherboardObjectRef("/custom_properties");
  auto gain = JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "gain"))));
  auto p = static_cast<int>(JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "my_int_prop"))));
});
```

> #### Note
> Since the Jukebox API is "device" free (meaning there is no way to know on which device the call is supposed to be directed to), you need to wrap it in a `device.use([]{/* access Jukebox here */});` statement.

#### Accessing the implementation

Using the _arrow_ notation, you have access to the implementation class directly (the one created by `JBox_Export_CreateNativeObject` and used in `JBox_Export_RenderRealtime` as the private state).

```cpp
tester.device()->fMyProperty;

// which is 100% equivalent to
tester.device().getNativeObjectRW<Device>("/custom_properties/instance")->fMyProperty;
```

### How to test sockets?
In general, for each additional socket not handled by the given tester, you can add a `MockDevice` to test it:

#### Testing a Stereo Audio Input socket

You can use a `MAUSrc` mock device which is a source of stereo audio value (set `fBuffer` to the value you want to be made available to your audio input prior to calling `nextBatch`).

Example:
```cpp
// assuming the name of the input sockets are "aui_left" and "aui_right"
auto auSrc = tester.wireNewAUSrc("aui_left", "aui_right");

// set its buffer prior to calling nextBatch
auSrc->fBuffer = MockAudioDevice::buffer(0.5, 0.6);

// next frame => the device will "receive" the audio buffer on its "aui_left" and "aui_right" sockets
tester.nextBatch(...);
```

#### Testing a Stereo Audio Output socket

You can use a `MAUDst` mock device which is a destination of stereo audio value (its `fBuffer` is populated by whatever buffer it received during `nextBatch`).

Example:
```cpp
// assuming the name of the output sockets are "auo_left" and "auo_right"
auto auDst = tester.wireNewAUDst("auo_left", "auo_right");

// next batch => the device generates the audio buffers on its "auo_left" and "auo_right" sockets
tester.nextBatch(...);

// auDst has received the buffer that the device did output (this example assumes that the device has divided
// the input by 2.0)
ASSERT_EQ(MockAudioDevice::buffer(0.5 / 2.0, 0.6 / 2.0), auDst->fBuffer);
```

#### Testing a CV Input socket

You can use a `MCVSrc` mock device which is a source of CV value (set `fValue` to the value you want to be made available on your cv input prior to calling `nextBatch`).

Example:
```cpp
// assuming the name of the input socket is "cvi"
auto cvSrc = tester.wireNewCVSrc("cvi");

// set its value prior to calling nextBatch
cvSrc->fValue = 1.0;

// next batch => the device will "receive" the cv value on its "cvi" socket
tester.nextBatch(...);
```

#### Testing a CV Output socket

You can use a `MCVDst` mock device which is a destination of CV value (its `fValue` is populated by whatever value it received during `nextBatch`).

Example:
```cpp
// assuming the name of the output socket is "cvo"
auto cvDst = tester.wireNewCVDst("cvo");

// next batch => the device outputs the cv value on its "cvo" socket
tester.nextBatch(...);

// cvDst has received the cv value that the device did output
ASSERT_FLOAT_EQ(<expected value>, cvDst->fValue);
```

### Accessing the rack

The tester lets you access the main device under test with `tester.device()` and the rack with `tester.rack()`. The rack (`Rack`) is actually the main class that models the full system (the concept of testers is merely a wrapper around the rack to provide higher level APIs designed to facilitate testing a single device). You need to access the rack when you want to access system level properties, like the tempo, the play position, etc...

### The sequencer track

Each device has a sequencer track. You can access the sequencer track of the device under test with the shortcut `tester.sequencerTrack()`. This lets you add events like you would on the sequencer track in Reason/Recon. The class `sequencer::Track` has a builder pattern API to help in adding events:

```cpp
tester.sequencerTrack()
  .note(Midi::A(3), sequencer::Time(2,1,1,0), sequencer::Duration(0,0,0,120), 99)
  .at(sequencer::Time(3,1,1,0))
  .note(Midi::C(3), sequencer::Duration::k1Beat_4x4, 127) // uses the previous at() call for "when" it happens 
```

This would add an A3 note (with a velocity of 99) at 2.1.1.0 for 120 ticks and a C3 note (with a velocity of 127) at 3.1.1.0 for 1 beat.

> #### Note
> This code will generate 4 events: 2 "note on" events and 2 "note off" events

Note that the API is not limited to adding notes as you can also add any kind of generic events like modifying a property. For example:

```cpp
tester.sequencerTrack()
  .reset()
  .at(sequencer::Time(3,1,1,0))
  .event([&tester]() { tester.device().setBool("/custom_property/my_bool_prop", false); })
```

This clears all events from the track and add a single one at time 3.1.1.0 that will set the `my_bool_prop` to `false`.

> #### Note
> As is the case with Reason/Recon, the sequencer track is only active if the transport is playing which can either be achieved by calling `tester.transportStart()` directly or using the `play` (or other APIs) which automatically handle playing/stopping the transport.

### The timeline

The concept of timeline (`tester::Timeline`) is similar to the concept of sequencer track, the main differences being that it is active whether the transport is playing or not.

As a result the concept at "time" in the timeline is measured in batches since when it starts.

The timeline makes writing batch based code much simpler. For example:

```cpp
tester.newTimeline()
  .after(sequencer::Duration::k1Beat_4x4)
  .note(Midi::C(3), sequencer::Duration(1,0,0,0))
  .after(sequencer::Duration::k1Beat_4x4)
  .note(Midi::C(4), sequencer::Duration(1,0,0,0)))
  .execute(sequencer::Duration::k1Bar_4x4 * 2)
  
// would be equivalent to (and I am not 100% it is right ;)
tester.nextBatches(sequencer::Duration::k1Beat_4x4) // 1 beat
tester.setNoteInEvents(MockDevice::NoteEvents{}.noteOn(Midi::C(3)));
tester.nextBatches(sequencer::Duration::k1Beat_4x4) // 1 beat
tester.setNoteInEvents(MockDevice::NoteEvents{}.noteOn(Midi::C(4)));
tester.nextBatches(sequencer::Duration::k1Beat_4x4 * 3); // 3 beats
tester.setNoteInEvents(MockDevice::NoteEvents{}.noteOff(Midi::C(3)));
tester.nextBatches(sequencer::Duration::k1Beat_4x4) // 1 beat
tester.setNoteInEvents(MockDevice::NoteEvents{}.noteOff(Midi::C(4)));
tester.nextBatches(sequencer::Duration::k1Beat_4x4 * 2); // 2 bars is 8 beats so 8 - 6 = 2
```

Similarly to the sequencer track, you can add any kind of generic (not note related) events.

### Next

This page discusses the main concepts and classes from the framework. The [Advanced Topics](Advanced_Topics.md) guide describes less used concepts.