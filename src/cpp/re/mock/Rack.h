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
#ifndef __Pongasoft_re_mock_rack_h__
#define __Pongasoft_re_mock_rack_h__

#include <map>
#include "Motherboard.h"
#include "ObjectManager.hpp"
#include "Transport.h"
#include "Sequencer.h"
#include <MidiEventList.h>
#include "Extension.h"

namespace re::mock {

namespace time {
/**
 * A `time::Duration` represents an amount of time expressed in milliseconds */
struct Duration { float fMilliseconds{}; };
}

namespace sample {
/**
 * A `sample::Duration` represents an amount of time expressed in number of audio frames/samples. */
struct Duration { TJBox_AudioFramePos fFrames{}; };
}

namespace rack {
/**
 * A `rack::Duration` represents an amount of time expressed in number of rack batches. A batch always represents a
 * fixed 64 audio frames/samples irrelevant of the sample rate. If the sample rate is higher more batches needs
 * to be processed in order to render more samples. */
struct Duration { size_t fBatches{}; };
}

namespace timeline {
/**
 * A `timeline::Duration` is a special kind of duration which indicates that it is driven by the
 * timeline itself (returns `false` when done).
 *
 * @note Be careful when using this duration as if the timeline never returns `false` then it would run indefinitely */
struct Duration {};
}

//! Most apis which takes a `Duration` use this variant which allows for all kinds of durations
using Duration = std::variant<time::Duration, sample::Duration, rack::Duration, sequencer::Duration, timeline::Duration>;

namespace impl {

class ExtensionImpl
{
public:
  void use(std::function<void (Motherboard &)> iCallback);

  template<typename R>
  R use(std::function<R (Motherboard &)> iCallback)
  {
    InternalThreadLocalRAII raii{fMotherboard.get()};
    return iCallback(*fMotherboard.get());
  }

  friend class re::mock::Rack;
  friend class rack::Extension;

private:
  ExtensionImpl(int id, Rack *iRack, std::unique_ptr<Motherboard> iMotherboard);

  void wire(rack::Extension::AudioOutSocket const &iOutSocket, rack::Extension::AudioInSocket const &iInSocket);
  void wire(rack::Extension::CVOutSocket const &iOutSocket, rack::Extension::CVInSocket const &iInSocket);
  void wire(rack::Extension::NoteOutSocket const &iOutSocket, rack::Extension::NoteInSocket const &iInSocket);
  std::optional<rack::Extension::AudioInSocket> unwire(rack::Extension::AudioOutSocket const &iOutSocket);
  std::optional<rack::Extension::AudioOutSocket> unwire(rack::Extension::AudioInSocket const &iInSocket);
  std::optional<rack::Extension::AudioWire> findWire(rack::Extension::AudioInSocket const &iInSocket);
  std::optional<rack::Extension::CVInSocket> unwire(rack::Extension::CVOutSocket const &iOutSocket);
  std::optional<rack::Extension::CVOutSocket> unwire(rack::Extension::CVInSocket const &iInSocket);
  std::optional<rack::Extension::CVWire> findWire(rack::Extension::CVInSocket const &iInSocket);
  std::optional<rack::Extension::NoteInSocket> unwire(rack::Extension::NoteOutSocket const &iOutSocket);
  std::optional<rack::Extension::NoteOutSocket> unwire(rack::Extension::NoteInSocket const &iInSocket);
  std::optional<rack::Extension::NoteWire> findWire(rack::Extension::NoteInSocket const &iInSocket);

  std::set<int> const &getDependents() const;

  void loadMidiNotes(smf::MidiEventList const &iEvents);

private:
  int fId;
  std::unique_ptr<Motherboard> fMotherboard;
  sequencer::Track fSequencerTrack;
  std::vector<rack::Extension::AudioWire> fAudioOutWires{};
  std::vector<rack::Extension::AudioWire> fAudioInWires{};
  std::vector<rack::Extension::CVWire> fCVOutWires{};
  std::vector<rack::Extension::CVWire> fCVInWires{};
  std::optional<rack::Extension::NoteWire> fNoteOutWire{};
  std::optional<rack::Extension::NoteWire> fNoteInWire{};
  mutable std::optional<std::set<int>> fDependents{};
};

}

class Rack
{
public:
  constexpr static auto kNumSamplesPerBatch = 64;

  Rack(int iSampleRate = 44100);
  ~Rack();

  rack::Extension newExtension(Config const &iConfig);

  template<typename Device>
  rack::ExtensionDevice<Device> newDevice(DeviceConfig<Device> const &iConfig);

  void wire(rack::Extension::AudioOutSocket const &iOutSocket, rack::Extension::AudioInSocket const &iInSocket);
  void unwire(rack::Extension::AudioOutSocket const &iOutSocket);
  void unwire(rack::Extension::AudioInSocket const &iInSocket);
  void wire(rack::Extension::StereoAudioOutSocket const &iOutSocket, rack::Extension::StereoAudioInSocket const &iInSocket);
  void unwire(rack::Extension::StereoAudioOutSocket const &iOutSocket);
  void wire(rack::Extension::CVOutSocket const &iOutSocket, rack::Extension::CVInSocket const &iInSocket);
  void unwire(rack::Extension::CVOutSocket const &iOutSocket);
  void unwire(rack::Extension::CVInSocket const &iInSocket);
  void wire(rack::Extension::NoteOutSocket const &iOutSocket, rack::Extension::NoteInSocket const &iInSocket);
  void unwire(rack::Extension::NoteOutSocket const &iOutSocket);
  void unwire(rack::Extension::NoteInSocket const &iInSocket);

