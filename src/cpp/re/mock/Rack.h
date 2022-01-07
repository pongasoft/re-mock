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

namespace re::mock {

namespace time {
struct Duration { float fMilliseconds{}; };
}

namespace sample {
struct Duration { long fFrames{}; };
}

namespace rack {
struct Duration { long fBatches{}; };
}

using Duration = std::variant<time::Duration, sample::Duration, rack::Duration, sequencer::Duration>;

class Rack
{
protected:
  class ExtensionImpl;

public:
  class Extension
  {
  public:
    struct Socket {
      int fExtensionId{};
      TJBox_ObjectRef fSocketRef{};
    };

    struct AudioSocket : public Socket {};
    struct AudioOutSocket : public AudioSocket {};
    struct AudioInSocket : public AudioSocket {};
    struct StereoAudioOutSocket { AudioOutSocket fLeft; AudioOutSocket fRight; };
    struct StereoAudioInSocket { AudioInSocket fLeft; AudioInSocket fRight; };

    struct CVSocket : public Socket {};
    struct CVOutSocket : public CVSocket {};
    struct CVInSocket : public CVSocket {};

    struct AudioWire {
      AudioOutSocket fFromSocket{};
      AudioInSocket fToSocket{};

      static bool overlap(AudioWire const &iWire1, AudioWire const &iWire2);
    };

    struct CVWire {
      CVOutSocket fFromSocket{};
      CVInSocket fToSocket{};

      static bool overlap(CVWire const &iWire1, CVWire const &iWire2);
    };

    struct NoteSocket { int fExtensionId{}; };
    struct NoteOutSocket : public NoteSocket{};
    struct NoteInSocket : public NoteSocket{};

    struct NoteWire{
      NoteOutSocket fFromSocket{};
      NoteInSocket fToSocket{};
    };

  public:
    inline void use(std::function<void (Motherboard &)> iCallback) { fImpl->use(std::move(iCallback)); }
    inline void use(std::function<void ()> iCallback) {
      use([callback = std::move(iCallback)](auto &motherboard) { callback(); });
    }
    template<typename R>
    inline R use(std::function<R (Motherboard &)> iCallback) { return fImpl->use<R>(std::move(iCallback)); }
    template<typename R>
    inline R use(std::function<R ()> iCallback) {
      return use<R>([callback = std::move(iCallback)](auto &motherboard) -> R { return callback(); });
    }

    AudioOutSocket getAudioOutSocket(std::string const &iSocketName) const;
    StereoAudioOutSocket getStereoAudioOutSocket(std::string const &iLeftSocketName, std::string const &iRightSocketName) const;
    AudioInSocket getAudioInSocket(std::string const &iSocketName) const;
    StereoAudioInSocket getStereoAudioInSocket(std::string const &iLeftSocketName, std::string const &iRightSocketName) const;
    CVOutSocket getCVOutSocket(std::string const &iSocketName) const;
    CVInSocket getCVInSocket(std::string const &iSocketName) const;
    NoteOutSocket getNoteOutSocket() const;
    NoteInSocket getNoteInSocket() const;
    sequencer::Track &getSequencerTrack() const { return fImpl->fSequencerTrack; }

    inline TJBox_Value getValue(std::string const &iPropertyPath) const { return motherboard().getValue(iPropertyPath); }
    inline void setValue(std::string const &iPropertyPath, TJBox_Value const &iValue) { motherboard().setValue(iPropertyPath, iValue); }
    inline bool getBool(std::string const &iPropertyPath) const { return motherboard().getBool(iPropertyPath); }
    inline void setBool(std::string const &iPropertyPath, bool iValue) { motherboard().setBool(iPropertyPath, iValue); }

    inline std::string toString(TJBox_Value const &iValue, char const *iFormat = nullptr) { return motherboard().toString(iValue, iFormat); }
    inline std::string toString(std::string const &iPropertyPath, char const *iFormat = nullptr) { return motherboard().toString(iPropertyPath, iFormat); }
    inline std::string toString(TJBox_PropertyRef const &iPropertyRef) const { return motherboard().toString(iPropertyRef); }
    inline std::string getObjectPath(TJBox_ObjectRef iObjectRef) const { return motherboard().getObjectPath(iObjectRef); }

    template<typename T = TJBox_Float64>
    inline T getNum(std::string const &iPropertyPath) const { return motherboard().getNum<T>(iPropertyPath); }
    template<typename T = TJBox_Float64>
    inline void setNum(std::string const &iPropertyPath, T iValue) { motherboard().setNum<T>(iPropertyPath, iValue);}

