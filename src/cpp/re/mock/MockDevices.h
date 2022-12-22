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
#ifndef __Pongasoft_re_mock_mock_devices_h__
#define __Pongasoft_re_mock_mock_devices_h__

#include "Constants.h"
#include "Rack.h"
#include <array>
#include <ostream>

namespace re::mock {

/**
 * Define a convenient way to specify a midi note in most APIs that require a "note number". It is definitely
 * easier, more self documenting and less error prone to write `Midi::C(3)` than `60` */
struct Midi {
  constexpr static TJBox_UInt8 C(int octave) noexcept { return (octave + 2) * 12; }
  constexpr static TJBox_UInt8 C_sharp(int octave) noexcept { return C(octave) + 1; }
  constexpr static TJBox_UInt8 D_flat(int octave) noexcept { return C_sharp(octave); }
  constexpr static TJBox_UInt8 D(int octave) noexcept { return D_flat(octave) + 1; }
  constexpr static TJBox_UInt8 D_sharp(int octave) noexcept { return D(octave) + 1; }
  constexpr static TJBox_UInt8 E_flat(int octave) noexcept { return D_sharp(octave); }
  constexpr static TJBox_UInt8 E(int octave) noexcept { return E_flat(octave) + 1; }
  constexpr static TJBox_UInt8 F(int octave) noexcept { return E(octave) + 1; }
  constexpr static TJBox_UInt8 F_sharp(int octave) noexcept { return F(octave) + 1; }
  constexpr static TJBox_UInt8 G_flat(int octave) noexcept { return F_sharp(octave); }
  constexpr static TJBox_UInt8 G(int octave) noexcept { return G_flat(octave) + 1; }
  constexpr static TJBox_UInt8 G_sharp(int octave) noexcept { return G(octave) + 1; }
  constexpr static TJBox_UInt8 A_flat(int octave) noexcept { return G_sharp(octave); }
  constexpr static TJBox_UInt8 A(int octave) noexcept { return A_flat(octave) + 1; }
  constexpr static TJBox_UInt8 A_sharp(int octave) noexcept { return A(octave) + 1; }
  constexpr static TJBox_UInt8 B_flat(int octave) noexcept { return A_sharp(octave); }
  constexpr static TJBox_UInt8 B(int octave) noexcept { return B_flat(octave) + 1; }

  constexpr static TJBox_UInt8 A_440 = 69; // for some reason A(3) does not compile
};

/**
 * Mock devices are small rack extensions that are used by the various `DeviceTesters` to implement basic behavior.
 * This is the base class that implement the common behavior (keeps track of the sample rate and define the
 * `renderBatch` method)
 */
class MockDevice
{
public:
  /**
   * Convenient builder style api to create note events. For example:
   *
   * ```cpp
   * NoteEvents{}.noteOn(Midi::C(3), 98).noteOff(Midi::D(4));
   * ```
   */
  struct NoteEvents
  {
    NoteEvents &events(Motherboard::NoteEvents const &iNoteEvents);
    NoteEvents &event(TJBox_NoteEvent const &iNoteEvent);
    NoteEvents &noteOn(TJBox_UInt8 iNoteNumber, TJBox_UInt8 iVelocity = 100, TJBox_UInt16 iAtFrameIndex = 0);
    NoteEvents &note(TJBox_UInt8 iNoteNumber, TJBox_UInt8 iVelocity, TJBox_UInt16 iAtFrameIndex);
    NoteEvents &noteOff(TJBox_UInt8 iNoteNumber, TJBox_UInt16 iAtFrameIndex = 0);
    NoteEvents &allNotesOff();
    NoteEvents &clear();

    Motherboard::NoteEvents const &events() const { return fNoteEvents; }
    operator Motherboard::NoteEvents const &() const { return fNoteEvents; }

    friend std::ostream &operator<<(std::ostream &os, NoteEvents const &events);

