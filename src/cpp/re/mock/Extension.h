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

/**
 * This class represents the main api of the rack extension / device. Every tester gives access to an instance of this
 * class via the `tester.device()` api.
 *
 * @note This class is merely a facade to the lower level `Motherboard` in order to expose only the relevant parts
 *       of the api */
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
  /**
   * Low level api to "use" this extension while making the motherboard accessible during the callback thus
   * allowing to use the Jukebox API directly.
   *
   * For example:
   *
   * ```cpp
   * tester.device().use([](Motherboard &) {
   *  // All Jukebox API available
   *  auto ref = JBox_GetMotherboardObjectRef("/custom_properties");
   *  auto value = JBox_LoadMOMProperty(JBox_MakePropertyRef(ref, "my_prop"));
   *  auto v = JBox_GetNumber(value);
   *  // ...
   * });
   * ```
   *
   * @note this api is mostly used internally and should not really be needed in normal testing use cases since
   *       all Jukebox APIs have an equivalent on this class. For example the previous code can be achieved far more
   *       simply with `getNum("/custom_properties/my_prop")`
   */
  void withJukebox(std::function<void (Motherboard &)> iCallback) {
    impl::InternalThreadLocalRAII raii{&motherboard()};
    iCallback(motherboard());
  }

  //! Simpler api when the motherboard is not accessed
  inline void withJukebox(std::function<void ()> iCallback) {
    withJukebox([callback = std::move(iCallback)](auto &motherboard) { callback(); });
  }

  //! Returns the result of the callback
  template<typename R>
  R withJukebox(std::function<R (Motherboard &)> iCallback)
  {
    impl::InternalThreadLocalRAII raii{&motherboard()};
    return iCallback(motherboard());
  }

  //! Simpler api when the motherboard is not accessed
  template<typename R>
  inline R withJukebox(std::function<R ()> iCallback) {
    return withJukebox<R>([callback = std::move(iCallback)](auto &motherboard) -> R { return callback(); });
  }

  //! Returns the audio out socket given its name (not the full path)
  AudioOutSocket getAudioOutSocket(std::string const &iSocketName) const;

  //! Returns the stereo audio out socket given their names
  StereoAudioOutSocket getStereoAudioOutSocket(std::string const &iLeftSocketName, std::string const &iRightSocketName) const;

  //! Returns the audio in socket given its name (not the full path)
  AudioInSocket getAudioInSocket(std::string const &iSocketName) const;

  //! Returns the stereo audio in socket given their names
  StereoAudioInSocket getStereoAudioInSocket(std::string const &iLeftSocketName, std::string const &iRightSocketName) const;

  //! Returns the cv out socket given its name (not the full path)
  CVOutSocket getCVOutSocket(std::string const &iSocketName) const;

  //! Return the cv in socket given its name (not the full path)
  CVInSocket getCVInSocket(std::string const &iSocketName) const;

  /**
   * Return the note out socket.
   *
   * @note The concept of note socket is not a Reason/Jukebox concept (there is no equivalent property). But it is
   *       conceptually equivalent to a CV socket for note events with the difference that a device can only have
   *       one. For example when adding a note player in front of an instrument, there is an implicit "wire" between
   *       the note player "note out socket" and the instrument "note in socket". */
  NoteOutSocket getNoteOutSocket() const;

  /**
   * Return the note in socket.
   *
   * @see `getNoteOutSocket()` for an explanation about this kind of socket */
  NoteInSocket getNoteInSocket() const;

  /**
   * Return the sequencer track associated to this device. The sequencer track is the equivalent of the sequencer
   * track in Reason where it is possible to add notes and events (like property value changes). */
  sequencer::Track &getSequencerTrack() const;

  /**
   * Add the midi notes contained in the list of events (1 track of a midi file) to the sequencer track */
  void importMidiNotes(smf::MidiEventList const &iEvents);

  /**
   * Return the value of the property as the (opaque) Jukebox value
   * @param iPropertyPath the full path to the property (ex: `/custom_properties/my_prop`) */
  inline TJBox_Value getValue(std::string const &iPropertyPath) const { return motherboard().getValue(iPropertyPath); }

  /**
   * Sets the (opaque) Jukebox value to the property.
   *
   * @param iPropertyPath the full path to the property (ex: `/custom_properties/my_prop`)
   * @param iValue the (opaque) Jukebox value */
  inline void setValue(std::string const &iPropertyPath, TJBox_Value const &iValue) { motherboard().setValue(iPropertyPath, iValue); }

  //! Return the value of the property as a boolean
  inline bool getBool(std::string const &iPropertyPath) const { return motherboard().getBool(iPropertyPath); }

  //! Sets the value of the boolean property
  inline void setBool(std::string const &iPropertyPath, bool iValue) { motherboard().setBool(iPropertyPath, iValue); }

  /**
   * Return a string representation of the (opaque) Jukebox value
   *
   * @param iValue the (opaque) Jukebox value
   * @param iFormat can be used to tweak the outcome. For example if the value represents a number, by default, the
   *                format used is `"%f"`. If only 2 decimals is needed, the format `"%.2f"` can be provided instead */
  inline std::string toString(TJBox_Value const &iValue, char const *iFormat = nullptr) { return motherboard().toString(iValue, iFormat); }

  /**
   * Return a string representation of the property
   *
   * @param iPropertyPath the full path to the property (ex: `/custom_properties/my_prop`)
   * @param iFormat can be used to tweak the outcome */
  inline std::string toString(std::string const &iPropertyPath, char const *iFormat = nullptr) { return motherboard().toString(iPropertyPath, iFormat); }

  /**
   * Return the full path of the property ref.
   *
   * @return the full path to the property (ex: `/custom_properties/my_prop`) */
  inline std::string getPropertyPath(TJBox_PropertyRef const &iPropertyRef) const { return motherboard().getPropertyPath(iPropertyRef); }

  /**
   * Behaves like the inverse operation of `JBox_GetMotherboardObjectRef`
   *
   * @param iObjectRef the
   * @return the path of the ref provided (ex: `/custom_properties`) */
  inline std::string getObjectPath(TJBox_ObjectRef iObjectRef) const { return motherboard().getObjectPath(iObjectRef); }

  /**
   * Convenient API to get the value of the property and cast it to the proper (number) type. For example:
   *
   * ```cpp
   * tester.device().getNum<int>("/custom_properties/my_int_property");
   *
   * // equivalent to (but obviously much easier to write)
   * static_cast<int>(JBox_GetNumber(getValue("/custom_properties/my_int_property"));
   * ```
   *
   * @note If the property is not a number, this call fails (exception)
   */
  template<typename T = TJBox_Float64>
  inline T getNum(std::string const &iPropertyPath) const { return motherboard().getNum<T>(iPropertyPath); }

  /**
   * Convenient API to set the value of the property when it is a number. For example:
   *
   * ```cpp
   * tester.device().setNum("/custom_properties/my_int_property", 3);
   *
   * // equivalent to (but obviously much easier to write)
   * setValue("/custom_properties/my_int_property", JBox_MakeNumber(3));
   * ```
   *
   * @note If the property is not a number, this call fails (exception)
   */
  template<typename T = TJBox_Float64>
  inline void setNum(std::string const &iPropertyPath, T iValue) { motherboard().setNum<T>(iPropertyPath, iValue);}

  /**
   * Convenient API to get the value of the string property.
   *
   * @note If the property is not a string, this call fails (exception) */
  inline std::string getString(std::string const &iPropertyPath) const { return motherboard().getString(iPropertyPath); }

  /**
   * Convenient API to set the value of the string property.
   *
   * @note If the property is not a string, this call fails (exception) */
  inline void setString(std::string const &iPropertyPath, std::string iValue) { motherboard().setString(iPropertyPath, std::move(iValue)); }

  /**
   * Convenient API to get the value of the RT string property.
   *
   * @note If the property is not an RT string, this call fails (exception) */
  inline std::string getRTString(std::string const &iPropertyPath) const { return motherboard().getRTString(iPropertyPath); }

  /**
   * Convenient API to set the value of the RT string property.
   *
   * @note If the property is not an RT string, this call fails (exception) */
  inline void setRTString(std::string const &iPropertyPath, std::string const &iValue) { motherboard().setRTString(iPropertyPath, iValue); }

  //! Return the value of the `/custom_properties/builtin_onoffbypass` property (exception if not an effect)
  inline TJBox_OnOffBypassStates getEffectBypassState() const { return motherboard().getEffectBypassState(); }

  //! Set the value of the `/custom_properties/builtin_onoffbypass` property (exception if not an effect)
  inline void setEffectBypassState(TJBox_OnOffBypassStates iState) { motherboard().setEffectBypassState(iState); }

  //! Return the value of the `/environment/player_bypassed` property (exception if not a note player)
  inline bool isNotePlayerBypassed() const { return motherboard().isNotePlayerBypassed(); }

  //! Set the value of the `/environment/player_bypassed` property (exception if not a note player)
  inline void setNotePlayerBypassed(bool iBypassed) { motherboard().setNotePlayerBypassed(iBypassed); }

  /**
   * Set a single note event. For example:
   *
   * ```cpp
   * tester.device().setNoteInEvent(Midi::C(3), 100);
   * ```
   *
   * @param iNoteNumber The (midi) note number
   * @param iVelocity The velocity (0 for note off)
   * @param iAtFrameIndex When in the batch it happens (must be between 0 and 63).
   *
   * @note If this method is called for the same note number (between 2 batches), multiple diffs will be generated
   *       on the next batch call.
   *
   * @see `Midi` for a convenient API for the note number */
  inline void setNoteInEvent(TJBox_UInt8 iNoteNumber, TJBox_UInt8 iVelocity, TJBox_UInt16 iAtFrameIndex = 0) { motherboard().setNoteInEvent(iNoteNumber, iVelocity, iAtFrameIndex); }

  /**
   * Set a single note event (from a Jukebox `TJBox_NoteEvent`).
   *
   * @see `setNoteInEvent(TJBox_UInt8, TJBox_UInt8, TJBox_UInt16)` */
  inline void setNoteInEvent(TJBox_NoteEvent const &iNoteEvent) { motherboard().setNoteInEvent(iNoteEvent); }

  /**
   * Set multiple note events at once
   *
   * @see `setNoteInEvent(TJBox_UInt8, TJBox_UInt8, TJBox_UInt16)` */
  inline void setNoteInEvents(Motherboard::NoteEvents const &iNoteEvents) { motherboard().setNoteInEvents(iNoteEvents); }

   //! Get the value of the CV socket given its full path (`/cv_inputs/my_cv_socket`)
  inline TJBox_Float64 getCVSocketValue(std::string const &iSocketPath) const { return motherboard().getCVSocketValue(iSocketPath); }

  //! Set the value of the CV socket given its full path (`/cv_inputs/my_cv_socket`)
  inline void setCVSocketValue(std::string const &iSocketPath, TJBox_Float64 iValue) { motherboard().setCVSocketValue(iSocketPath, iValue); }

   //! Get the value of the CV socket
  inline TJBox_Float64 getCVSocketValue(CVSocket const &iSocket) const { return motherboard().getCVSocketValue(iSocket.fSocketRef); }

  //! Set the value of the CV socket
  inline void setCVSocketValue(CVSocket const &iSocket, TJBox_Float64 iValue) const { motherboard().setCVSocketValue(iSocket.fSocketRef, iValue); }

  /**
   * Get the current DSP buffer associated to the audio socket path. This is conceptually equivalent to:
   *
   * ```cpp
   * auto value = JBox_LoadMOMProperty(JBox_MakePropertyRef(JBox_GetMotherboardObjectRef("/audio_outputs"), "out")));
   * TJBox_AudioSample out[64];
   * JBox_GetDSPBufferData(value, 0, 64, out);
   * ```
   */
  inline Motherboard::DSPBuffer getDSPBuffer(std::string const &iAudioSocketPath) const { return motherboard().getDSPBuffer(iAudioSocketPath); }

  /**
   * Set the current DSP buffer associated to the audio socket path. This is conceptually equivalent to:
   *
   * ```cpp
   * auto value = JBox_LoadMOMProperty(JBox_MakePropertyRef(JBox_GetMotherboardObjectRef("/audio_outputs"), "out")));
   * TJBox_AudioSample in[64] = { ... };
   * JBox_SetDSPBufferData(value, 0, 64, out);
   * ```
   */
  inline void setDSPBuffer(std::string const &iAudioSocketPath, Motherboard::DSPBuffer iBuffer) { motherboard().setDSPBuffer(iAudioSocketPath, std::move(iBuffer)); }

  //! Get the current DSP buffer associated to the audio socket
  inline Motherboard::DSPBuffer getDSPBuffer(AudioSocket const &iSocket) const { return motherboard().getDSPBuffer(iSocket.fSocketRef); }

  //! Set the current DSP buffer associated to the audio socket
  inline void setDSPBuffer(AudioSocket const &iSocket, Motherboard::DSPBuffer iBuffer) { motherboard().setDSPBuffer(iSocket.fSocketRef, std::move(iBuffer)); }

  /**
   * Request a reset audio: this triggers the property `/transport/request_reset_audio` to be changed so that the
   * device can take action on the next batch and acts accordingly to the Reason requirements. */
  inline void requestResetAudio() { motherboard().requestResetAudio(); }

  /**
   * Trigger a change to the property `/transport/request_run` so that the device can take action on the next batch
   * and acts accordingly to the Reason requirements. */
  inline void requestRun() { motherboard().requestRun(); }

  /**
   * Trigger a change to the property `/transport/request_stop` so that the device can take action on the next batch
   * and acts accordingly to the Reason requirements. */
  inline void requestStop() { motherboard().requestStop(); }

  /**
   * Get the RW native object (similar to `JBox_GetNativeObjectRW`). This method is templated so that the cast
   * happens automatically. */
  template<typename T>
  inline T *getNativeObjectRW(std::string const &iPropertyPath) const { return motherboard().getNativeObjectRW<T>(iPropertyPath); }

  /**
   * Get the RO native object (similar to `JBox_GetNativeObjectRO`). This method is templated so that the cast
   * happens automatically. */
  template<typename T>
  inline const T *getNativeObjectRO(std::string const &iPropertyPath) const { return motherboard().getNativeObjectRO<T>(iPropertyPath); }

  /**
   * Loads and applies the patch represented by the "path". The implementation applies the following algorithm to
   * locate the patch until it finds a match.
   *
   * 1. Assumes the "path" is a resource path so tries to locate the patch via the config provided when creating the
   *    device (ex: `/Public/Kick.repatch`)
   * 2. Assumes the "path" is an absolute path to a file on the file system (ex: `/tmp/Bass.repatch`)
   * 3. Assumes the "path" is relative to the `Config::device_resources_dir()` directory
   *    (ex: `/test/Resources/test1.repatch`)
   * 4. Fail with exception
   */
  inline void loadPatch(std::string const &iPatchPath) { motherboard().loadPatch(iPatchPath); }

  //! Parses and applies the patch represented by the (XML formatted) string (see Reason doc for details on format)
  inline void loadPatch(resource::String const &iPatchString) { motherboard().loadPatch(iPatchString); }

  //! Loads and applies the patch file
  inline void loadPatch(resource::File const &iPatchFile) { motherboard().loadPatch(iPatchFile); }

  //! Applies the patch
  inline void loadPatch(resource::Patch const &iPatch) { motherboard().loadPatch(iPatch); }

  /**
   * Generate a patch object containing all properties that can be saved into a patch (persistence is set to `patch`).
   *
   * @note This can be an easy way to snapshot the state the of the device, modify it, then later on revert to this
   *       known state by calling `loadPatch(snapshotPatch)` */
  inline resource::Patch generatePatch() const { return motherboard().generatePatch(); }

  /**
   * When the device is instantiated, the lua code can provide default values for all the properties. If the device
   * uses patches and defines a default patch (in `info.lua`), then the default values are overiden by the values
   * defined in the patch. This call returns a patch that represents the state of the device prior to applying the
   * default patch.
   */
  inline resource::Patch const &getDefaultValuesPatch() const { return motherboard().getDefaultValuesPatch(); }

  /**
   * Reset the device to its default state (equivalent to `loadPatch(getDefaultValuesPatch())`)
   *
   * @see `getDefaultValuesPatch()` */
  inline void reset() { motherboard().reset(); }

  /**
   * When using the "loading context" feature to simulate slow disk access, this API loads more data for the blob
   * for the given property.
   *
   * @param iPropertyPath full path to the blob property (ex: `/custom_properties/my_blob`)
   * @param iCount if `-1` loads the remainder of the blob, otherwise loads an additional `iCount`
   * @return `true` if the blob is fully loaded (aka resident), `false` otherwise
   *
   * @see The Advanced Topic documentation for more details on this feature */
  inline bool loadMoreBlob(std::string const &iPropertyPath, long iCount = -1) { return motherboard().loadMoreBlob(iPropertyPath, iCount); }

  //! Shortcut to set the current user sample property (`/device_host/sample_context`)
  inline void selectCurrentUserSample(int iUserSampleIndex) { motherboard().selectCurrentUserSample(iUserSampleIndex); }

  /**
   * Load a user sample associated to the property. The implementation applies the following algorithm to
   * locate the sample until it finds a match.
   *
   * 1. Assumes the "path" is a resource path so tries to locate the sample via the config provided when creating
   *    the device (ex: `/Public/sine.wav`)
   * 2. Assumes the "path" is an absolute path to a file on the file system (ex: `/tmp/sine.wav`)
   * 3. Assumes the "path" is relative to the `Config::device_resources_dir()` directory
   *    (ex: `/test/Resources/sine.wav`)
   * 4. Fail with exception
   *
   * @param iPropertyPath the path to the user sample property (must be of the form `/user_samples/<num>/item`)
   * @param iResourcePath see above for details on what the "path" represents
   * @param iCtx an optional loading context to simulate/mock issues while loading the sample
   */
  inline void loadUserSampleAsync(std::string const &iPropertyPath,
                                  std::string const &iResourcePath,
                                  std::optional<resource::LoadingContext> iCtx = std::nullopt) {
    motherboard().loadUserSampleAsync(iPropertyPath, iResourcePath, iCtx);
  }

  //! Shortcut to `loadUserSampleAsync("/user_samples/<iUserSampleIndex>/item", iResourcePath, iCtx)`
  inline void loadUserSampleAsync(int iUserSampleIndex,
                                  std::string const &iResourcePath,
                                  std::optional<resource::LoadingContext> iCtx = std::nullopt) {
    motherboard().loadUserSampleAsync(iUserSampleIndex, iResourcePath, iCtx);
  }

  //! Shortcut using the current user sample (`/device_host/sample_context`)
  inline void loadCurrentUserSampleAsync(std::string const &iResourcePath,
                                         std::optional<resource::LoadingContext> iCtx = std::nullopt) {
    motherboard().loadCurrentUserSampleAsync(iResourcePath, iCtx);
  }

  //! Delete the user sample associated to the property (must be of the form `/user_samples/<num>/item`)
  inline void deleteUserSample(std::string const &iPropertyPath) { motherboard().deleteUserSample(iPropertyPath); }

  //! Shortcut to `deleteUserSample("/user_samples/<iUserSampleIndex>/item")`
  inline void deleteUserSample(int iUserSampleIndex) { motherboard().deleteUserSample(iUserSampleIndex); }

  //! Shortcut using the current user sample (`/device_host/sample_context`)
  inline void deleteCurrentUserSample() { motherboard().deleteCurrentUserSample(); }

  /**
   * When using the "loading context" feature to simulate slow disk access, this API loads more data for the sample
   * for the given property.
   *
   * @param iPropertyPath full path to the sample property (ex: `/custom_properties/my_sample` or
   *                      `/user_samples/<num>/item`)
   * @param iCount if `-1` loads the remainder of the sample, otherwise loads an additional `iCount`
   * @return `true` if the sample is fully loaded (aka resident), `false` otherwise
   *
   * @note this API works for both regular samples and user samples
   *
   * @see The Advanced Topic documentation for more details on this feature */
  inline bool loadMoreSample(std::string const &iPropertyPath, long iFrameCount = -1) { return motherboard().loadMoreSample(iPropertyPath, iFrameCount); }

  //! Set the properties `/transport/pattern_index` and `/transport/pattern_start_pos` with the given values
  inline void selectCurrentPattern(int iPatternIndex, int iPatternStartPos) { motherboard().selectCurrentPattern(iPatternIndex, iPatternStartPos); }

  /**
   * Associate a resource loading context to the provided resource path.
   *
   * @note If a loading context is defined in the config for the resource path provided to this call, this method
   *       overrides it
   *
   * @see The Advanced Topic documentation for more details on this feature */
  inline void setResourceLoadingContext(std::string const &iResourcePath, resource::LoadingContext const &iCtx) { motherboard().setResourceLoadingContext(iResourcePath, iCtx); }

  //! Clear a previously associated resource loading context (including if provided via config)
  inline void clearResourceLoadingContext(std::string const &iResourcePath) { motherboard().clearResourceLoadingContext(iResourcePath); }

  //! Shortcut method to retrieve the actual device as instantiated in `JBox_Export_CreateNativeObject`
  template<typename T>
  inline T* getInstance() const { return motherboard().getInstance<T>(); }

  //! Return the information about the given property (meta information)
  JboxPropertyInfo const &getPropertyInfo(std::string const &iPropertyPath) const { return motherboard().getPropertyInfo(iPropertyPath); }

  //! Return the information about ALL properties known to the motherboard (includes transport, device_host, ...)
  std::vector<JboxPropertyInfo> getPropertyInfos() const { return motherboard().getPropertyInfos(); }

  //! Enable dispatching property diffs to the device
  void enableRTCNotify() { motherboard().enableRTCNotify(); }

  //! Disable dispatching property diffs to the device (behaves as if rt_input_setup / notify is empty)
  void disableRTCNotify() { motherboard().disableRTCNotify(); }

  //! Enable dispatching property diffs to rtc bindings
  void enableRTCBindings() { motherboard().enableRTCBindings(); }

  //! Disable dispatching property diffs to rtc bindings (behaves as if rtc_bindings is empty)
  void disableRTCBindings() { motherboard().disableRTCBindings(); }

  int getInstanceId() const;

  friend class re::mock::Rack;

protected:
  Motherboard &motherboard() const;

protected:
  explicit Extension(std::shared_ptr<impl::ExtensionImpl> iExtensionImpl) : fImpl{iExtensionImpl} {}

private:
  std::shared_ptr<impl::ExtensionImpl> fImpl;
};

/**
 * The main purpose of this subclass is to provide a direct and convenient access to the device (of templated type
 * `Device`) under test via the arrow notation. For example:
 *
 * ```cpp
 * tester.device().xxx(); // access all methods on "this" class (`Extension`)
 * tester.device()->yyy(); // access all methods/member of the device under test (`typename Device`)
 * ```
 * @tparam Device
 */
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