    inline std::string getString(std::string const &iPropertyPath) const { return motherboard().getString(iPropertyPath); }
    inline void setString(std::string const &iPropertyPath, std::string iValue) { motherboard().setString(iPropertyPath, std::move(iValue)); }

    inline std::string getRTString(std::string const &iPropertyPath) const { return motherboard().getRTString(iPropertyPath); }
    inline void setRTString(std::string const &iPropertyPath, std::string const &iValue) { motherboard().setRTString(iPropertyPath, iValue); }

    inline TJBox_OnOffBypassStates getEffectBypassState() const { return motherboard().getEffectBypassState(); }
    inline void setEffectBypassState(TJBox_OnOffBypassStates iState) { motherboard().setEffectBypassState(iState); }

    inline bool isNotePlayerBypassed() const { return motherboard().isNotePlayerBypassed(); }
    inline void setNotePlayerBypassed(bool iBypassed) { motherboard().setNotePlayerBypassed(iBypassed); }

    inline void setNoteInEvent(TJBox_UInt8 iNoteNumber, TJBox_UInt8 iVelocity, TJBox_UInt16 iAtFrameIndex = 0) { motherboard().setNoteInEvent(iNoteNumber, iVelocity, iAtFrameIndex); }
    inline void setNoteInEvent(TJBox_NoteEvent const &iNoteEvent) { motherboard().setNoteInEvent(iNoteEvent); }
    inline void setNoteInEvents(Motherboard::NoteEvents const &iNoteEvents) { motherboard().setNoteInEvents(iNoteEvents); }

    inline TJBox_Float64 getCVSocketValue(std::string const &iSocketPath) const { return motherboard().getCVSocketValue(iSocketPath); }
    inline void setCVSocketValue(std::string const &iSocketPath, TJBox_Float64 iValue) { motherboard().setCVSocketValue(iSocketPath, iValue); }
    inline TJBox_Float64 getCVSocketValue(CVSocket const &iSocket) const { return motherboard().getCVSocketValue(iSocket.fSocketRef); }
    inline void setCVSocketValue(CVSocket const &iSocket, TJBox_Float64 iValue) const { motherboard().setCVSocketValue(iSocket.fSocketRef, iValue); }

    inline Motherboard::DSPBuffer getDSPBuffer(std::string const &iAudioSocketPath) const { return motherboard().getDSPBuffer(iAudioSocketPath); }
    inline void setDSPBuffer(std::string const &iAudioSocketPath, Motherboard::DSPBuffer iBuffer) { motherboard().setDSPBuffer(iAudioSocketPath, std::move(iBuffer)); }
    inline Motherboard::DSPBuffer getDSPBuffer(AudioSocket const &iSocket) const { return motherboard().getDSPBuffer(iSocket.fSocketRef); }
    inline void setDSPBuffer(AudioSocket const &iSocket, Motherboard::DSPBuffer iBuffer) { motherboard().setDSPBuffer(iSocket.fSocketRef, std::move(iBuffer)); }

    inline void requestResetAudio() { motherboard().requestResetAudio(); }
    inline void requestRun() { motherboard().requestRun(); }
    inline void requestStop() { motherboard().requestStop(); }

    template<typename T>
    inline T *getNativeObjectRW(std::string const &iPropertyPath) const { return motherboard().getNativeObjectRW<T>(iPropertyPath); }

    template<typename T>
    inline const T *getNativeObjectRO(std::string const &iPropertyPath) const { return motherboard().getNativeObjectRO<T>(iPropertyPath); }

    inline void loadPatch(std::string const &iPatchPath) { motherboard().loadPatch(iPatchPath); }
    inline void loadPatch(ConfigString const &iPatchString) { motherboard().loadPatch(iPatchString); }
    inline void loadPatch(ConfigFile const &iPatchFile) { motherboard().loadPatch(iPatchFile); }
    inline void loadPatch(Resource::Patch const &iPatch) { motherboard().loadPatch(iPatch); }

    inline Resource::Patch generatePatch() const { return motherboard().generatePatch(); }
    inline Resource::Patch const &getDefaultValuesPatch() const { return motherboard().getDefaultValuesPatch(); }
    inline void reset() { motherboard().reset(); }

    inline bool loadMoreBlob(std::string const &iPropertyPath, long iCount = -1) { return motherboard().loadMoreBlob(iPropertyPath, iCount); }

