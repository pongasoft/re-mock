/*
 * Copyright (c) 2021 pongasoft
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 *
 * @author Yan Pujante
 */

#pragma once
#ifndef __Pongasoft_re_mock_device_testers_h__
#define __Pongasoft_re_mock_device_testers_h__

#include "Rack.h"
#include "MockDevices.h"
#include <limits>

namespace re::mock {

class DeviceTester;

namespace tester {

/**
 * The concept of timeline is similar to the concept of sequencer track, the main difference being that it is active
 * whether the transport is playing or not. As a result the concept at "time" in the timeline is measured in
 * batches since when it starts.
 *
 * The timeline makes writing batch based code much simpler. For example:
 *
 * ```cpp
 * tester.newTimeline()
 *  .after(sequencer::Duration::k1Beat_4x4)
 *  .note(Midi::C(3), sequencer::Duration(1,0,0,0))
 *  .after(sequencer::Duration::k1Beat_4x4)
 *  .note(Midi::C(4), sequencer::Duration(1,0,0,0)))
 *  .execute(sequencer::Duration::k1Bar_4x4 * 2);
 * ```
 *
 * @note Most APIs return the timeline itself for easy chaining
 *
 * @see `re::mock::Duration` for the variants the api accepts
 */
class Timeline
{
public:
  //! A simpler definition of an event (interpreted as always returning `true`)
  using SimpleEvent = std::function<void()>;

  /**
   * Definition of an event which is a function which takes the current batch.
   *
   * @return `true` if the timeline should proceed or `false` if the time should stop */
  using Event = std::function<bool(long iAtBatch)>;

  //! An event that does nothing
  static const Event kNoOp;

  //! An event that terminates the timeline (event that returns `false`)
  static const Event kEnd;

  //! Wraps a simple event into an event (ignores parameter and returns `true`)
  static Event wrap(SimpleEvent iEvent);

public:
  /**
   * Moves the timeline by the given duration. All event calls after this one will happen after the duration has
   * passed. Example:
   *
   * ```cpp
   * tester.newTimeline()
   *  .after(sequencer::Duration::k1Beat_4x4)
   *  .note(Midi::C(3), sequencer::Duration(1,0,0,0))
   *  .execute(sequencer::Duration::k1Bar_4x4 * 2);
   * ```
   *
   * The C3 "note On" event will happen after enough batches have elapsed to represent 1 beat. */
  Timeline &after(Duration iDuration);

   //! Moves the timeline by exactly one batch
  Timeline &after1Batch() { return after(rack::Duration{1}); }

  //! Add an event (happens at the "current" batch based on calls to `Timeline::after()`)
  Timeline &event(Event iEvent) { return event(fCurrentBath, std::move(iEvent)); }

  //! Add a simple event (happens at the "current" batch based on calls to Timeline::after)
  Timeline &event(SimpleEvent iEvent) { return event(fCurrentBath, std::move(iEvent)); }

  /**
   * Add an event that happens on every batch (**before** `Rack::nextBatch()` is called). This API for example
   * allows writing a timeline that runs until some condition is met. Example:
   *
   * ```cpp
   * tester.newTimeline()
   *  .event([&tester](long iAtBatch) {
   *    // if(<some condition met>)
   *    //   return false;
   *    })
   *  .execute(timeline::Duration{}); // runs until an event returns false
   * ```
   */
  Timeline &onEveryBatch(Event iEvent);

  //! Add a simple event that happens on every batch (**before** `Rack::nextBatch()` is called)
  Timeline &onEveryBatch(SimpleEvent iEvent) { return onEveryBatch(wrap(iEvent)); }

  /**
   * Starts the transport (equivalent to pressing "Play" in Reason)
   *
   * @note This method ignores the `DeviceTester` concept of transport enabled/disabled and forces the transport to start. */
  Timeline &transportStart();

  //! Stops the transport (equivalent to pressing "Stop" in Reason)
  Timeline &transportStop();

