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

#include "Rack.h"
#include <array>

namespace re::mock {

class MockDevice
{
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
  constexpr static auto NUM_SAMPLES_PER_FRAME = 64;

  using buffer_type = std::array<TJBox_AudioSample, NUM_SAMPLES_PER_FRAME>;

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

  static StereoBuffer buffer(TJBox_AudioSample iLeftSample, TJBox_AudioSample iRightSample);

  static bool eq(TJBox_AudioSample iSample1, TJBox_AudioSample iSample2);
  static bool eq(StereoBuffer const &iBuffer1, StereoBuffer const &iBuffer2);
  static bool eq(buffer_type const &iBuffer1, buffer_type const &iBuffer2);

public:
  explicit MockAudioDevice(int iSampleRate);
  static void copyBuffer(StereoSocket const &iFromSocket, StereoBuffer &iToBuffer);
  static void copyBuffer(StereoBuffer const &iFromBuffer, StereoSocket const &iToSocket);
  static void copyBuffer(TJBox_ObjectRef const &iFromSocket, buffer_type &iToBuffer);
  static void copyBuffer(buffer_type const &iFromBuffer, TJBox_ObjectRef const &iToSocket);
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

protected:
  StereoSocket fOutSocket{};
};

/**
 * Audio destination mock device. Copy input to `fBuffer`. */
class MAUDst : public MockAudioDevice
{
public:
  explicit MAUDst(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff iPropertyDiffs[], TJBox_UInt32 iDiffCount) override;

  static const DeviceConfig<MAUDst> CONFIG;

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

protected:
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
  static void loadValue(TJBox_ObjectRef const &iFromSocket, TJBox_Float64 &oValue);
  static void storeValue(TJBox_Float64 iValue, TJBox_ObjectRef const &iToSocket);
  void loadValue(TJBox_ObjectRef const &iFromSocket);
  void storeValue(TJBox_ObjectRef const &iToSocket);

  template<typename From, typename To>
  static void wire(Rack &iRack, Rack::ExtensionDevice<From> const &iFromExtension, Rack::ExtensionDevice<To> const &iToExtension);
  template<typename From>
  static void wire(Rack &iRack, Rack::ExtensionDevice<From> const &iFromExtension, Rack::Extension::CVInSocket const &iToSocket);
  template<typename To>
  static void wire(Rack &iRack, Rack::Extension::CVInSocket const &iFromSocket, Rack::ExtensionDevice<To> const &iToExtension);

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
                        Rack::ExtensionDevice <From> const &iFromExtension,
                        Rack::Extension::CVInSocket const &iToSocket)
{
  static_assert(std::is_convertible<From*, MockCVDevice*>::value, "From must be a subclass of MockCVDevice");
  iRack.wire(iFromExtension.getCVOutSocket(SOCKET), iToSocket);
}

//------------------------------------------------------------------------
// MockCVDevice::wire
//------------------------------------------------------------------------
template<typename To>
void MockCVDevice::wire(Rack &iRack, Rack::Extension::CVInSocket const &iFromSocket,
                        Rack::ExtensionDevice <To> const &iToExtension)
{
  static_assert(std::is_convertible<To*, MockCVDevice*>::value, "To must be a subclass of MockCVDevice");
  iRack.wire(iFromSocket, iToExtension.getCVInSocket(SOCKET));
}

}

#endif //__Pongasoft_re_mock_mock_devices_h__