    friend bool operator==(MockDevice::NoteEvents const &lhs, MockDevice::NoteEvents const &rhs);
    friend bool operator!=(MockDevice::NoteEvents const &lhs, MockDevice::NoteEvents const &rhs);

    Motherboard::NoteEvents fNoteEvents{};
  };

public:
  explicit MockDevice(int iSampleRate);
  virtual ~MockDevice() = default; // allow for subclassing
  virtual void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) {};

public:
  int fSampleRate;
};

//------------------------------------------------------------------------
// Audio Devices
//------------------------------------------------------------------------

/**
 * Base class for all mock devices that represent an audio device: they all have a pair of audio sockets and a
 * `fBuffer` for the current batch. */
class MockAudioDevice : public MockDevice
{
public:
  constexpr static auto LEFT_SOCKET = "L";
  constexpr static auto RIGHT_SOCKET = "R";

  //! Represent the standard buffer for audio processing in Reason: an array of 64 `TJBox_AudioSample`
  using buffer_type = std::array<TJBox_AudioSample, constants::kBatchSize>;

  /**
   * Encapsulates a pair of buffers, one for the left, one for the right */
  struct StereoBuffer
  {
    buffer_type fLeft{};
    buffer_type fRight{};

    //! This convenient API fills the left buffer with `iLeftSample` and the right buffer with `iRightSample`.
    StereoBuffer &fill(TJBox_AudioSample iLeftSample, TJBox_AudioSample iRightSample);

    friend std::ostream& operator<<(std::ostream& os, const StereoBuffer& iBuffer);
    friend bool operator==(StereoBuffer const &lhs, StereoBuffer const &rhs);
    friend bool operator!=(StereoBuffer const &lhs, StereoBuffer const &rhs);
  };

  //! Define a pair of sockets
  struct StereoSocket
  {
    TJBox_ObjectRef fLeft{};
    TJBox_ObjectRef fRight{};

    static StereoSocket input();
    static StereoSocket output();
  };

  /**
   * Concept of a sample used for testing. For example the `DeviceTester::loadSample` api returns an instance of
   * this class. The primary difference between this class and `resource::Sample` is that this one is meant to be
   * used in testing with many convenient methods provided to manipulate the sample itself.
   *
   * @note Most APIs use the builder pattern syntax and return the object itself for easy chaining */
  struct Sample
  {
    TJBox_UInt32 fChannels{1};
    TJBox_UInt32 fSampleRate{1};
    std::vector<TJBox_AudioSample> fData{};

    Sample &channels(TJBox_UInt32 c) { fChannels = c; return *this; }
    Sample &mono() { return channels(1); }
    Sample &stereo() { return channels(2); }
    Sample &sample_rate(TJBox_UInt32 s) { fSampleRate = s; return *this; }
    Sample &data(std::vector<TJBox_AudioSample> d) { fData = std::move(d); return *this; }

    //! Return a copy of this object
    Sample clone() const { return *this; }

    //! Clear the sample data
    void clear() { fData.clear(); }

    //! Append up to `iFrameCount` from the stereo buffer to this sample (if `-1`, appends all)
    Sample &append(StereoBuffer const &iAudioBuffer, size_t iFrameCount = -1);

    //! Append up to `iFrameCount` from the other sample to this sample (if `-1`, appends all)
    Sample &append(Sample const &iOtherSample, size_t iFrameCount = -1);

    /**
     * Mix, up to `iFrameCount` frames, from the other sample with this sample. Mixing simply add the 2 samples
     * together. If `iFrameCount` is `-1` then mixes all samples.
     *
     * @note the implementation takes care of increasing the size of `this` sample if more samples are required to do
     *       the mix.
     */
    Sample &mixWith(Sample const &iOtherSample, size_t iFrameCount = -1);

    //! Multiply every sample (all channels) by the gain provided
    Sample &applyGain(TJBox_Float32 iGain);

    /**
     * Extract the subsample starting at frame `iFromFrame`.
     *
     * @param iFromFrame which frame to start
     * @param iFrameCount how many frames to include. If `-1` include all until the end.
     *
     * @note this call modifies the sample itself! */
    Sample &subSample(size_t iFromFrame = 0, size_t iFrameCount = -1);