  /**
   * Inserts an event that ends the timeline at the current batch. For example:
   *
   * ```cpp
   * tester.newTimeline()
   *   .after(time::Duration{250})     // after 250ms
   *   .event(...)                     // do something
   *   .after(time::Duration{250})     // after another 250ms
   *   .end()                          // end the timeline (so it runs for 500ms)
   *   .execute(timeline::Duration{}); // runs until an event returns false
   * ```
   */
  Timeline &end() { return event(kEnd); }

  //! Add the note events at the current batch
  Timeline &notes(MockDevice::NoteEvents iNoteEvents);

  /**
   * Adds a "Note on" event at the current batch and a "Note off" event after the duration.
   *
   * @note The timeline current batch is **not** moved by `iDuration`  */
  Timeline &note(TJBox_UInt8 iNoteNumber, Duration iDuration, TJBox_UInt8 iNoteVelocity = 100);

  /**
   * Execute the events in the timeline for the given duration and with or without the transport playing.
   *
   * @param iWithTransport if `true`, starts the transport before executing the events and stops it when done
   * @param iDuration if the duration is not provided, it stops at current batch + 1
   *
   * @note If the transport is disabled (see `DeviceTester::disableTransport()`), then using `true` for this API is
   *       behaves like using `false`! */
  void execute(bool iWithTransport, std::optional<Duration> iDuration = std::nullopt) const;

  //! Executes the timeline (does not play the transport)
  inline void execute(std::optional<Duration> iDuration = std::nullopt) const { execute(false, iDuration); }

  //! Executes the timeline while playing the transport
  inline void play(std::optional<Duration> iDuration = std::nullopt) const { execute(true, iDuration); }

  // required for compilation
  friend class re::mock::DeviceTester;

private:
  //! Private constructor
  Timeline(DeviceTester *iTester) : fTester{iTester} {}

  // add an event at the given batch
  Timeline &event(size_t iAtBatch, Event iEvent);

  // add a simple event at the given batch
  inline Timeline &event(size_t iAtBatch, SimpleEvent iEvent) { return event(iAtBatch, wrap(std::move(iEvent))); }

private:
  struct EventImpl
  {
    int fId;
    size_t fAtBatch;
    Event fEvent;
  };

protected:
  void ensureSorted() const;

  std::vector<EventImpl> const &getEvents() const { ensureSorted(); return fEvents; }

  size_t executeEvents(std::optional<Duration> iDuration) const;

private:
  DeviceTester *fTester;
  size_t fCurrentBath{};
  std::vector<EventImpl> fEvents{};
  mutable bool fSorted{true};
  int fLastEventId{};
  std::vector<Event> fOnEveryBatchEvents{};
};

}

/**
 * This class represents the base class for all the testers provided by this framework:
 *
 * - HelperTester
 * - StudioEffectTester
 * - CreativeEffectTester
 * - InstrumentTester
 * - NotePlayerTester
 *
 * Check the documentation for more details https://github.com/pongasoft/re-mock/blob/master/docs/Documentation.md
 */
class DeviceTester
{
public:
  //! Constructor (called by subclasses)
  explicit DeviceTester(Config const &iDeviceConfig, int iSampleRate);

  //! Returns the rack for lower level access
  inline Rack &rack() { return fRack; }

  //! Returns the rack for lower level access
  inline Rack const &rack() const { return fRack; }

  //! Wires an existing mock audio source (`MAUSrc`) to the left and right (in) sockets of the device under test
  rack::ExtensionDevice<MAUSrc> &wire(rack::ExtensionDevice<MAUSrc> &iSrc,
                                      std::optional<std::string> iLeftInSocketName = std::nullopt,
                                      std::optional<std::string> iRightInSocketName = std::nullopt);

  //! Creates and wires a mock audio source (`MAUSrc`) to the left and right (in) sockets of the device under test
  rack::ExtensionDevice<MAUSrc> wireNewAudioSrc(std::optional<std::string> iLeftInSocketName = std::nullopt,
                                                std::optional<std::string> iRightInSocketName = std::nullopt);

  //! Unwires the mock audio source provided
  void unwire(rack::ExtensionDevice<MAUSrc> &iSrc);

