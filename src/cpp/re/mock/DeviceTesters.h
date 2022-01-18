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

class Timeline
{
public:
  using SimpleEvent = std::function<void()>;
  using Event = std::function<bool(long iAtBatch)>;

  static const Event kNoOp;
  static const Event kEnd;

  static Event wrap(SimpleEvent iEvent);

public:
  Timeline &after(Duration iDuration);

  Timeline &after1Batch() { return after(rack::Duration{1}); }

  Timeline &event(Event iEvent) { return event(fCurrentBath, std::move(iEvent)); }
  Timeline &event(SimpleEvent iEvent) { return event(fCurrentBath, std::move(iEvent)); }

  Timeline &onEveryBatch(Event iEvent);
  Timeline &onEveryBatch(SimpleEvent iEvent) { return onEveryBatch(wrap(iEvent)); }

  Timeline &transportStart();
  Timeline &transportStop();

  Timeline &end() { return event(kEnd); }

  Timeline &notes(MockDevice::NoteEvents iNoteEvents);
  Timeline &note(TJBox_UInt8 iNoteNumber, Duration iDuration, TJBox_UInt8 iNoteVelocity = 100);

  void execute(bool iWithTransport, std::optional<Duration> iDuration = std::nullopt) const;
  inline void execute(std::optional<Duration> iDuration = std::nullopt) const { execute(false, iDuration); }
  inline void play(std::optional<Duration> iDuration = std::nullopt) const { execute(true, iDuration); }

  friend class re::mock::DeviceTester;

private:
  Timeline(DeviceTester *iTester) : fTester{iTester} {}

  Timeline &event(size_t iAtBatch, Event iEvent);
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
  explicit DeviceTester(Config const &iDeviceConfig, int iSampleRate);

  inline Rack &rack() { return fRack; }
  inline Rack const &rack() const { return fRack; }

  rack::ExtensionDevice<MAUSrc> &wire(rack::ExtensionDevice<MAUSrc> &iSrc,
                                      std::optional<std::string> iLeftInSocketName = std::nullopt,
                                      std::optional<std::string> iRightInSocketName = std::nullopt);

  rack::ExtensionDevice<MAUSrc> wireNewAudioSrc(std::optional<std::string> iLeftInSocketName = std::nullopt,
                                                std::optional<std::string> iRightInSocketName = std::nullopt);

  void unwire(rack::ExtensionDevice<MAUSrc> &iSrc);

  rack::ExtensionDevice<MAUDst> &wire(rack::ExtensionDevice<MAUDst> &iDst,
                                      std::optional<std::string> iLeftOutSocketName = std::nullopt,
                                      std::optional<std::string> iRightOutSocketName = std::nullopt);

  rack::ExtensionDevice<MAUDst> wireNewAudioDst(std::optional<std::string> iLeftOutSocketName = std::nullopt,
                                                std::optional<std::string> iRightOutSocketName = std::nullopt);

  void unwire(rack::ExtensionDevice<MAUDst> &iDst);

  rack::ExtensionDevice<MCVSrc> &wire(rack::ExtensionDevice<MCVSrc> &iSrc, std::string const &iCVInSocketName);

  rack::ExtensionDevice<MCVSrc> wireNewCVSrc(std::optional<std::string> iCVInSocketName = std::nullopt);

  void unwire(rack::ExtensionDevice<MCVSrc> &iSrc);

  rack::ExtensionDevice<MCVDst> &wire(rack::ExtensionDevice<MCVDst> &iDst, std::string const &iCVOutSocketName);

  rack::ExtensionDevice<MCVDst> wireNewCVDst(std::optional<std::string> iCVOutSocketName = std::nullopt);

  void unwire(rack::ExtensionDevice<MCVDst> &iDst);

  rack::ExtensionDevice<MNPSrc> wireNewNotePlayerSrc();

  void unwire(rack::ExtensionDevice<MNPSrc> &iSrc);

  rack::ExtensionDevice<MNPDst> wireNewNotePlayerDst();

  void unwire(rack::ExtensionDevice<MNPDst> &iDst);

  int getSampleRate() const { return fRack.getSampleRate(); }

  inline void enableTransport() { fTransportEnabled = true; }
  inline void disableTransport() { transportStop(); fTransportEnabled = false; }

  void transportStart();
  void transportStop();
  inline bool transportPlaying() const { return fRack.getTransportPlaying(); }
  inline void transportReset() { fRack.setTransportPlayPos(0); }

  void requestResetAudio() { fRack.requestResetAudio(); }

  sequencer::Time transportPlayPos() const;
  void transportPlayPos(sequencer::Time iTime) { fRack.setTransportPlayPos(iTime); }