    //! Return `true` if 1 channel
    bool isMono() const { return fChannels == 1; }

    //! Return `true` if 2 channels
    bool isStereo() const { return fChannels == 2; }

    //! Return the number of channels
    TJBox_UInt32 getChannels() const { return fChannels; }

    //! Return the sample rate
    TJBox_UInt32 getSampleRate() const { return fSampleRate; }

    //! Return the vector containing the (interleaved) audio samples
    std::vector<TJBox_AudioSample> const &getData() const { return fData; }

    //! Return the number of samples total
    TJBox_AudioFramePos getSampleCount() const { return static_cast<TJBox_AudioFramePos>(fData.size()); }

    //! Return the number of samples per channel
    TJBox_AudioFramePos getFrameCount() const {
      RE_MOCK_ASSERT(fData.size() % fChannels == 0);
      return static_cast<TJBox_AudioFramePos>(fData.size() / fChannels);
    }

    //! Return the duration of the sample in milliseconds
    float getDurationMilliseconds() const { return getFrameCount() * 1000.0 / fSampleRate; }

    //! Create and return a mono sample containing just the left channel
    Sample getLeftChannelSample() const;

    //! Create and return a mono sample containing just the right channel
    Sample getRightChannelSample() const;

    //! Removes all silent frames from the beginning of the sample
    Sample &trimBeginning();

    //! Removes all silent frames from the end of the sample
    Sample &trimEnd();

    //! Removes all silent frames from both the beginning and the end of the sample
    Sample &trim();

    //! Returns a string representation of this sample (up to `iFrameCount`)
    std::string toString(size_t iFrameCount = -1) const;

    //! Convert a `resource::Sample` to a `MockAudioDevice::Sample`
    static Sample from(resource::Sample iSample);

    //! Create a Sample from a stereo buffer
    static Sample from(StereoBuffer const &iStereoBuffer, TJBox_UInt32 iSampleRate);

    //! Convert a `resource::Sample` to a `MockAudioDevice::Sample` (avoid copies)
    static std::unique_ptr<Sample> from(std::unique_ptr<resource::Sample> iSample);

    static const std::size_t npos = -1;

    friend std::ostream& operator<<(std::ostream& os, const Sample& iBuffer);
    friend bool operator==(Sample const &lhs, Sample const &rhs);
    friend bool operator!=(Sample const &lhs, Sample const &rhs) { return !(rhs == lhs); }
    friend Sample operator+(Sample const &lhs, Sample const &rhs) { return lhs.clone().append(rhs); }

  private:
    void maybeGrowExponentially(size_t iNewCapacity);
  };

  class SampleConsumer
  {
  public:
    explicit SampleConsumer(MockAudioDevice::Sample const &iSample);

    bool consume(StereoBuffer &oBuffer);

  private:
    MockAudioDevice::Sample const &fSample;
    decltype(std::vector<TJBox_AudioSample>{}.cbegin()) fPtr;
  };

  //! Create a stereo buffer and fill the left channel with `iLeftSample` (resp. right with `iRightSample`)
  static StereoBuffer buffer(TJBox_AudioSample iLeftSample, TJBox_AudioSample iRightSample);

  //! Create a stereo buffer from left and right buffers
  static StereoBuffer buffer(buffer_type const &iLeftBuffer, buffer_type const &iRightBuffer);

  //! Compare 2 samples (to a certain precision) (use `std::almost_equal`)
  static bool eq(TJBox_AudioSample iSample1, TJBox_AudioSample iSample2);

  //! Compare 2 stereo buffers (use `eq(TJBox_AudioSample, TJBox_AudioSample)`)
  static bool eq(StereoBuffer const &iBuffer1, StereoBuffer const &iBuffer2);

  //! Compare 2 buffers (use `eq(TJBox_AudioSample, TJBox_AudioSample)`)
  static bool eq(buffer_type const &iBuffer1, buffer_type const &iBuffer2);