  //! Wires an existing mock audio destination (`MAUDst`) to the left and right (out) sockets of the device under test
  rack::ExtensionDevice<MAUDst> &wire(rack::ExtensionDevice<MAUDst> &iDst,
                                      std::optional<std::string> iLeftOutSocketName = std::nullopt,
                                      std::optional<std::string> iRightOutSocketName = std::nullopt);

  //! Creates and wires a mock audio destination (`MAUDst`) to the left and right (out) sockets of the device under test
  rack::ExtensionDevice<MAUDst> wireNewAudioDst(std::optional<std::string> iLeftOutSocketName = std::nullopt,
                                                std::optional<std::string> iRightOutSocketName = std::nullopt);

  //! Unwires the mock audio destination provided
  void unwire(rack::ExtensionDevice<MAUDst> &iDst);

  //! Wires an existing mock cv source (`MCVSrc`) to the named (in) socket of the device under test
  rack::ExtensionDevice<MCVSrc> &wire(rack::ExtensionDevice<MCVSrc> &iSrc, std::string const &iCVInSocketName);

  //! Creates and wires a mock cv source (`MCVSrc`) to the named (in) socket of the device under test
  rack::ExtensionDevice<MCVSrc> wireNewCVSrc(std::optional<std::string> iCVInSocketName = std::nullopt);

  //! Unwires the mock cv source
  void unwire(rack::ExtensionDevice<MCVSrc> &iSrc);

  //! Wires an existing mock cv destination (`MCVDst`) to the named (out) socket of the device under test
  rack::ExtensionDevice<MCVDst> &wire(rack::ExtensionDevice<MCVDst> &iDst, std::string const &iCVOutSocketName);

  //! Creates and wires a mock cv destination (`MCVDst`) to the named (out) socket of the device under test
  rack::ExtensionDevice<MCVDst> wireNewCVDst(std::optional<std::string> iCVOutSocketName = std::nullopt);

  //! Unwires the mock cv destination
  void unwire(rack::ExtensionDevice<MCVDst> &iDst);

  //! Creates and wires a mock note player source (`MNPSrc`) to the device under test
  rack::ExtensionDevice<MNPSrc> wireNewNotePlayerSrc();

  //! Unwires a mock note player source
  void unwire(rack::ExtensionDevice<MNPSrc> &iSrc);

  //! Creates and wires a mock note player destination (`MNPDst`) to the device under test
  rack::ExtensionDevice<MNPDst> wireNewNotePlayerDst();

  //! Unwires the mock note player destination
  void unwire(rack::ExtensionDevice<MNPDst> &iDst);

  //! Returns the sample rate
  int getSampleRate() const { return fRack.getSampleRate(); }

  //! Enables the transport
  inline void enableTransport() { fTransportEnabled = true; }

  /**
   * Disables the transport. If the transport was playing when this method is called, it stops the transport. After
   * this call, any call to `DeviceTester::transportStart()` is ignored (until `DeviceTester::enableTransport()`
   * is called again).
   *
   * This call makes `Timeline::play()` behaves like `Timeline::execute()` and is useful for calls that takes a
   * `Timeline` as a parameter. For example:
   *
   * ```cpp
   * auto timeline = ...;
   * tester.disableTransport();
   * tester.bounce(time::Duration({1000}), timeline);
   * tester.enableTransport();
   * ```
   *
   * The `InstrumentTester::bounce()` method calls `Timeline::play()` which plays the transport. If this is not desired,
   * then disabling the transport prior to the call ensures that it won't be playing during the bounce operation. */
  inline void disableTransport() { transportStop(); fTransportEnabled = false; }

   //! Starts the transport but **only** if the transport is enabled
  void transportStart();

  /**
   * Stops the transport if it was playing.
   *
   * @note Stopping the transport (like in Reason), sends a note stop events for all notes */
  void transportStop();

  //! Returns `true` if the transport is playing
  inline bool transportPlaying() const { return fRack.getTransportPlaying(); }

  //! Set the transport position to the beginning (1.1.1.0)
  inline void transportReset() { fRack.setTransportPlayPos(0); }