  TJBox_Float64 transportTempo() const { return fRack.getTransportTempo(); }
  void transportTempo(TJBox_Float64 iTempo) { fRack.setTransportTempo(iTempo); }

  sequencer::TimeSignature transportTimeSignature() const { return fRack.getTransportTimeSignature(); }
  void transportTimeSignature(sequencer::TimeSignature iTimeSignature) { fRack.setTransportTimeSignature(iTimeSignature); }

  bool transportLoopEnabled() const { return fRack.getTransportLoopEnabled(); }

  void transportEnableLoop() { fRack.setTransportLoopEnabled(true); }
  void transportDisableLoop() { fRack.setTransportLoopEnabled(false); }

  void transportLoop(sequencer::Time iStart, sequencer::Time iEnd) { fRack.setTransportLoop(iStart, iEnd); }
  void transportLoop(sequencer::Time iStart, sequencer::Duration iDuration) { fRack.setTransportLoop(iStart, iDuration); }

  sequencer::Time transportSongEnd() const { return fRack.getSongEnd(); }
  void transportSongEnd(sequencer::Time iSongEnd) { fRack.setSongEnd(iSongEnd); }

  void setNoteEvents(MockDevice::NoteEvents iNoteEvents) { fDevice.setNoteInEvents(iNoteEvents.events()); }

  sequencer::Track &sequencerTrack() const { return fDevice.getSequencerTrack(); }

  tester::Timeline newTimeline() { return tester::Timeline(this); }

  inline void nextBatch() { fRack.nextBatch(); }

  void nextBatches(Duration iDuration, std::optional<tester::Timeline> iTimeline = std::nullopt);
  inline void nextBatches(tester::Timeline iTimeline) { iTimeline.execute(); }

  void play(Duration iDuration, std::optional<tester::Timeline> iTimeline = std::nullopt);
  inline void play(tester::Timeline iTimeline) { iTimeline.play(); }

  std::unique_ptr<MockAudioDevice::Sample> loadSample(resource::File const &iSampleFile) const;
  std::unique_ptr<MockAudioDevice::Sample> loadSample(std::string const &iSampleResource) const;
  void saveSample(MockAudioDevice::Sample const &iSample, resource::File const &iToFile) const;

  void deviceReset() { fDevice.reset(); }

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
 * Since utilities can be pretty much anything, this tester does not provide any kind of default devices. Use the
 * HelperTester::rack() method to access the rack and add/wire any other devices necessary for testing the helper.
 */
template<typename Helper>
class HelperTester : public DeviceTester
{
public:
  explicit HelperTester(DeviceConfig<Helper> const &iDeviceConfig, int iSampleRate = 44100) :
    DeviceTester(iDeviceConfig.getConfig(), iSampleRate),
    fHelper{getExtensionDevice<Helper>()}
    {
      RE_MOCK_ASSERT(iDeviceConfig.getConfig().info().fDeviceType == DeviceType::kHelper);
    }

  inline rack::ExtensionDevice<Helper> &device() { return fHelper; }
  inline rack::ExtensionDevice<Helper> const &device() const { return fHelper; }

protected:
  rack::ExtensionDevice<Helper> fHelper;
};

/**
 * Base tester class to test an effect (`device_type="creative_fx"` or `device_type="studio_fx"` in `info.lua`).
 * You should instantiate the proper subclass: StudioEffectTester or CreativeEffectTester
 */
class ExtensionEffectTester : public DeviceTester
{
public:
  using optional_duration_t = std::optional<Duration>;

public:
  explicit ExtensionEffectTester(Config const &iDeviceConfig, int iSampleRate);

  void wireMainIn(std::optional<std::string> iLeftInSocketName, std::optional<std::string> iRightInSocketName);
  void wireMainOut(std::optional<std::string> iLeftOutSocketName, std::optional<std::string> iRightOutSocketName);

  using DeviceTester::nextBatch;
  MockAudioDevice::StereoBuffer nextBatch(MockAudioDevice::StereoBuffer const &iInputBuffer);
  void nextBatch(MockAudioDevice::StereoBuffer const &iInputBuffer, MockAudioDevice::StereoBuffer &oOutputBuffer);

  std::unique_ptr<MockAudioDevice::Sample> processSample(MockAudioDevice::Sample const &iSample,
                                                         std::optional<Duration> iTail = std::nullopt,
                                                         std::optional<tester::Timeline> iTimeline = std::nullopt);

  std::unique_ptr<MockAudioDevice::Sample> processSample(resource::File const &iSampleFile,
                                                         std::optional<Duration> iTail = std::nullopt,
                                                         std::optional<tester::Timeline> iTimeline = std::nullopt) {
    return processSample(*loadSample(iSampleFile), iTail, std::move(iTimeline));
  }

