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
  void copyBuffer(StereoSocket const &iFromSocket, StereoBuffer &iToBuffer);
  void copyBuffer(StereoBuffer const &iFromBuffer, StereoSocket const &iToSocket);
  void copyBuffer(TJBox_ObjectRef const &iFromSocket, buffer_type &iToBuffer);
  void copyBuffer(buffer_type const &iFromBuffer, TJBox_ObjectRef const &iToSocket);

  static void wire(Rack &iRack, std::shared_ptr<Rack::Extension> iFromExtension, std::shared_ptr<Rack::Extension> iToExtension);

public:
  StereoBuffer fBuffer{};
  int fSampleRate;
};

/**
 * Audio source mock device. Copy `fBuffer` to output. */
class MAuSrc : public MockAudioDevice
{
public:
  explicit MAuSrc(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32);

  static const Rack::Extension::Configuration Config;

private:
  StereoSocket fOutSocket{};
};

/**
 * Audio destination mock device. Copy input to `fBuffer`. */
class MAuDst : public MockAudioDevice
{
public:
  explicit MAuDst(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32);

  static const Rack::Extension::Configuration Config;

private:
  StereoSocket fInSocket{};
};

/**
 * Audio pass through mock device: copy input to output. Keep a copy of the buffer in `fBuffer` */
class MAuPst : public MockAudioDevice
{
public:
  explicit MAuPst(int iSampleRate);
  void renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32);

  static const Rack::Extension::Configuration Config;

private:
  StereoSocket fInSocket{};
  StereoSocket fOutSocket{};
};

}

#endif //__PongasoftCommon_re_mock_mock_devices_h__
