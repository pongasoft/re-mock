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

class MockDevice
{
public:
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

class MockAudioDevice : public MockDevice
{
public:
  constexpr static auto LEFT_SOCKET = "L";
  constexpr static auto RIGHT_SOCKET = "R";

  using buffer_type = std::array<TJBox_AudioSample, constants::kBatchSize>;

  struct StereoBuffer
  {
    buffer_type fLeft{};
    buffer_type fRight{};

    StereoBuffer &fill(TJBox_AudioSample iLeftSample, TJBox_AudioSample iRightSample);

    friend std::ostream& operator<<(std::ostream& os, const StereoBuffer& iBuffer);
    friend bool operator==(StereoBuffer const &lhs, StereoBuffer const &rhs);
    friend bool operator!=(StereoBuffer const &lhs, StereoBuffer const &rhs);
  };

  struct StereoSocket
  {
    TJBox_ObjectRef fLeft{};
    TJBox_ObjectRef fRight{};

    static StereoSocket input();
    static StereoSocket output();
  };

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
    Sample &reserveFromSampleCount(size_t iSampleCount) { fData.reserve(iSampleCount); return *this; }
    Sample &reserveFromFrameCount(TJBox_AudioFramePos iFrameCount) { return reserveFromSampleCount(iFrameCount * fChannels); }

    Sample clone() const { return *this; }
    void clear() { fData.clear(); }

    Sample &append(StereoBuffer const &iAudioBuffer, size_t iFrameCount = -1);
    Sample &append(Sample const &iOtherSample, size_t iFrameCount = -1);

    Sample &mixWith(Sample const &iOtherSample, size_t iFrameCount = -1);
    Sample &applyGain(TJBox_Float32 iGain);

    Sample subSample(size_t iFromFrame = 0, size_t iFrameCount = -1) const;

    bool isMono() const { return fChannels == 1; }
    bool isStereo() const { return fChannels == 2; }

    TJBox_UInt32 getChannels() const { return fChannels; }
    TJBox_UInt32 getSampleRate() const { return fSampleRate; }
    std::vector<TJBox_AudioSample> const &getData() const { return fData; }

    TJBox_AudioFramePos getSampleCount() const { return fData.size(); }
    TJBox_AudioFramePos getFrameCount() const {
      RE_MOCK_ASSERT(fData.size() % fChannels == 0);
      return fData.size() / fChannels;
    }
    float getDurationMilliseconds() const { return getFrameCount() * 1000.0 / fSampleRate; }

    Sample getLeftChannelSample() const;
    Sample getRightChannelSample() const;

    Sample trimLeft() const;
    Sample trimRight() const;
    Sample trim() const;

    std::string toString(size_t iFrameCount = -1) const;

    static Sample from(resource::Sample iSample);
    static Sample from(StereoBuffer const &iStereoBuffer, TJBox_UInt32 iSampleRate);

    static const std::size_t npos = -1;

    friend std::ostream& operator<<(std::ostream& os, const Sample& iBuffer);
    friend bool operator==(Sample const &lhs, Sample const &rhs);
    friend bool operator!=(Sample const &lhs, Sample const &rhs) { return !(rhs == lhs); }
    friend Sample operator+(Sample const &lhs, Sample const &rhs) { return lhs.clone().append(rhs); }
  };

  static StereoBuffer buffer(TJBox_AudioSample iLeftSample, TJBox_AudioSample iRightSample);
  static StereoBuffer buffer(buffer_type const &iLeftBuffer, buffer_type const &iRightBuffer);

  static bool eq(TJBox_AudioSample iSample1, TJBox_AudioSample iSample2);
  static bool eq(StereoBuffer const &iBuffer1, StereoBuffer const &iBuffer2);
  static bool eq(buffer_type const &iBuffer1, buffer_type const &iBuffer2);