  //! Compare 2 samples (to the given precision) (in case the `eq(TJBox_AudioSample, TJBox_AudioSample)` method is too stringent)
  static bool eqWithPrecision(TJBox_AudioSample iPrecision, TJBox_AudioSample iSample1, TJBox_AudioSample iSample2);

  //! Compare 2 stereo buffers (to the given precision)
  static bool eqWithPrecision(TJBox_AudioSample iPrecision, StereoBuffer const &iBuffer1, StereoBuffer const &iBuffer2);

  //! Compare 2 buffers (to the given precision)
  static bool eqWithPrecision(TJBox_AudioSample iPrecision, buffer_type const &iBuffer1, buffer_type const &iBuffer2);

  //! Return `true` if the audio sample is silent as defined by the RE SDK
  static constexpr bool isSilent(TJBox_AudioSample iSample) {
    if(iSample < constants::kSilentThreshold)
      iSample = -iSample;
    else
      return false;
    return iSample < constants::kSilentThreshold;
  }

public:
  explicit MockAudioDevice(int iSampleRate);

  /**
   * Copy the stereo socket content to the provided buffer. Check that the socket is actually connected before doing
   * the copy operation.
   *
   * @return `true` if at least one socket is connected
   *
   * @note this API uses the Jukebox api so is designed to be called from `renderBatch`! */
  static bool copyBuffer(StereoSocket const &iFromSocket, StereoBuffer &iToBuffer);

  /**
   * Copy the content of the buffer to the socket. Check that the socket is actually connected before doing
   * the copy operation.
   *
   * @return `true` if at least one socket is connected
   *
   * @note this API uses the Jukebox api so is designed to be called from `renderBatch`! */
  static bool copyBuffer(StereoBuffer const &iFromBuffer, StereoSocket const &iToSocket);

  /**
   * Copy the socket content to the provided buffer. Check that the socket is actually connected before doing
   * the copy operation.
   *
   * @return `true` if the socket is connected
   *
   * @note this API uses the Jukebox api so is designed to be called from `renderBatch`! */
  static bool copyBuffer(TJBox_ObjectRef const &iFromSocket, buffer_type &iToBuffer);

  /**
   * Copy the content of the buffer to the socket. Check that the socket is actually connected before doing
   * the copy operation.
   *
   * @return `true` if the socket is connected
   *
   * @note this API uses the Jukebox api so is designed to be called from `renderBatch`! */
  static bool copyBuffer(buffer_type const &iFromBuffer, TJBox_ObjectRef const &iToSocket);

  //! Copy `iFromBuffer` to `oToBuffer`
  static void copyBuffer(StereoBuffer const &iFromBuffer, StereoBuffer &oToBuffer);

  //! Copy `iFromBuffer` to `oToBuffer`
  static void copyBuffer(buffer_type const &iFromBuffer, buffer_type &oToBuffer);

  /**
   * Wire the stereo audio sockets from `iFromExtension` to the stereo audio sockets of `iToExtension`
   *
   * @note `From` and `To` must be subclasses of `MockAudioDevice` */
  template<typename From, typename To>
  static void wire(Rack &iRack,
                   rack::ExtensionDevice<From> const &iFromExtension,
                   rack::ExtensionDevice<To> const &iToExtension);

  /**
   * Wire the stereo audio sockets from `iFromExtension` to the stereo audio sockets provided
   *
   * @note `From` must be a subclass of `MockAudioDevice` */
  template<typename From>
  static void wire(Rack &iRack,
                   rack::ExtensionDevice<From> const &iFromExtension,
                   rack::Extension::StereoAudioInSocket const &iToSockets);