    inline void selectCurrentUserSample(int iUserSampleIndex) { motherboard().selectCurrentUserSample(iUserSampleIndex); }

    inline void loadUserSampleAsync(std::string const &iPropertyPath,
                                    std::string const &iResourcePath,
                                    std::optional<Resource::LoadingContext> iCtx = std::nullopt) {
      motherboard().loadUserSampleAsync(iPropertyPath, iResourcePath, iCtx);
    }

    inline void loadUserSampleAsync(int iUserSampleIndex,
                                    std::string const &iResourcePath,
                                    std::optional<Resource::LoadingContext> iCtx = std::nullopt) {
      motherboard().loadUserSampleAsync(iUserSampleIndex, iResourcePath, iCtx);
    }

    inline void loadCurrentUserSampleAsync(std::string const &iResourcePath,
                                           std::optional<Resource::LoadingContext> iCtx = std::nullopt) {
      motherboard().loadCurrentUserSampleAsync(iResourcePath, iCtx);
    }

    inline void deleteUserSample(std::string const &iPropertyPath) { motherboard().deleteUserSample(iPropertyPath); }

    inline void deleteUserSample(int iUserSampleIndex) { motherboard().deleteUserSample(iUserSampleIndex); }

    inline void deleteCurrentUserSample() { motherboard().deleteCurrentUserSample(); }

    inline bool loadMoreSample(std::string const &iPropertyPath, long iFrameCount = -1) { return motherboard().loadMoreSample(iPropertyPath, iFrameCount); }

    inline void selectCurrentPattern(int iPatternIndex, int iPatternStartPos) { motherboard().selectCurrentPattern(iPatternIndex, iPatternStartPos); }

    inline void setResourceLoadingContext(std::string const &iResourcePath, Resource::LoadingContext const &iCtx) { motherboard().setResourceLoadingContext(iResourcePath, iCtx); }
    inline void clearResourceLoadingContext(std::string const &iResourcePath) { motherboard().clearResourceLoadingContext(iResourcePath); }

    template<typename T>
    inline T* getInstance() const { return motherboard().getInstance<T>(); }

    inline int getInstanceId() const { return fImpl->fId; }

    friend class Rack;

  protected:
    Motherboard &motherboard() const { return *fImpl->fMotherboard.get(); }

  private:
    explicit Extension(std::shared_ptr<ExtensionImpl> iExtensionImpl) : fImpl{iExtensionImpl} {}

  private:
    std::shared_ptr<ExtensionImpl> fImpl;
  };

  template<typename Device>
  class ExtensionDevice : public Extension
  {
  public:
    Device *operator->() { return getInstance<Device>(); };
    Device const *operator->() const { return getInstance<Device>(); };

    friend class Rack;

  private:
    explicit ExtensionDevice(std::shared_ptr<ExtensionImpl> iExtensionImpl) : Extension{iExtensionImpl} {}
  };

private:
  struct InternalThreadLocalRAII
  {
    explicit InternalThreadLocalRAII(Motherboard *iMotherboard);
    InternalThreadLocalRAII(InternalThreadLocalRAII &&) = delete;
    InternalThreadLocalRAII(InternalThreadLocalRAII const &) = delete;
    ~InternalThreadLocalRAII();
  private:
    Motherboard *fPrevious;
  };

protected:
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

    friend class Rack;
    friend class Rack::Extension;

  private:
    ExtensionImpl(int id, Rack *iRack, std::unique_ptr<Motherboard> iMotherboard) :
      fId{id}, fRack{iRack}, fMotherboard{std::move(iMotherboard)}, fSequencerTrack{iRack->getTransportTimeSignature()} {};

    void wire(Extension::AudioOutSocket const &iOutSocket, Extension::AudioInSocket const &iInSocket);
    void wire(Extension::CVOutSocket const &iOutSocket, Extension::CVInSocket const &iInSocket);
    void wire(Extension::NoteOutSocket const &iOutSocket, Extension::NoteInSocket const &iInSocket);
    std::optional<Extension::AudioInSocket> unwire(Extension::AudioOutSocket const &iOutSocket);
    std::optional<Extension::AudioOutSocket> unwire(Extension::AudioInSocket const &iInSocket);
    std::optional<Extension::AudioWire> findWire(Extension::AudioInSocket const &iInSocket);
    std::optional<Extension::CVInSocket> unwire(Extension::CVOutSocket const &iOutSocket);
    std::optional<Extension::CVOutSocket> unwire(Extension::CVInSocket const &iInSocket);
    std::optional<Extension::CVWire> findWire(Extension::CVInSocket const &iInSocket);
    std::optional<Extension::NoteInSocket> unwire(Extension::NoteOutSocket const &iOutSocket);
    std::optional<Extension::NoteOutSocket> unwire(Extension::NoteInSocket const &iInSocket);
    std::optional<Extension::NoteWire> findWire(Extension::NoteInSocket const &iInSocket);

