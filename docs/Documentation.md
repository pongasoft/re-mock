Documentation
-------------

All classes are under the `re::mock` namespace.

### The rack

The main API/entry point of the framework is the `Rack`. You create a `Rack` on which you add and wire extensions.
You then call `Rack::nextFrame()` which builds the dependency graph of the extensions and call the 
_render real time_ api accordingly (usually `JBox_Export_RenderRealtime` for the device under test) in the proper 
order dictated by the dependency graph.

Between each call to `Rack::nextFrame()` you are free to modify the properties of each device (simulating user action, 
automation, etc...), wiring, unwiring, devices and check expected values (for example, making sure that an LED light
representing _sound on_ has been turned on).

If the device under test uses realtime property change notification (`rt_input_setup / notify`), these changes are 
automatically made available to the device via the `iPropertyDiffs` parameter whenever the `Rack::nextFrame()` call 
is made.

### Testers

The concept of testers is just a thin wrapper around the rack in order to simplify some of the most common wiring and
use cases, which is why it is the recommended concept to write your test with. For example, if the device under test is a note player, then the `NotePlayerTester` will automatically 
create and wire a `MNPSrc` (note player source device) and a `MNPDst` (note player destination device) and provide more
convenient APIs for this particular use case.

The class `DeviceTester` represents the base class for all the testers provided by this framework:

- `HelperTester`
- `StudioEffectTester`
- `CreativeEffectTester`
- `InstrumentTester`
- `NotePlayerTester`

The generic type provided to the tester is the type of the device under test (the type of the "private state" provided to `JBox_Export_RenderRealtime`), for example `StudioEffectTester<MyEffect>`.

Each tester creates a basic test infrastructure for each type of device:

1. a rack is created and the device to test is automatically added to it
2. the individual tester creates more devices and wire them to the device to test (see each tester for details)
3. the rack can be accessed directly via the `DeviceTester::rack()` api in case there is a (rare) need
4. each tester provides a `device()` api which gives access to the device under test thus allowing:
   - direct manipulation of the device itself (using the arrow notation) (ex: `tester.device()->xxx`)
   - manipulation of the properties of the device in the motherboard, simulating user input or automation 
     (ex: `tester.device().setBool("/custom_properties/my_bool_prop", true);`)

#### HelperTester