  /**
   * Wire the stereo audio sockets provided to the stereo audio socket of `iToExtension`
   *
   * @note `To` must be a subclass of `MockAudioDevice` */
  template<typename To>
  static void wire(Rack &iRack,
                   rack::Extension::StereoAudioOutSocket const &iFromSockets,
                   rack::ExtensionDevice<To> const &iToExtension);

public:
  /**
   * All subclasses use this buffer during the `renderBatch` operation and it is publicly available so
   * that testing code can access it:
   *
   * ```cpp
   * // assuming tester is an InstrumentTester
   * tester.src()->fBuffer; // access the buffer directly
   * ```
   */
  StereoBuffer fBuffer{};
};

/**
 * Audio source mock device. The `renderBath` method copies `fBuffer` to a stereo pair of output sockets
 * (if connected).
 *
 * For example:
 *
 * ```cpp
 * // assuming tester is a StudioEffectTester (tester.src() is MAUSrc)
 * tester.src().fBuffer.fill(0.25, 0.5);
 * tester.nextBatch(); // MAUSrc copies fBuffer to its stereo out sockets
 * ```
 *
 * Optionally, this mock audio device can "consume" a sample: the sample is turned into chunks of 64 frames and
 * every call to `renderBatch` copies a chunk to the output sockets.
 *
 * ```cpp
 * // assuming tester is a StudioEffectTester (tester.src() is MAUSrc)
 * auto sample = tester.loadSample(...);
 * tester.src().consumeSample(*sample);
 * tester.nextBatch(); // MAUSrc copies frames [0, 64) of "sample" to its stereo out sockets
 * tester.nextBatch(); // MAUSrc copies frames [64, 128) of "sample" to its stereo out sockets
 * // etc...
 * ```
 *
 * @note Consuming a sample is used by `StudioEffectTester::processSample` apis */
class MAUSrc : public MockAudioDevice
{
public:
  explicit MAUSrc(int iSampleRate);

  //! Copy `fBuffer` to the out socket or consume a sample if one provided
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MAUSrc> CONFIG;

  /**
   * Instruct this device to consume the sample (see description of the class for example).
   *
   * @note once the sample is fully consumed, `fBuffer` is filled with 0 and processing continues with `fBuffer` only */
  void consumeSample(Sample const &iSample);

  //! Return `true` is the sample is fully consumed
  bool isSampleConsumed() const { return fSampleConsumer == nullptr; }

protected:
  void renderBuffer();
  void renderSample();

protected:
  StereoSocket fOutSocket{};
  std::unique_ptr<SampleConsumer> fSampleConsumer{};
};

/**
 * Audio destination mock device. The `renderBath` method copies the stereo pair of input sockets to
 * `fBuffer` (if sockets are connected)
 *
 * For example:
 *
 * ```cpp
 * // assuming tester is a StudioEffectTester (tester.dst() is MAUDst)
 * tester.nextBatch(); // MAUDst copies its stereo input sockets to fBuffer
 * tester.dst()->fBuffer; // contains the result of the copy operation
 * ```
 *
 * Optionally, this mock audio device can produce a sample by concatenating the buffers generated in each `renderBatch`.
 *
 * ```cpp
 * // assuming tester is a StudioEffectTester (tester.dst() is MAUDst)
 * tester.dst().produceSample();
 * tester.nextBatch(); // MAUDst appends the content of its stereo input sockets to a sample
 * tester.nextBatch(); // MAUDst appends the content of its stereo input sockets to a sample
 * // etc...
 * auto sample = tester.dst()->getSample(); // the generated sample
 * ```
 *
 * @note Producing a sample is used by `InstrumentTester::bounce` apis */
class MAUDst : public MockAudioDevice
{
public:
  explicit MAUDst(int iSampleRate);

  //! Copy the content of the stereo input sockets to `fBuffer` and optionally append to a sample
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MAUDst> CONFIG;

  //! Instruct the device to generate a sample by concatenating the buffers produced
  void produceSample(TJBox_UInt32 iChannels, TJBox_UInt32 iSampleRate);

  //! Instruct the device to generate a sample by concatenating the buffers produce (stereo / `fSampleRate`)
  void produceSample() { produceSample(2, fSampleRate); }