  //! Sends a reset audio request to all devices in the Rack (property `/transport/request_reset_audio`)
  void requestResetAudio() { fRack.requestResetAudio(); }

  //! Returns the current transport play position
  sequencer::Time transportPlayPos() const;

  //! Sets the current transport play position (does not start or stop the transport)
  void transportPlayPos(sequencer::Time iTime) { fRack.setTransportPlayPos(iTime); }

  //! Returns the tempo
  TJBox_Float64 transportTempo() const { return fRack.getTransportTempo(); }

  //! Sets the tempo
  void transportTempo(TJBox_Float64 iTempo) { fRack.setTransportTempo(iTempo); }

  //! Returns the time signature of the transport
  sequencer::TimeSignature transportTimeSignature() const { return fRack.getTransportTimeSignature(); }

  //! Sets the time signature of the transport
  void transportTimeSignature(sequencer::TimeSignature iTimeSignature) { fRack.setTransportTimeSignature(iTimeSignature); }

  //! Returns `true` if the transport is looping
  bool transportLoopEnabled() const { return fRack.getTransportLoopEnabled(); }

  //! Enable transport looping
  void transportEnableLoop() { fRack.setTransportLoopEnabled(true); }

  //! Disable transport looping
  void transportDisableLoop() { fRack.setTransportLoopEnabled(false); }

  //! Sets the transport loop start and end (equivalent to L and R lines in the Reason sequencer)
  void transportLoop(sequencer::Time iStart, sequencer::Time iEnd) { fRack.setTransportLoop(iStart, iEnd); }

  //! Sets the transport loop start and end (the end is computed as a duration from start)
  void transportLoop(sequencer::Time iStart, sequencer::Duration iDuration) { fRack.setTransportLoop(iStart, iDuration); }

  //! Gets the transport "end of the song" marker (equivalent to E line in the Reason sequencer)
  sequencer::Time transportSongEnd() const { return fRack.getSongEnd(); }

  //! Sets the transport "end of the song" marker (equivalent to E line in the Reason sequencer)
  void transportSongEnd(sequencer::Time iSongEnd) { fRack.setSongEnd(iSongEnd); }

  /**
   * Shortcut to set note events on the device under test
   *
   * @note The events are propagated to the device on the next batch */
  void setNoteEvents(MockDevice::NoteEvents iNoteEvents) { fDevice.setNoteInEvents(iNoteEvents.events()); }

  //! Returns the sequencer track of the device under test
  sequencer::Track &sequencerTrack() const { return fDevice.getSequencerTrack(); }

  //! Creates a new timeline that can either be executed (resp. played) or passed to various other calls
  tester::Timeline newTimeline() { return tester::Timeline(this); }

  //! Executes exactly one batch on the rack (which translates to one call to `JBox_Export_RenderRealtime` for all the devices in the rack).
  inline void nextBatch() { fRack.nextBatch(); }

  /**
   * Executes as many batches necessary to cover the given duration:
   *
   * @param iDuration the duration is converted into a number of batches (for example 1 second would be
   *                  converted to 750 batches with a sample rate of 48000)
   * @param iTimeline an optional timeline that can be executed while the batches are called */
  void nextBatches(Duration iDuration, std::optional<tester::Timeline> iTimeline = std::nullopt);

  //! Same as `iTimeline.execute()`
  inline void nextBatches(tester::Timeline iTimeline) { iTimeline.execute(); }

  /**
   * Executes as many batches necessary to cover the given duration while playing the transport, equivalent to:
   *
   * ```cpp
   * tester.transportStart();
   * tester.nextBatches(iDuration, iTimeline);
   * tester.transportStop();
   * ```
   *
   * @param iDuration the duration is converted into a number of batches (for example 1 second would be
   *                  converted to 750 batches with a sample rate of 48000)
   * @param iTimeline an optional timeline that can be executed while the batches are processed
   *
   * @note If the transport is disabled (`DeviceTester::disableTransport()`), then `play` behaves exactly like
   *       `nextBatches()` */
  void play(Duration iDuration, std::optional<tester::Timeline> iTimeline = std::nullopt);