You use this tester when your device is a helper/utility (`device_type="helper"` in `info.lua`). This tester does not
create or wire any other devices as it is not really possible to determine what the helper will do. 
Refer to the section [How to test sockets?](#how-to-test-sockets) for details on how to wire devices based on the sockets exposed by the 
device.

#### StudioEffectTester and CreativeEffectTester

You use these testers when your device is an effect (`StudioEffectTester` for `device_type="studio_fx"`, 
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

The tester provides a convenient `nextFrame` api which automatically injects the audio stereo buffer (64 samples) in the 
input (main in) and returns the audio stereo buffer (64 samples) from the output (main out). In other words:

```c++
auto inputBuffer = ...;

// this convenient api
auto outputBuffer = tester.nextFrame(inputBuffer);

// is 100% equivalent to:
tester.src()->fBuffer = inputBuffer;
tester.rack().nextFrame();
auto outputBuffer = tester.dst()->fBuffer;
```

#### InstrumentTester

You use this tester when your device is an instrument (`device_type="instrument"` in `info.lua`).

Because an instrument is designed to generate sound, this tester automatically creates:

- a destination of audio of type `MAUDst` (accessible via `tester.dst()`)

After instantiating the tester, you need to wire your out sockets by calling `tester.wireMainOut(...)`. 
This allows you to control exactly when the wiring happens (for example if you want to test the device when 
nothing is connected).

Note that the `wireMainOut` api uses `std::optional` in the event you only have one socket to wire (mono device).

The tester provides a convenient `nextFrame` api which lets you provide note events and returns the audio 
stereo buffer (64 samples) from the output (main out). In other words:

```c++
auto noteEvents = MockNotePlayer::NoteEvents{}.noteOn(69);

// this convenient api
auto outputBuffer = tester.nextFrame(noteEvents);

// is 100% equivalent to:
tester.device().setNoteInEvents(noteEvents.events());
tester.rack().nextFrame();
auto outputBuffer = tester.dst()->fBuffer;
```

#### NotePlayerTester

You use this tester when your device is a note player (`device_type="note_player"` in `info.lua`).

Because a note player is designed to generate notes (while potentially being in a chain of note players), 
this tester automatically creates:

- a source of note events of type `MNPSrc` (accessible via `tester.src()`)
- a destination of note events of type `MNPDst` (accessible via `tester.dst()`)

The tester provides a shortcut to get (resp. set) the bypass state of the note player (`isBypassed` resp. `setBypassed`).

The tester provides a convenient `nextFrame` api which lets you provide note events (from a potential previous 
note player) and returns the note events generated by the note player under test. In other words:

```c++
auto noteEvents = MockNotePlayer::NoteEvents{}.noteOn(69);

// this convenient api
auto events = tester.nextFrame(noteEvents);

// is 100% equivalent to:
tester.src()->fNoteEvents = noteEvents;
tester.rack().nextFrame();
auto events = tester.dst()->fNoteEvents;
```

### Accessing the device under test

The device under test is always accessible via `tester.device()` and you can access either the motherboard/properties
side of it, or directly the device instance (the one created by `JBox_Export_CreateNativeObject`).

#### Accessing properties

Using the _dot_ notation, you have access to many convenient apis to directly read or write the device motherboard properties. For example:

```c++
 // returns the gain property value as a number (TJBox_Float64)
 auto gain = tester.device().getNum("/custom_properties/gain");
 
// returns the my_int_prop value as an int
 auto p = tester.device().getNum<int>("/custom_properties/my_int_prop");
```

A lower level api lets you access the device via the Jukebox api (although it is far more verbose):
```c++
tester.device().use([] {
  auto customProperties = JBox_GetMotherboardObjectRef("/custom_properties");
  auto gain = JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "gain"))));
  auto p = static_cast<int>(JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "my_int_prop"))));
});
```

> #### Note
> Since the Jukebox API is "device" free (meaning there is no way to know on which device the call is supposed 
> to be directed to), you need to wrap it in a `device.use([]{/* access Jukebox here */});` statement.

#### Accessing the implementation

Using the _arrow_ notation, you have access to the implementation class directly (the one created by `JBox_Export_CreateNativeObject` and used in `JBox_Export_RenderRealtime` as the private state).

```c++
tester.device()->fMyProperty;

// which is 100% equivalent to
tester.device().getNativeObjectRW<Device>("/custom_properties/instance")->fMyProperty;
```

### How to test sockets?
In general, for each additional socket not handled by the given tester, you can add a `MockDevice` to test it:

#### Testing a Stereo Audio Input socket

You can use a `MAUSrc` mock device which is a source of stereo audio value (set `fBuffer` to the value you want 
to be made available to your audio input prior to calling `nextFrame`).

Example:
```c++
// assuming the name of the input sockets are "aui_left" and "aui_right"
auto auSrc = tester.wireNewAUSrc("aui_left", "aui_right");

// set its buffer prior to calling nextFrame
auSrc->fBuffer = MockAudioDevice::buffer(0.5, 0.6);

// next frame => the device will "receive" the audio buffer on its "aui_left" and "aui_right" sockets
tester.nextFrame(...);
```

#### Testing a Stereo Audio Output socket

You can use a `MAUDst` mock device which is a destination of stereo audio value (its `fBuffer` is populated by whatever
buffer it received during `nextFrame`).

Example:
```c++
// assuming the name of the output sockets are "auo_left" and "auo_right"
auto auDst = tester.wireNewAUDst("auo_left", "auo_right");

// next frame => the device generates the audio buffers on its "auo_left" and "auo_right" sockets
tester.nextFrame(...);

// auDst has received the buffer that the device did output (this example assumes that the device has divided
// the input by 2.0)
ASSERT_EQ(MockAudioDevice::buffer(0.5 / 2.0, 0.6 / 2.0), auDst->fBuffer);
```

#### Testing a CV Input socket

You can use a `MCVSrc` mock device which is a source of CV value (set `fValue` to the value you want to be made 
available on your cv input prior to calling `nextFrame`).

Example:
```c++
// assuming the name of the input socket is "cvi"
auto cvSrc = tester.wireNewCVSrc("cvi");

// set its value prior to calling nextFrame
cvSrc->fValue = 1.0;

// next frame => the device will "receive" the cv value on its "cvi" socket
tester.nextFrame(...);
```

#### Testing a CV Output socket

You can use a `MCVDst` mock device which is a destination of CV value (its `fValue` is populated by whatever
value it received during `nextFrame`).

Example:
```c++
// assuming the name of the output socket is "cvo"
auto cvDst = tester.wireNewCVDst("cvo");

// next frame => the device outputs the cv value on its "cvo" socket
tester.nextFrame(...);

// cvDst has received the cv value that the device did output
ASSERT_FLOAT_EQ(<expected value>, cvDst->fValue);
```

### How to deal with `/transport`?

In order to change one of the globally available transport properties (like "playing"), you call an API on the rack
itself.

Example:
```c++
// this changes the transport "playing" property for all extensions
rack.setTransportPlaying(true); // rack.transportStart(); for a shortcut notation
```