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
#ifndef __Pongasoft_re_mock_effect_tester_h__
#define __Pongasoft_re_mock_effect_tester_h__

#include "Rack.h"
#include "MockDevices.h"

namespace re::mock {

class DeviceTester
{
public:
  explicit DeviceTester(Config const &iDeviceConfig);

  Rack &getRack() { return fRack; }
  Rack const &getRack() const { return fRack; }

  template<typename Device>
  Rack::ExtensionDevice<Device> getExtensionDevice();

protected:
  Rack fRack{};
  Rack::Extension fDevice;
};


class ExtensionEffectTester : public DeviceTester
{
public:
  explicit ExtensionEffectTester(Config const &iDeviceConfig);

  void setMainIn(std::string const &iLeftInSocketName, std::string const &iRightInSocketName);
  void setMainOut(std::string const &iLeftOutSocketName, std::string const &iRightOutSocketName);

  MockAudioDevice::StereoBuffer nextFrame(MockAudioDevice::StereoBuffer const &iInputBuffer);
  void nextFrame(MockAudioDevice::StereoBuffer const &iInputBuffer, MockAudioDevice::StereoBuffer &oOutputBuffer);

protected:
  Rack::ExtensionDevice<MAUSrc> fSrc;
  Rack::ExtensionDevice<MAUDst> fDst;
};

template<typename Effect>
class EffectTester : public ExtensionEffectTester
{
public:
  explicit EffectTester(Config const &iDeviceConfig) : ExtensionEffectTester(iDeviceConfig) {}
  Rack::ExtensionDevice<Effect> getEffect() { return getExtensionDevice<Effect>(); }
};

class ExtensionInstrumentTester : public DeviceTester
{
public:
  explicit ExtensionInstrumentTester(Config const &iDeviceConfig);

  void setMainOut(std::string const &iLeftOutSocketName, std::string const &iRightOutSocketName);

  MockAudioDevice::StereoBuffer nextFrame();
  void nextFrame(MockAudioDevice::StereoBuffer &oOutputBuffer);

protected:
  Rack::ExtensionDevice<MAUDst> fDst;
};

template<typename Instrument>
class InstrumentTester : public ExtensionInstrumentTester
{
public:
  explicit InstrumentTester(Config const &iDeviceConfig) : ExtensionInstrumentTester(iDeviceConfig) {}
  Rack::ExtensionDevice<Instrument> getInstrument() { return getExtensionDevice<Instrument>(); }
};


//------------------------------------------------------------------------
// DeviceTester::getExtensionDevice
//------------------------------------------------------------------------
template<typename Device>
Rack::ExtensionDevice<Device> DeviceTester::getExtensionDevice()
{
  return fRack.getDevice<Device>(fDevice.getInstanceId());
}


}

#endif //__Pongasoft_re_mock_effect_tester_h__