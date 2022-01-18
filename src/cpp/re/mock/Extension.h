/*
 * Copyright (c) 2022 pongasoft
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

#ifndef RE_MOCK_EXTENSION_H
#define RE_MOCK_EXTENSION_H

#include <JukeboxTypes.h>
#include <functional>
#include "Motherboard.h"
#include "Sequencer.h"
#include <MidiEventList.h>

namespace re::mock::impl { class ExtensionImpl; }

namespace re::mock::rack {

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
  void use(std::function<void (Motherboard &)> iCallback) {
    impl::InternalThreadLocalRAII raii{&motherboard()};
    iCallback(motherboard());
  }

  inline void use(std::function<void ()> iCallback) {
    use([callback = std::move(iCallback)](auto &motherboard) { callback(); });
  }

  template<typename R>
  R use(std::function<R (Motherboard &)> iCallback)
  {
    impl::InternalThreadLocalRAII raii{&motherboard()};
    return iCallback(motherboard());
  }

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
  sequencer::Track &getSequencerTrack() const;
  void loadMidiNotes(smf::MidiEventList const &iEvents);

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
  inline void loadPatch(resource::String const &iPatchString) { motherboard().loadPatch(iPatchString); }
  inline void loadPatch(resource::File const &iPatchFile) { motherboard().loadPatch(iPatchFile); }
  inline void loadPatch(resource::Patch const &iPatch) { motherboard().loadPatch(iPatch); }

  inline resource::Patch generatePatch() const { return motherboard().generatePatch(); }
  inline resource::Patch const &getDefaultValuesPatch() const { return motherboard().getDefaultValuesPatch(); }
  inline void reset() { motherboard().reset(); }

  inline bool loadMoreBlob(std::string const &iPropertyPath, long iCount = -1) { return motherboard().loadMoreBlob(iPropertyPath, iCount); }

  inline void selectCurrentUserSample(int iUserSampleIndex) { motherboard().selectCurrentUserSample(iUserSampleIndex); }

  inline void loadUserSampleAsync(std::string const &iPropertyPath,
                                  std::string const &iResourcePath,
                                  std::optional<resource::LoadingContext> iCtx = std::nullopt) {
    motherboard().loadUserSampleAsync(iPropertyPath, iResourcePath, iCtx);
  }

  inline void loadUserSampleAsync(int iUserSampleIndex,
                                  std::string const &iResourcePath,
                                  std::optional<resource::LoadingContext> iCtx = std::nullopt) {
    motherboard().loadUserSampleAsync(iUserSampleIndex, iResourcePath, iCtx);
  }

  inline void loadCurrentUserSampleAsync(std::string const &iResourcePath,
                                         std::optional<resource::LoadingContext> iCtx = std::nullopt) {
    motherboard().loadCurrentUserSampleAsync(iResourcePath, iCtx);
  }

  inline void deleteUserSample(std::string const &iPropertyPath) { motherboard().deleteUserSample(iPropertyPath); }

  inline void deleteUserSample(int iUserSampleIndex) { motherboard().deleteUserSample(iUserSampleIndex); }

  inline void deleteCurrentUserSample() { motherboard().deleteCurrentUserSample(); }

  inline bool loadMoreSample(std::string const &iPropertyPath, long iFrameCount = -1) { return motherboard().loadMoreSample(iPropertyPath, iFrameCount); }

  inline void selectCurrentPattern(int iPatternIndex, int iPatternStartPos) { motherboard().selectCurrentPattern(iPatternIndex, iPatternStartPos); }

  inline void setResourceLoadingContext(std::string const &iResourcePath, resource::LoadingContext const &iCtx) { motherboard().setResourceLoadingContext(iResourcePath, iCtx); }
  inline void clearResourceLoadingContext(std::string const &iResourcePath) { motherboard().clearResourceLoadingContext(iResourcePath); }

  template<typename T>
  inline T* getInstance() const { return motherboard().getInstance<T>(); }

  int getInstanceId() const;

  friend class re::mock::Rack;

protected:
  Motherboard &motherboard() const;

protected:
  explicit Extension(std::shared_ptr<impl::ExtensionImpl> iExtensionImpl) : fImpl{iExtensionImpl} {}

private:
  std::shared_ptr<impl::ExtensionImpl> fImpl;
};

template<typename Device>
class ExtensionDevice : public Extension
{
public:
  Device *operator->() { return getInstance<Device>(); };
  Device const *operator->() const { return getInstance<Device>(); };

  friend class re::mock::Rack;

private:
  explicit ExtensionDevice(std::shared_ptr<impl::ExtensionImpl> iExtensionImpl) : Extension{iExtensionImpl} {}
};

bool operator==(Extension::AudioOutSocket const &lhs, Extension::AudioOutSocket const &rhs);
bool operator==(Extension::AudioInSocket const &lhs, Extension::AudioInSocket const &rhs);
bool operator==(Extension::CVOutSocket const &lhs, Extension::CVOutSocket const &rhs);
bool operator==(Extension::CVInSocket const &lhs, Extension::CVInSocket const &rhs);

}

#endif //RE_MOCK_EXTENSION_H