  //! Same as `iTimeline.play()`
  inline void play(tester::Timeline iTimeline) { iTimeline.play(); }

  /**
   * Convenient method to load a sample given a path
   *
   * @return the sample (never `nullptr` as it will assert if not found)
   *
   * @note This is just a convenient wrapper and doesn't do anything with the sample besides loading it */
  std::unique_ptr<MockAudioDevice::Sample> loadSample(resource::File const &iSampleFile) const;

  /**
   * Convenient method to load a sample given a resource path (tied to the device under test)
   *
   * @return the sample (never `nullptr` as it will assert if not found)
   *
   * @note This is just a convenient wrapper and doesn't do anything with the sample besides loading it */
  std::unique_ptr<MockAudioDevice::Sample> loadSample(std::string const &iSampleResource) const;

  //! Convenient method to save the sample to a file (wav / 32 bits)
  void saveSample(MockAudioDevice::Sample const &iSample, resource::File const &iToFile) const;

  /**
   * When the device under test is first loaded, it has default values which can modified after creation and overriden
   * with a default patch. This call resets the device to its default values (similar to the "Reset Device"
   * option in Reason) */
  void deviceReset() { fDevice.reset(); }

  /**
   * Loads the midi file provided and populates the sequencer track (of the device under test) with
   * the notes contained on all tracks (if `iTrack` is set to `-1`) or a specific track.
   *
   * @param iTrack if `-1`, then all tracks are used otherwise only the specified track in the midi file is processed
   *               (with the first track being 0)
   * @param iImportTempo if set to `true` then the tempo contained in the midi file sets the tempo of the rack,
   *                     otherwise it is ignored
   */
  void importMidi(resource::File const &iMidiFile, int iTrack = -1, bool iImportTempo = true);

  friend class re::mock::tester::Timeline;

protected:
  template<typename Device>
  rack::ExtensionDevice<Device> getExtensionDevice();

  Rack fRack;
  rack::Extension fDevice;
  Config fDeviceConfig;
  bool fTransportEnabled{true};
};

/**
 * Tester class to test a helper/utility (`device_type="helper"` in `info.lua`).
 *
 * Since utilities can be pretty much anything, this tester does not provide any kind of default devices. The various
 * `DeviceTester::wire()` methods can be used to wire the various extra devices (both audio and cv) desired in writing
 * the test. */
template<typename Helper>
class HelperTester : public DeviceTester
{
public:
  //! Constructor
  explicit HelperTester(DeviceConfig<Helper> const &iDeviceConfig, int iSampleRate = 44100) :
    DeviceTester(iDeviceConfig.getConfig(), iSampleRate),
    fHelper{getExtensionDevice<Helper>()}
    {
      RE_MOCK_ASSERT(iDeviceConfig.getConfig().info().fDeviceType == DeviceType::kHelper);
    }

  //! Accesses the device under test (typed api)
  inline rack::ExtensionDevice<Helper> &device() { return fHelper; }

  //! Accesses the device under test (typed api)
  inline rack::ExtensionDevice<Helper> const &device() const { return fHelper; }

protected:
  rack::ExtensionDevice<Helper> fHelper;
};

/**
 * Base tester class to test an effect (`device_type="creative_fx"` or `device_type="studio_fx"` in `info.lua`).
 * The proper subclass should be instantiated instead: `StudioEffectTester` or `CreativeEffectTester`
 *
 * Since an effect is designed to process audio, this tester automatically creates a mock audio source (accessible via
 * `ExtensionEffectTester::src()`) and a mock audio destination (accessible via  `ExtensionEffectTester::dst()`)
 */
class ExtensionEffectTester : public DeviceTester
{
public:
  //! Constructor
  explicit ExtensionEffectTester(Config const &iDeviceConfig, int iSampleRate);

  //! Wire the main device input sockets to the mock audio source (optional used to wire only one of them if necessary)
  void wireMainIn(std::optional<std::string> iLeftInSocketName, std::optional<std::string> iRightInSocketName);