  static bool eqWithPrecision(TJBox_AudioSample iPrecision, TJBox_AudioSample iSample1, TJBox_AudioSample iSample2);
  static bool eqWithPrecision(TJBox_AudioSample iPrecision, StereoBuffer const &iBuffer1, StereoBuffer const &iBuffer2);
  static bool eqWithPrecision(TJBox_AudioSample iPrecision, buffer_type const &iBuffer1, buffer_type const &iBuffer2);

  static constexpr bool isSilent(TJBox_AudioSample iSample) {
    if(iSample < constants::kSilentThreshold)
      iSample = -iSample;
    else
      return false;
    return iSample < constants::kSilentThreshold;
  }

public:
  explicit MockAudioDevice(int iSampleRate);
  static bool copyBuffer(StereoSocket const &iFromSocket, StereoBuffer &iToBuffer);
  static bool copyBuffer(StereoBuffer const &iFromBuffer, StereoSocket const &iToSocket);
  static bool copyBuffer(TJBox_ObjectRef const &iFromSocket, buffer_type &iToBuffer);
  static bool copyBuffer(buffer_type const &iFromBuffer, TJBox_ObjectRef const &iToSocket);
  static void copyBuffer(StereoBuffer const &iFromBuffer, StereoBuffer &iToBuffer);
  static void copyBuffer(buffer_type const &iFromBuffer, buffer_type &iToBuffer);

  template<typename From, typename To>
  static void wire(Rack &iRack, Rack::ExtensionDevice<From> const &iFromExtension, Rack::ExtensionDevice<To> const &iToExtension);
  template<typename From>
  static void wire(Rack &iRack, Rack::ExtensionDevice<From> const &iFromExtension, Rack::Extension::StereoAudioInSocket const &iToSockets);
  template<typename To>
  static void wire(Rack &iRack, Rack::Extension::StereoAudioOutSocket const &iFromSockets, Rack::ExtensionDevice<To> const &iToExtension);

public:
  StereoBuffer fBuffer{};
};

/**
 * Audio source mock device. Copy `fBuffer` to output. */
class MAUSrc : public MockAudioDevice
{
public:
  explicit MAUSrc(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MAUSrc> CONFIG;

  Sample fSample;
  size_t fTailInFrames{};
  bool fUseSample{false};

protected:
  void renderBuffer();
  void renderSample();

protected:
  StereoSocket fOutSocket{};

  size_t fNumFramesToProcess{};
  size_t fTotalNumFrames{};
  TJBox_AudioSample *fPtr{};
};

/**
 * Audio destination mock device. Copy input to `fBuffer` and append to `fSample`. */
class MAUDst : public MockAudioDevice
{
public:
  explicit MAUDst(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MAUDst> CONFIG;

  Sample fSample;
  bool fUseSample{false};

protected:
  StereoSocket fInSocket{};
};

/**
 * Audio pass through mock device: copy input to output. Keep a copy of the buffer in `fBuffer` */
class MAUPst : public MockAudioDevice
{
public:
  explicit MAUPst(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MAUPst> CONFIG;

  TJBox_OnOffBypassStates getBypassState() const;
  void setBypassState(TJBox_OnOffBypassStates iState);

protected:
  TJBox_ObjectRef fCustomPropertiesRef;
  StereoSocket fInSocket{};
  StereoSocket fOutSocket{};
};

//------------------------------------------------------------------------
// CV Devices
//------------------------------------------------------------------------

class MockCVDevice : public MockDevice
{
public:
  constexpr static auto SOCKET = "C";

  using value_type = TJBox_Float64; // cv values are float64 numbers

public:
  explicit MockCVDevice(int iSampleRate);
  static bool loadValue(TJBox_ObjectRef const &iFromSocket, TJBox_Float64 &oValue);
  static bool storeValue(TJBox_Float64 iValue, TJBox_ObjectRef const &iToSocket);
  bool loadValue(TJBox_ObjectRef const &iFromSocket);
  bool storeValue(TJBox_ObjectRef const &iToSocket);