  std::unique_ptr<MockAudioDevice::Sample> processSample(std::string const &iSampleResource,
                                                         optional_duration_t iTail = std::nullopt,
                                                         std::optional<tester::Timeline> iTimeline = std::nullopt) {
    return processSample(*loadSample(iSampleResource), iTail, std::move(iTimeline));
  }

  TJBox_OnOffBypassStates getBypassState() const { return fDevice.getEffectBypassState(); }
  void setBypassState(TJBox_OnOffBypassStates iState) { fDevice.setEffectBypassState(iState); }

  inline rack::ExtensionDevice<MAUSrc> &src() { return fSrc; }
  inline rack::ExtensionDevice<MAUSrc> const &src() const { return fSrc; }

  inline rack::ExtensionDevice<MAUDst> &dst() { return fDst; }
  inline rack::ExtensionDevice<MAUDst> const &dst() const { return fDst; }

protected:
  rack::ExtensionDevice<MAUSrc> fSrc;
  rack::ExtensionDevice<MAUDst> fDst;
};

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

  inline rack::ExtensionDevice<Effect> &device() { return fEffect; }
  inline rack::ExtensionDevice<Effect> const &device() const { return fEffect; }

protected:
  rack::ExtensionDevice<Effect> fEffect;
};

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

  inline rack::ExtensionDevice<Effect> &device() { return fEffect; }
  inline rack::ExtensionDevice<Effect> const &device() const { return fEffect; }

protected:
  rack::ExtensionDevice<Effect> fEffect;
};

class ExtensionInstrumentTester : public DeviceTester
{
public:
  explicit ExtensionInstrumentTester(Config const &iDeviceConfig, int iSampleRate);

  void wireMainOut(std::optional<std::string> iLeftOutSocketName, std::optional<std::string> iRightOutSocketName);

  using DeviceTester::nextBatch;
  MockAudioDevice::StereoBuffer nextBatch(MockDevice::NoteEvents iNoteEvents);
  std::unique_ptr<MockAudioDevice::Sample> bounce(tester::Timeline iTimeline);
  std::unique_ptr<MockAudioDevice::Sample> bounce(Duration iDuration, std::optional<tester::Timeline> iTimeline = std::nullopt);

  inline rack::ExtensionDevice<MAUDst> &dst() { return fDst; }
  inline rack::ExtensionDevice<MAUDst> const &dst() const { return fDst; }

protected:
  rack::ExtensionDevice<MAUDst> fDst;
};

template<typename Instrument>
class InstrumentTester : public ExtensionInstrumentTester
{
public:
  explicit InstrumentTester(DeviceConfig<Instrument> const &iDeviceConfig, int iSampleRate = 44100) :
    ExtensionInstrumentTester(iDeviceConfig.getConfig(), iSampleRate),
    fInstrument{getExtensionDevice<Instrument>()}
  {}

  inline rack::ExtensionDevice<Instrument> &device() { return fInstrument; }
  inline rack::ExtensionDevice<Instrument> const &device() const { return fInstrument; }

protected:
  rack::ExtensionDevice<Instrument> fInstrument;
};

class ExtensionNotePlayerTester : public DeviceTester
{
public:
  explicit ExtensionNotePlayerTester(Config const &iDeviceConfig, int iSampleRate);

  using DeviceTester::nextBatch;
  MockDevice::NoteEvents nextBatch(MockDevice::NoteEvents iSourceEvents);

  bool isBypassed() const { return fDevice.isNotePlayerBypassed(); }
  void setBypassed(bool iBypassed) { fDevice.setNotePlayerBypassed(iBypassed); }

  inline rack::ExtensionDevice<MNPSrc> &src() { return fSrc; }
  inline rack::ExtensionDevice<MNPSrc> const &src() const { return fSrc; }

  inline rack::ExtensionDevice<MNPDst> &dst() { return fDst; }
  inline rack::ExtensionDevice<MNPDst> const &dst() const { return fDst; }

protected:
  rack::ExtensionDevice<MNPSrc> fSrc;
  rack::ExtensionDevice<MNPDst> fDst;
};

template<typename NotePlayer>
class NotePlayerTester : public ExtensionNotePlayerTester
{
public:
  explicit NotePlayerTester(DeviceConfig<NotePlayer> const &iDeviceConfig, int iSampleRate = 44100) :
    ExtensionNotePlayerTester(iDeviceConfig.getConfig(), iSampleRate),
    fNotePlayer{getExtensionDevice<NotePlayer>()}
  {}

  inline rack::ExtensionDevice<NotePlayer> &device() { return fNotePlayer; }
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