  //! Wire the main device output sockets to the mock audio destination (optional used to wire only one of them if necessary)
  void wireMainOut(std::optional<std::string> iLeftOutSocketName, std::optional<std::string> iRightOutSocketName);

  //! @see DeviceTester::nextBatch()
  using DeviceTester::nextBatch;

  //! Convenient method that injects `iInputBuffer` into `src()`, processes 1 batch and returns the result from `dst()`
  MockAudioDevice::StereoBuffer nextBatch(MockAudioDevice::StereoBuffer const &iInputBuffer);

  //! Convenient method that injects `iInputBuffer` into `src()`, processes 1 batch and copies the result from `dst()` to `oOutputBuffer`
  void nextBatch(MockAudioDevice::StereoBuffer const &iInputBuffer, MockAudioDevice::StereoBuffer &oOutputBuffer);

  /**
   * Processes an entire sample by calling `nextBatch()` repeatedly until the sample (+ optional tail) is fully
   * processed. The resulting sample is returned.
   *
   * This method turns the sample provided into chunks of 64 audio samples and processes them through `nextBatch()`
   * (thus through the device under test) then assembles the result into a full sample.
   *
   * @param iSample the sample to process
   * @param iTail an optional tail (for example if the effect is a reverb, there is more samples
   *              generated after the sample is fully processed)
   * @param iTimeline an optional timeline that is played while the sample is processed (if events need to be injected)
   * @return the sample (never `nullptr`) */
  std::unique_ptr<MockAudioDevice::Sample> processSample(MockAudioDevice::Sample const &iSample,
                                                         std::optional<Duration> iTail = std::nullopt,
                                                         std::optional<tester::Timeline> iTimeline = std::nullopt);

  /**
   * Processes a sample from the file system
   *
   * @see processSample(MockAudioDevice::Sample const &) for more details */
  std::unique_ptr<MockAudioDevice::Sample> processSample(resource::File const &iSampleFile,
                                                         std::optional<Duration> iTail = std::nullopt,
                                                         std::optional<tester::Timeline> iTimeline = std::nullopt) {
    return processSample(*loadSample(iSampleFile), iTail, std::move(iTimeline));
  }

  /**
   * Processes a built-in sample (ex: "/Private/sample.wav")
   *
   * @see processSample(MockAudioDevice::Sample const &) for more details */
  std::unique_ptr<MockAudioDevice::Sample> processSample(std::string const &iSampleResource,
                                                         std::optional<Duration> iTail = std::nullopt,
                                                         std::optional<tester::Timeline> iTimeline = std::nullopt) {
    return processSample(*loadSample(iSampleResource), iTail, std::move(iTimeline));
  }

  //! Convenient api to get the bypass state of the effect device under test (property `/custom_properties/builtin_onoffbypass`)
  TJBox_OnOffBypassStates getBypassState() const { return fDevice.getEffectBypassState(); }

  //! Convenient api to set the bypass state of the effect device under test (property `/custom_properties/builtin_onoffbypass`)
  void setBypassState(TJBox_OnOffBypassStates iState) { fDevice.setEffectBypassState(iState); }

  //! Return the mock audio source created by this helper
  inline rack::ExtensionDevice<MAUSrc> &src() { return fSrc; }

  //! Return the mock audio source created by this helper
  inline rack::ExtensionDevice<MAUSrc> const &src() const { return fSrc; }

  //! Return the mock audio destination created by this helper
  inline rack::ExtensionDevice<MAUDst> &dst() { return fDst; }

  //! Return the mock audio destination created by this helper
  inline rack::ExtensionDevice<MAUDst> const &dst() const { return fDst; }

protected:
  rack::ExtensionDevice<MAUSrc> fSrc;
  rack::ExtensionDevice<MAUDst> fDst;
};

/**
 * Tester class to test a (studio) effect (`device_type="studio_fx"` in `info.lua`).
 *
 * @see ExtensionEffectTester for common methods */
template<typename Effect>
class StudioEffectTester : public ExtensionEffectTester
{
public:
  explicit StudioEffectTester(DeviceConfig<Effect> const &iDeviceConfig, int iSampleRate = 44100) :
    ExtensionEffectTester(iDeviceConfig.getConfig(), iSampleRate),
    fEffect{getExtensionDevice<Effect>()}
  {
    RE_MOCK_ASSERT(iDeviceConfig.getConfig().info().fDeviceType == DeviceType::kStudioFX);
  }