  void nextBatch();

  int getSampleRate() const { return fSampleRate; }
  rack::Duration toRackDuration(Duration iDuration);
  sample::Duration toSampleDuration(Duration iDuration);

  void requestResetAudio();

  bool getTransportPlaying() const { return fTransport.getPlaying(); }
  void setTransportPlaying(bool iPlaying);

  void transportStart() { setTransportPlaying(true); }
  void transportStop() { setTransportPlaying(false); }

  TJBox_Int64 getTransportPlayPos() const { return fTransport.getPlayPos(); }
  void setTransportPlayPos(TJBox_Int64 iPos) { fTransport.setPlayPos(iPos); }
  void setTransportPlayPos(sequencer::Time iTime) { setTransportPlayPos(iTime.toPPQ().count()); }
  void resetTransportPlayPos() { setTransportPlayPos(0); }

  TJBox_Float64 getTransportTempo() const { return fTransport.getTempo(); }
  void setTransportTempo(TJBox_Float64 iTempo) { fTransport.setTempo(iTempo); }

  TJBox_Float64 getTransportFilteredTempo() const { return fTransport.getFilteredTempo(); }
  void setTransportFilteredTempo(TJBox_Float64 iFilteredTempo) { fTransport.setFilteredTempo(iFilteredTempo); }

  bool getTransportTempoAutomation() const { return fTransport.getTempoAutomation(); }
  void setTransportTempoAutomation(bool iTempoAutomation) { fTransport.setTempoAutomation(iTempoAutomation); }

  void setTransportTimeSignature(sequencer::TimeSignature iTimeSignature);

  sequencer::TimeSignature getTransportTimeSignature() const {
    return sequencer::TimeSignature(fTransport.getTimeSignatureNumerator(), fTransport.getTimeSignatureDenominator());
  }

  bool getTransportLoopEnabled() const { return fTransport.getLoopEnabled(); }
  void setTransportLoopEnabled(bool iLoopEnabled) { fTransport.setLoopEnabled(iLoopEnabled); }

  TJBox_UInt64 getTransportLoopStartPos() const { return fTransport.getLoopStartPos(); }
  void setTransportLoopStartPos(TJBox_UInt64 iLoopStartPos) { fTransport.setLoopStartPos(iLoopStartPos); }

  TJBox_UInt64 getTransportLoopEndPos() const { return fTransport.getLoopEndPos(); }
  void setTransportLoopEndPos(TJBox_UInt64 iLoopEndPos) { fTransport.setLoopEndPos(iLoopEndPos); }

  void setTransportLoop(sequencer::Time iStart, sequencer::Time iEnd) {
    setTransportLoopStartPos(iStart.toPPQ().count());
    setTransportLoopEndPos(iEnd.toPPQ().count());
  }

  void setTransportLoop(sequencer::Time iStart, sequencer::Duration iDuration) {
    setTransportLoop(iStart, iStart + iDuration);
  }

  TJBox_UInt64 getTransportBarStartPos() const { return fTransport.getBarStartPos(); }

  sequencer::Time getSongEnd() const { return fSongEnd; }
  void setSongEnd(sequencer::Time iSongEnd);

  inline size_t getBatchCount() const { return fBatchCount; }

  static Motherboard &currentMotherboard();

  template<typename Device>
  rack::ExtensionDevice<Device> getDevice(int iExtensionId);

protected:
  void copyAudioBuffers(rack::Extension::AudioWire const &iWire);
  void copyCVValue(rack::Extension::CVWire const &iWire);
  void copyNoteEvents(rack::Extension::NoteWire const &iWire);
  void nextBatch(impl::ExtensionImpl &iExtension);
  void nextBatch(impl::ExtensionImpl &iExtension, std::set<int> &iProcessedExtensions);

protected:

protected:
  int fSampleRate;
  Transport fTransport;
  size_t fBatchCount{};
  sequencer::Time fSongEnd{101,1,1,0}; // same default as Reason, not exported to device
  ObjectManager<std::shared_ptr<impl::ExtensionImpl>> fExtensions{};
};

//------------------------------------------------------------------------
// Rack::newDevice
//------------------------------------------------------------------------
template<typename Device>
rack::ExtensionDevice<Device> Rack::newDevice(DeviceConfig<Device> const &iConfig)
{
  return rack::ExtensionDevice<Device>(newExtension(iConfig.getConfig()).fImpl);
}

//------------------------------------------------------------------------
// Rack::getDevice
//------------------------------------------------------------------------
template<typename Device>
rack::ExtensionDevice<Device> Rack::getDevice(int iExtensionId)
{
  return rack::ExtensionDevice<Device>{ fExtensions.get(iExtensionId) };
}

}

#endif //__Pongasoft_re_mock_rack_h__