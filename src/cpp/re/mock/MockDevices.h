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
#ifndef __PongasoftCommon_re_mock_mock_devices_h__
#define __PongasoftCommon_re_mock_mock_devices_h__

#include "Rack.h"
#include <array>

namespace re::mock {

//------------------------------------------------------------------------
// Audio Devices
//------------------------------------------------------------------------

class MockAudioDevice
{
public:
  constexpr static auto LEFT_SOCKET = "L";
  constexpr static auto RIGHT_SOCKET = "R";

  using buffer_type = std::array<TJBox_AudioSample, 64>;

  struct StereoBuffer
  {
    buffer_type fLeft{};
    buffer_type fRight{};

    void fill(TJBox_AudioSample iLeftSample, TJBox_AudioSample iRightSample);
    bool check(TJBox_AudioSample iLeftSample, TJBox_AudioSample iRightSample) const;
  };

  struct StereoSocket
  {
    TJBox_ObjectRef fLeft{};
    TJBox_ObjectRef fRight{};

    static StereoSocket input();
    static StereoSocket output();
  };

public:
  explicit MockAudioDevice(int iSampleRate);
  virtual ~MockAudioDevice() = default; // allow for subclassing
  static void copyBuffer(StereoSocket const &iFromSocket, StereoBuffer &iToBuffer);
  static void copyBuffer(StereoBuffer const &iFromBuffer, StereoSocket const &iToSocket);
  static void copyBuffer(TJBox_ObjectRef const &iFromSocket, buffer_type &iToBuffer);
  static void copyBuffer(buffer_type const &iFromBuffer, TJBox_ObjectRef const &iToSocket);
  static void copyBuffer(StereoBuffer const &iFromBuffer, StereoBuffer &iToBuffer);
  static void copyBuffer(buffer_type const &iFromBuffer, buffer_type &iToBuffer);

  static void wire(Rack &iRack, std::shared_ptr<Rack::Extension> iFromExtension, std::shared_ptr<Rack::Extension> iToExtension);

public:
  int fSampleRate;
  StereoBuffer fBuffer{};
};

/**
 * Audio source mock device. Copy `fBuffer` to output. */
class MAUSrc : public MockAudioDevice
{
public:
  explicit MAUSrc(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32);

  static const Rack::Extension::Configuration Config;

private:
  StereoSocket fOutSocket{};
};

/**
 * Audio destination mock device. Copy input to `fBuffer`. */
class MAUDst : public MockAudioDevice
{
public:
  explicit MAUDst(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32);

  static const Rack::Extension::Configuration Config;

private:
  StereoSocket fInSocket{};
};

/**
 * Audio pass through mock device: copy input to output. Keep a copy of the buffer in `fBuffer` */
class MAUPst : public MockAudioDevice
{
public:
  explicit MAUPst(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32);

  static const Rack::Extension::Configuration Config;

private:
  StereoSocket fInSocket{};
  StereoSocket fOutSocket{};
};

//------------------------------------------------------------------------
// CV Devices
//------------------------------------------------------------------------

class MockCVDevice
{
public:
  constexpr static auto SOCKET = "C";

  using value_type = TJBox_Float64; // cv values are float64 numbers

public:
  explicit MockCVDevice(int iSampleRate);
  virtual ~MockCVDevice() = default; // allow for subclassing
  static void loadValue(TJBox_ObjectRef const &iFromSocket, TJBox_Float64 &oValue);
  static void storeValue(TJBox_Float64 iValue, TJBox_ObjectRef const &iToSocket);
  void loadValue(TJBox_ObjectRef const &iFromSocket);
  void storeValue(TJBox_ObjectRef const &iToSocket);

  static void wire(Rack &iRack, std::shared_ptr<Rack::Extension> iFromExtension, std::shared_ptr<Rack::Extension> iToExtension);

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
  void renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32);

  static const Rack::Extension::Configuration Config;

private:
  TJBox_ObjectRef fOutSocket{};
};

/**
 * CV destination mock device. Copy input to `fValue`. */
class MCVDst : public MockCVDevice
{
public:
  explicit MCVDst(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32);

  static const Rack::Extension::Configuration Config;

private:
  TJBox_ObjectRef fInSocket{};
};

/**
 * CV pass through mock device: copy input to output. Keep a copy of the value in `fValue` */
class MCVPst : public MockCVDevice
{
public:
  explicit MCVPst(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32);

  static const Rack::Extension::Configuration Config;

private:
  TJBox_ObjectRef fInSocket{};
  TJBox_ObjectRef fOutSocket{};
};

}

#endif //__PongasoftCommon_re_mock_mock_devices_h__