  //! Accesses the device under test (typed api)
  inline rack::ExtensionDevice<Effect> &device() { return fEffect; }

  //! Accesses the device under test (typed api)
  inline rack::ExtensionDevice<Effect> const &device() const { return fEffect; }

protected:
  rack::ExtensionDevice<Effect> fEffect;
};

/**
 * Tester class to test a (creative) effect (`device_type="creative_fx"` in `info.lua`).
 *
 * @see ExtensionEffectTester for common methods */
template<typename Effect>
class CreativeEffectTester : public ExtensionEffectTester
{
public:
  explicit CreativeEffectTester(DeviceConfig<Effect> const &iDeviceConfig, int iSampleRate = 44100) :
    ExtensionEffectTester(iDeviceConfig.getConfig(), iSampleRate),
    fEffect{getExtensionDevice<Effect>()}
  {
    RE_MOCK_ASSERT(iDeviceConfig.getConfig().info().fDeviceType == DeviceType::kCreativeFX);
  }

  //! Accesses the device under test (typed api)
  inline rack::ExtensionDevice<Effect> &device() { return fEffect; }

  //! Accesses the device under test (typed api)
  inline rack::ExtensionDevice<Effect> const &device() const { return fEffect; }

protected:
  rack::ExtensionDevice<Effect> fEffect;
};

/**
 * Base tester class to test an instrument (`device_type="instrument"` in `info.lua`). Contains the non templated
 * methods.
 *
 * Since an instrument is designed to produce audio, this tester automatically creates a mock audio
 * destination (accessible via  `ExtensionInstrumentTester::dst()`)
 *
 * @note In order to access the typed device, the `InstrumentTester<Instrument>` class should be used */
class ExtensionInstrumentTester : public DeviceTester
{
public:
  explicit ExtensionInstrumentTester(Config const &iDeviceConfig, int iSampleRate);

  //! Wire the main device output sockets to the mock audio destination (optional used to wire only one of them if necessary)
  void wireMainOut(std::optional<std::string> iLeftOutSocketName, std::optional<std::string> iRightOutSocketName);

  //! @see DeviceTester::nextBatch()
  using DeviceTester::nextBatch;

  /**
   * Convenient version of `nextBatch()` which injects the note events in the device under test, runs 1 batch and
   * returns the audio generated in the destination (one buffer of 64 samples) */
  MockAudioDevice::StereoBuffer nextBatch(MockDevice::NoteEvents iNoteEvents);

  /**
   * Equivalent to "Bounce" in Reason/Recon. Play the timeline and collect the output into the sample returned.
   *
   * @note If the transport should not be playing, then simply disable it (`DeviceTester::disableTransport()`) prior
   *       to calling this method. */
  std::unique_ptr<MockAudioDevice::Sample> bounce(tester::Timeline iTimeline);

  /**
   * Equivalent to "Bounce" in Reason/Recon. Play for the duration and collect the output into the sample returned.
   *
   * @param iDuration the duration is converted into a number of batches (for example 1 second would be
   *                  converted to 750 batches with a sample rate of 48000)
   * @param iTimeline an optional timeline that can be executed while the batches are processed
   *
   * @note If the transport should not be playing, then simply disable it (`DeviceTester::disableTransport()`) prior
   *       to calling this method. */
  std::unique_ptr<MockAudioDevice::Sample> bounce(Duration iDuration, std::optional<tester::Timeline> iTimeline = std::nullopt);

  //! Return the mock audio destination created by this helper
  inline rack::ExtensionDevice<MAUDst> &dst() { return fDst; }