  /**
   * @return the sample generated and effectively stops producing in subsequent `renderBatch` calls
   *         (until `produceSample` is called again) */
  std::unique_ptr<Sample> getSample();

  /**
   * @return the sample (so far) generated. Does not stop producing the sample. */
  Sample const &peekSample() const { RE_MOCK_ASSERT(fSample != nullptr); return *fSample; }

protected:
  StereoSocket fInSocket{};
  std::unique_ptr<Sample> fSample{};
};

/**
 * Audio pass through mock device: copy input to output. Keep a copy of the buffer in `fBuffer`.
 *
 * @note This device is a studio effect and the action of copying can be disabled by changing the bypass state */
class MAUPst : public MockAudioDevice
{
public:
  explicit MAUPst(int iSampleRate);

  //! Copy the input sockets to the output sockets (while keeping a copy in `fBuffer`) if the effect is not bypassed
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MAUPst> CONFIG;

  //! Return the state of the effect
  TJBox_OnOffBypassStates getBypassState() const;

  //! Changes the state of the effect (setting it to off disables the copy operation)
  void setBypassState(TJBox_OnOffBypassStates iState);

protected:
  TJBox_ObjectRef fCustomPropertiesRef;
  StereoSocket fInSocket{};
  StereoSocket fOutSocket{};
};

//------------------------------------------------------------------------
// CV Devices
//------------------------------------------------------------------------

/**
 * Base class for all mock devices that represent a CV device: they all have a cv sockets and a
 * `fValue` for the current value. */
class MockCVDevice : public MockDevice
{
public:
  constexpr static auto SOCKET = "C";

  using value_type = TJBox_Float64; // cv values are float64 numbers

public:
  explicit MockCVDevice(int iSampleRate);

  /**
   * Load the CV value from the socket and store it in `oValue`
   *
   * @return `true` if the socket is connected
   *
   * @note this API uses the Jukebox api so is designed to be called from `renderBatch`! */
  static bool loadValue(TJBox_ObjectRef const &iFromSocket, TJBox_Float64 &oValue);

  /**
   * Store the value to the CV the socket.
   *
   * @return `true` if the socket is connected
   *
   * @note this API uses the Jukebox api so is designed to be called from `renderBatch`! */
  static bool storeValue(TJBox_Float64 iValue, TJBox_ObjectRef const &iToSocket);


  /**
   * Load the CV value from the socket and store it in `fValue`
   *
   * @return `true` if the socket is connected
   *
   * @note this API uses the Jukebox api so is designed to be called from `renderBatch`! */
  bool loadValue(TJBox_ObjectRef const &iFromSocket);

  /**
   * Store `fValue` to the CV the socket.
   *
   * @return `true` if the socket is connected
   *
   * @note this API uses the Jukebox api so is designed to be called from `renderBatch`! */
  bool storeValue(TJBox_ObjectRef const &iToSocket);

  /**
   * Wire the CV socket from `iFromExtension` to the CV socket of `iToExtension`
   *
   * @note `From` and `To` must be subclasses of `MockCVDevice` */
  template<typename From, typename To>
  static void wire(Rack &iRack, rack::ExtensionDevice<From> const &iFromExtension, rack::ExtensionDevice<To> const &iToExtension);

  /**
   * Wire the CV socket from `iFromExtension` to the CV socket provided
   *
   * @note `From` must be a subclass of `MockCVDevice` */
  template<typename From>
  static void wire(Rack &iRack, rack::ExtensionDevice<From> const &iFromExtension, rack::Extension::CVInSocket const &iToSocket);

  /**
   * Wire the CV socket provided to the CV socket of `iToExtension`
   *
   * @note `To` must be a subclass of `MockCVDevice` */
  template<typename To>
  static void wire(Rack &iRack, rack::Extension::CVOutSocket const &iFromSocket, rack::ExtensionDevice<To> const &iToExtension);