  template<typename From, typename To>
  static void wire(Rack &iRack, Rack::ExtensionDevice<From> const &iFromExtension, Rack::ExtensionDevice<To> const &iToExtension);
  template<typename From>
  static void wire(Rack &iRack, Rack::ExtensionDevice<From> const &iFromExtension, Rack::Extension::CVInSocket const &iToSocket);
  template<typename To>
  static void wire(Rack &iRack, Rack::Extension::CVOutSocket const &iFromSocket, Rack::ExtensionDevice<To> const &iToExtension);

  static bool eq(TJBox_Float64 iCV1, TJBox_Float64 iCV2);

public:
  int fSampleRate;
  TJBox_Float64 fValue{};
};

/**
 * CV source mock device. Copy `fValue` to output. */
class MCVSrc : public MockCVDevice
{
public:
  explicit MCVSrc(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MCVSrc> CONFIG;

protected:
  TJBox_ObjectRef fOutSocket{};
};

/**
 * CV destination mock device. Copy input to `fValue`. */
class MCVDst : public MockCVDevice
{
public:
  explicit MCVDst(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MCVDst> CONFIG;

public:
  std::vector<TJBox_Float64> fValues{};

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
class MockNotePlayer : public MockDevice
{
public:
  explicit MockNotePlayer(int iSampleRate);

  static void wire(Rack &iRack, Rack::Extension const &iFromExtension, Rack::Extension const &iToExtension);

  bool isBypassed() const;
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
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MNPSrc> CONFIG;
};

/**
 * Note player destination device. Extract note events from diffs and copy into fNoteEvents. */
class MNPDst : public MockNotePlayer
{
public:
  explicit MNPDst(int iSampleRate);
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
                           Rack::ExtensionDevice<From> const &iFromExtension,
                           Rack::ExtensionDevice<To> const &iToExtension)
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
                           Rack::ExtensionDevice<From> const &iFromExtension,
                           Rack::Extension::StereoAudioInSocket const &iToSockets)
{
  static_assert(std::is_convertible<From*, MockAudioDevice*>::value, "From must be a subclass of MockAudioDevice");
  iRack.wire(iFromExtension.getStereoAudioOutSocket(LEFT_SOCKET, RIGHT_SOCKET), iToSockets);
}

//------------------------------------------------------------------------
// MockAudioDevice::wire
//------------------------------------------------------------------------
template<typename To>
void MockAudioDevice::wire(Rack &iRack,
                           Rack::Extension::StereoAudioOutSocket const &iFromSockets,
                           Rack::ExtensionDevice<To> const &iToExtension)
{
  static_assert(std::is_convertible<To*, MockAudioDevice*>::value, "To must be a subclass of MockAudioDevice");
  iRack.wire(iFromSockets, iToExtension.getStereoAudioInSocket(LEFT_SOCKET, RIGHT_SOCKET));
}

//------------------------------------------------------------------------
// MockCVDevice::wire
//------------------------------------------------------------------------
template<typename From, typename To>
void MockCVDevice::wire(Rack &iRack,
                        Rack::ExtensionDevice<From> const &iFromExtension,
                        Rack::ExtensionDevice<To> const &iToExtension)
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
                        Rack::ExtensionDevice<From> const &iFromExtension,
                        Rack::Extension::CVInSocket const &iToSocket)
{
  static_assert(std::is_convertible<From*, MockCVDevice*>::value, "From must be a subclass of MockCVDevice");
  iRack.wire(iFromExtension.getCVOutSocket(SOCKET), iToSocket);
}

//------------------------------------------------------------------------
// MockCVDevice::wire
//------------------------------------------------------------------------
template<typename To>
void MockCVDevice::wire(Rack &iRack, Rack::Extension::CVOutSocket const &iFromSocket,
                        Rack::ExtensionDevice<To> const &iToExtension)
{
  static_assert(std::is_convertible<To*, MockCVDevice*>::value, "To must be a subclass of MockCVDevice");
  iRack.wire(iFromSocket, iToExtension.getCVInSocket(SOCKET));
}

}

#endif //__Pongasoft_re_mock_mock_devices_h__