  //! Return the mock audio destination created by this helper
  inline rack::ExtensionDevice<MAUDst> const &dst() const { return fDst; }

protected:
  rack::ExtensionDevice<MAUDst> fDst;
};

/**
 * Tester class to test an instrument (`device_type="instrument"` in `info.lua`). */
template<typename Instrument>
class InstrumentTester : public ExtensionInstrumentTester
{
public:
  explicit InstrumentTester(DeviceConfig<Instrument> const &iDeviceConfig, int iSampleRate = 44100) :
    ExtensionInstrumentTester(iDeviceConfig.getConfig(), iSampleRate),
    fInstrument{getExtensionDevice<Instrument>()}
  {}

  //! Accesses the device under test (typed api)
  inline rack::ExtensionDevice<Instrument> &device() { return fInstrument; }

  //! Accesses the device under test (typed api)
  inline rack::ExtensionDevice<Instrument> const &device() const { return fInstrument; }

protected:
  rack::ExtensionDevice<Instrument> fInstrument;
};

/**
 * Base tester class to test a note player (`device_type="note_player"` in `info.lua`). Contains the non templated
 * methods.
 *
 * Since a note player is designed to produce note events, this tester automatically creates a mock note player
 * destination (accessible via  `ExtensionNotePlayerTester::dst()`). Since note players can be chained and receive
 * notes as well, it also creates a mock note player source (accessible via `ExtensionNotePlayerTester::src()`)
 *
 * @note In order to access the typed device, the `NotePlayerTester<NotePlayer>` class should be used */
class ExtensionNotePlayerTester : public DeviceTester
{
public:
  explicit ExtensionNotePlayerTester(Config const &iDeviceConfig, int iSampleRate);

  //! @see DeviceTester::nextBatch()
  using DeviceTester::nextBatch;

  /**
   * Convenient version of `nextBatch()` which injects the note events in the device under test, runs 1 batch and
   * returns the note events produced by the note player. */
  MockDevice::NoteEvents nextBatch(MockDevice::NoteEvents iSourceEvents);

  //! Convenient api to get the bypass state of the note player under test (property `/environment/player_bypassed`)
  bool isBypassed() const { return fDevice.isNotePlayerBypassed(); }

  //! Convenient api to set the bypass state of the note player under test (property `/environment/player_bypassed`)
  void setBypassed(bool iBypassed) { fDevice.setNotePlayerBypassed(iBypassed); }

  //! Return the mock note player source created by this helper
  inline rack::ExtensionDevice<MNPSrc> &src() { return fSrc; }

  //! Return the mock note player source created by this helper
  inline rack::ExtensionDevice<MNPSrc> const &src() const { return fSrc; }

  //! Return the mock note player destination created by this helper
  inline rack::ExtensionDevice<MNPDst> &dst() { return fDst; }

  //! Return the mock note player destination created by this helper
  inline rack::ExtensionDevice<MNPDst> const &dst() const { return fDst; }

protected:
  rack::ExtensionDevice<MNPSrc> fSrc;
  rack::ExtensionDevice<MNPDst> fDst;
};

/**
 * Tester class to test a note player (`device_type="note_player"` in `info.lua`). */
template<typename NotePlayer>
class NotePlayerTester : public ExtensionNotePlayerTester
{
public:
  explicit NotePlayerTester(DeviceConfig<NotePlayer> const &iDeviceConfig, int iSampleRate = 44100) :
    ExtensionNotePlayerTester(iDeviceConfig.getConfig(), iSampleRate),
    fNotePlayer{getExtensionDevice<NotePlayer>()}
  {}

  //! Accesses the device under test (typed api)
  inline rack::ExtensionDevice<NotePlayer> &device() { return fNotePlayer; }

  //! Accesses the device under test (typed api)
  inline rack::ExtensionDevice<NotePlayer> const &device() const { return fNotePlayer; }

protected:
  rack::ExtensionDevice<NotePlayer> fNotePlayer;
};

//------------------------------------------------------------------------
// DeviceTester::getExtensionDevice
//------------------------------------------------------------------------
template<typename Device>
rack::ExtensionDevice<Device> DeviceTester::getExtensionDevice()
{
  return fRack.getDevice<Device>(fDevice.getInstanceId());
}

}

#endif //__Pongasoft_re_mock_device_testers_h__