  //! Compare 2 CV values (to a certain precision) (use `std::almost_equal`)
  static bool eq(TJBox_Float64 iCV1, TJBox_Float64 iCV2);

public:
  /**
   * All subclasses use this value during the `renderBatch` operation and it is publicly available so
   * that testing code can access it:
   *
   * ```cpp
   * auto src = tester.wireNewCVSrc();
   * src->fValue; // accesses the value directly
   * ```
   */
  TJBox_Float64 fValue{};
};

/**
 * CV source mock device. The `renderBath` method copies `fValue` to the out socket.
 *
 * For example:
 *
 * ```cpp
 * auto src = tester.wireNewCVSrc("cv_in"); // cv_in is the cv input socket of the device under test
 * src->fValue = 0.5;
 * tester.nextBatch(); // MCVSrc copies fValue to its out socket hence the device tested "receives" it on its cv_in socket
 * ```
 */
class MCVSrc : public MockCVDevice
{
public:
  explicit MCVSrc(int iSampleRate);

  //! Copy `fValue` to out socket (if connected)
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MCVSrc> CONFIG;

protected:
  TJBox_ObjectRef fOutSocket{};
};

/**
 * CV destination mock device. The `renderBath` method copies the cv input socket to `fValue`.
 *
 * For example:
 *
 * ```cpp
 * auto dst = tester.wireNewCVDst("cv_out"); // cv_out is the cv output socket of the device under test
 * tester.nextBatch(); // MCVDst copies the value from its input socket (which it got from the device tested) to fValue
 * dst->fValue; // contains the value from the device under test
 * ```
 */
class MCVDst : public MockCVDevice
{
public:
  explicit MCVDst(int iSampleRate);

  //! Copy input socket to `fValue` (if connected)
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MCVDst> CONFIG;

protected:
  TJBox_ObjectRef fInSocket{};
};

/**
 * CV pass through mock device: copy input to output. Keep a copy of the value in `fValue` */
class MCVPst : public MockCVDevice
{
public:
  explicit MCVPst(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MCVPst> CONFIG;

protected:
  TJBox_ObjectRef fInSocket{};
  TJBox_ObjectRef fOutSocket{};
};

//------------------------------------------------------------------------
// Note Player
//------------------------------------------------------------------------

/**
 * Base class for all mock devices that represent a note player: they all have `fNoteEvents` for the current value */
class MockNotePlayer : public MockDevice
{
public:
  explicit MockNotePlayer(int iSampleRate);

   //! Wire the "note out" socket from `iFromExtension` to the "note in" socket of `iToExtension`
  static void wire(Rack &iRack, rack::Extension const &iFromExtension, rack::Extension const &iToExtension);


  /**
   * Return `true` if this note player is bypassed
   *
   * @note this API uses the Jukebox api so is designed to be called from `renderBatch`! */
  bool isBypassed() const;

  /**
   * Set the bypass state of this note player
   *
   * @note this API uses the Jukebox api so is designed to be called from `renderBatch`! */
  void setBypassed(bool iBypassed);

public:
  NoteEvents fNoteEvents{};

protected:
  TJBox_ObjectRef fEnvironmentRef;
};

/**
 * Note player source device. Copy note events to output. */
class MNPSrc : public MockNotePlayer
{
public:
  explicit MNPSrc(int iSampleRate);

  //! Copy the notes from `fNoteEvents` to output (use `JBox_OutputNoteEvent` on each note)
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MNPSrc> CONFIG;
};

/**
 * Note player destination device. Extract note events from diffs and copy into fNoteEvents. */
class MNPDst : public MockNotePlayer
{
public:
  explicit MNPDst(int iSampleRate);

  //! Extract all note events contained in `iPropertyDiffs` and copy them into `fNoteEvents`
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MNPDst> CONFIG;

protected:
  TJBox_ObjectRef fNoteStatesRef;
};

/**
 * Note player pass through device. Extract note events from diffs, copy into fNoteEvents and to output. */
class MNPPst : public MockNotePlayer
{
public:
  explicit MNPPst(int iSampleRate);