    std::set<int> const &getDependents() const;

  private:
    int fId;
    [[maybe_unused]] Rack *fRack; // unused at this time...
    std::unique_ptr<Motherboard> fMotherboard;
    sequencer::Track fSequencerTrack;
    std::vector<Extension::AudioWire> fAudioOutWires{};
    std::vector<Extension::AudioWire> fAudioInWires{};
    std::vector<Extension::CVWire> fCVOutWires{};
    std::vector<Extension::CVWire> fCVInWires{};
    std::optional<Extension::NoteWire> fNoteOutWire{};
    std::optional<Extension::NoteWire> fNoteInWire{};
    mutable std::optional<std::set<int>> fDependents{};
  };

public:
  constexpr static auto kNumSamplesPerBatch = 64;

  Rack(int iSampleRate = 44100);

  Extension newExtension(Config const &iConfig);

  template<typename Device>
  ExtensionDevice<Device> newDevice(DeviceConfig<Device> const &iConfig);

  void wire(Extension::AudioOutSocket const &iOutSocket, Extension::AudioInSocket const &iInSocket);
  void unwire(Extension::AudioOutSocket const &iOutSocket);
  void unwire(Extension::AudioInSocket const &iInSocket);
  void wire(Extension::StereoAudioOutSocket const &iOutSocket, Extension::StereoAudioInSocket const &iInSocket);
  void unwire(Extension::StereoAudioOutSocket const &iOutSocket);
  void wire(Extension::CVOutSocket const &iOutSocket, Extension::CVInSocket const &iInSocket);
  void unwire(Extension::CVOutSocket const &iOutSocket);
  void unwire(Extension::CVInSocket const &iInSocket);
  void wire(Extension::NoteOutSocket const &iOutSocket, Extension::NoteInSocket const &iInSocket);
  void unwire(Extension::NoteOutSocket const &iOutSocket);
  void unwire(Extension::NoteInSocket const &iInSocket);

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
  void setTransportPlayPos(sequencer::Time iTime) { setTransportPlayPos(iTime.toPPQ(getTransportTimeSignature()).fCount); }
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
    setTransportLoopStartPos(iStart.toPPQ(getTransportTimeSignature()).fCount);
    setTransportLoopEndPos(iEnd.toPPQ(getTransportTimeSignature()).fCount);
  }

  void setTransportLoop(sequencer::Time iStart, sequencer::Duration iDuration) {
    setTransportLoop(iStart, iStart + iDuration);
  }

  TJBox_UInt64 getTransportBarStartPos() const { return fTransport.getBarStartPos(); }

  sequencer::Time getSongEnd() const { return fSongEnd; }
  void setSongEnd(sequencer::Time iSongEnd);

  static Motherboard &currentMotherboard();

  template<typename Device>
  ExtensionDevice<Device> getDevice(int iExtensionId);

protected:
  void copyAudioBuffers(Extension::AudioWire const &iWire);
  void copyCVValue(Extension::CVWire const &iWire);
  void copyNoteEvents(Extension::NoteWire const &iWire);
  void nextBatch(ExtensionImpl &iExtension);
  void nextBatch(ExtensionImpl &iExtension, std::set<int> &iProcessedExtensions);

protected:

protected:
  int fSampleRate;
  Transport fTransport;
  sequencer::Time fSongEnd{101,1,1,0}; // same default as Reason, not exported to device
  ObjectManager<std::shared_ptr<ExtensionImpl>> fExtensions{};
};

//------------------------------------------------------------------------
// Rack::newDevice
//------------------------------------------------------------------------
template<typename Device>
Rack::ExtensionDevice<Device> Rack::newDevice(DeviceConfig<Device> const &iConfig)
{
  return ExtensionDevice<Device>(newExtension(iConfig.getConfig()).fImpl);
}

//------------------------------------------------------------------------
// Rack::getDevice
//------------------------------------------------------------------------
template<typename Device>
Rack::ExtensionDevice<Device> Rack::getDevice(int iExtensionId)
{
  return Rack::ExtensionDevice<Device>{ fExtensions.get(iExtensionId) };
}

}

#endif //__Pongasoft_re_mock_rack_h__