  //! Extract all note events contained in `iPropertyDiffs`, copy them into `fNoteEvents` and to output (`JBox_OutputNoteEvent`)
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MNPPst> CONFIG;

protected:
  TJBox_ObjectRef fNoteStatesRef;
};

//------------------------------------------------------------------------
// MockAudioDevice::wire
//------------------------------------------------------------------------
template<typename From, typename To>
void MockAudioDevice::wire(Rack &iRack,
                           rack::ExtensionDevice<From> const &iFromExtension,
                           rack::ExtensionDevice<To> const &iToExtension)
{
  static_assert(std::is_convertible<From*, MockAudioDevice*>::value, "From must be a subclass of MockAudioDevice");
  static_assert(std::is_convertible<To*, MockAudioDevice*>::value, "To must be a subclass of MockAudioDevice");
  iRack.wire(iFromExtension.getStereoAudioOutSocket(LEFT_SOCKET, RIGHT_SOCKET),
             iToExtension.getStereoAudioInSocket(LEFT_SOCKET, RIGHT_SOCKET));
}

//------------------------------------------------------------------------
// MockAudioDevice::wire
//------------------------------------------------------------------------
template<typename From>
void MockAudioDevice::wire(Rack &iRack,
                           rack::ExtensionDevice<From> const &iFromExtension,
                           rack::Extension::StereoAudioInSocket const &iToSockets)
{
  static_assert(std::is_convertible<From*, MockAudioDevice*>::value, "From must be a subclass of MockAudioDevice");
  iRack.wire(iFromExtension.getStereoAudioOutSocket(LEFT_SOCKET, RIGHT_SOCKET), iToSockets);
}

//------------------------------------------------------------------------
// MockAudioDevice::wire
//------------------------------------------------------------------------
template<typename To>
void MockAudioDevice::wire(Rack &iRack,
                           rack::Extension::StereoAudioOutSocket const &iFromSockets,
                           rack::ExtensionDevice<To> const &iToExtension)
{
  static_assert(std::is_convertible<To*, MockAudioDevice*>::value, "To must be a subclass of MockAudioDevice");
  iRack.wire(iFromSockets, iToExtension.getStereoAudioInSocket(LEFT_SOCKET, RIGHT_SOCKET));
}

//------------------------------------------------------------------------
// MockCVDevice::wire
//------------------------------------------------------------------------
template<typename From, typename To>
void MockCVDevice::wire(Rack &iRack,
                        rack::ExtensionDevice<From> const &iFromExtension,
                        rack::ExtensionDevice<To> const &iToExtension)
{
  static_assert(std::is_convertible<From*, MockCVDevice*>::value, "From must be a subclass of MockCVDevice");
  static_assert(std::is_convertible<To*, MockCVDevice*>::value, "To must be a subclass of MockCVDevice");
  iRack.wire(iFromExtension.getCVOutSocket(SOCKET), iToExtension.getCVInSocket(SOCKET));
}

//------------------------------------------------------------------------
// MockCVDevice::wire
//------------------------------------------------------------------------
template<typename From>
void MockCVDevice::wire(Rack &iRack,
                        rack::ExtensionDevice<From> const &iFromExtension,
                        rack::Extension::CVInSocket const &iToSocket)
{
  static_assert(std::is_convertible<From*, MockCVDevice*>::value, "From must be a subclass of MockCVDevice");
  iRack.wire(iFromExtension.getCVOutSocket(SOCKET), iToSocket);
}

//------------------------------------------------------------------------
// MockCVDevice::wire
//------------------------------------------------------------------------
template<typename To>
void MockCVDevice::wire(Rack &iRack,
                        rack::Extension::CVOutSocket const &iFromSocket,
                        rack::ExtensionDevice<To> const &iToExtension)
{
  static_assert(std::is_convertible<To*, MockCVDevice*>::value, "To must be a subclass of MockCVDevice");
  iRack.wire(iFromSocket, iToExtension.getCVInSocket(SOCKET));
}

}

#endif //__Pongasoft_re_mock_mock_devices_h__
