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

template<typename Helper>
class HelperTester : public DeviceTester
{
public:
  explicit HelperTester(DeviceConfig<Helper> const &iDeviceConfig) :
    DeviceTester(iDeviceConfig.clone().device_type(DeviceType::kHelper).getConfig()) {}
  Rack::ExtensionDevice<Helper> getDevice() { return getExtensionDevice<Helper>(); }

  Helper *operator->() { return getDevice().operator->(); };
  Helper const *operator->() const { return getDevice().operator->(); };

  void nextFrame() { fRack.nextFrame(); }
};

class ExtensionEffectTester : public DeviceTester
{
public:
  explicit ExtensionEffectTester(Config const &iDeviceConfig);

  void setMainIn(std::string const &iLeftInSocketName, std::string const &iRightInSocketName);
  void setMainOut(std::string const &iLeftOutSocketName, std::string const &iRightOutSocketName);

  MockAudioDevice::StereoBuffer nextFrame(MockAudioDevice::StereoBuffer const &iInputBuffer);
  void nextFrame(MockAudioDevice::StereoBuffer const &iInputBuffer, MockAudioDevice::StereoBuffer &oOutputBuffer);

  TJBox_OnOffBypassStates getBypassState() const { return fDevice.getEffectBypassState(); }
  void setBypassState(TJBox_OnOffBypassStates iState) { fDevice.setEffectBypassState(iState); }

protected:
  Rack::ExtensionDevice<MAUSrc> fSrc;
  Rack::ExtensionDevice<MAUDst> fDst;
};

template<typename Effect>
class StudioEffectTester : public ExtensionEffectTester
{
public:
  explicit StudioEffectTester(DeviceConfig<Effect> const &iDeviceConfig) :
    ExtensionEffectTester(iDeviceConfig.clone().device_type(DeviceType::kStudioFX).getConfig()) {}
  Rack::ExtensionDevice<Effect> getDevice() { return getExtensionDevice<Effect>(); }

  Effect *operator->() { return getDevice().operator->(); };
  Effect const *operator->() const { return getDevice().operator->(); };
};

template<typename Effect>
class CreativeEffectTester : public ExtensionEffectTester
{
public:
  explicit CreativeEffectTester(DeviceConfig<Effect> const &iDeviceConfig) :
    ExtensionEffectTester(iDeviceConfig.clone().device_type(DeviceType::kCreativeFX).getConfig()) {}
  Rack::ExtensionDevice<Effect> getDevice() { return getExtensionDevice<Effect>(); }

  Effect *operator->() { return getDevice().operator->(); };
  Effect const *operator->() const { return getDevice().operator->(); };
};

class ExtensionInstrumentTester : public DeviceTester
{
public:
  explicit ExtensionInstrumentTester(Config const &iDeviceConfig);

  void setMainOut(std::string const &iLeftOutSocketName, std::string const &iRightOutSocketName);

  MockAudioDevice::StereoBuffer nextFrame(MockDevice::NoteEvents iNoteEvents = {});
  void nextFrame(MockDevice::NoteEvents iNoteEvents, MockAudioDevice::StereoBuffer &oOutputBuffer);
  void nextFrame(MockAudioDevice::StereoBuffer &oOutputBuffer);

protected:
  Rack::ExtensionDevice<MAUDst> fDst;
};

template<typename Instrument>
class InstrumentTester : public ExtensionInstrumentTester
{
public:
  explicit InstrumentTester(DeviceConfig<Instrument> const &iDeviceConfig) :
    ExtensionInstrumentTester(iDeviceConfig.clone().device_type(DeviceType::kInstrument).getConfig()) {}
  Rack::ExtensionDevice<Instrument> getDevice() { return getExtensionDevice<Instrument>(); }

  Instrument *operator->() { return getDevice().operator->(); };
  Instrument const *operator->() const { return getDevice().operator->(); };
};

class ExtensionNotePlayerTester : public DeviceTester
{
public:
  explicit ExtensionNotePlayerTester(Config const &iDeviceConfig);

  MockDevice::NoteEvents nextFrame(MockDevice::NoteEvents iSourceEvents = {});

  bool isBypassed() const { return fDevice.isNotePlayerBypassed(); }
  void setBypassed(bool iBypassed) { fDevice.setNotePlayerBypassed(iBypassed); }

protected:
  Rack::ExtensionDevice<MNPSrc> fSrc;
  Rack::ExtensionDevice<MNPDst> fDst;
};

template<typename NotePlayer>
class NotePlayerTester : public ExtensionNotePlayerTester
{
public:
  explicit NotePlayerTester(DeviceConfig<NotePlayer> const &iDeviceConfig) :
    ExtensionNotePlayerTester(iDeviceConfig.clone().device_type(DeviceType::kNotePlayer).getConfig()) {}
  Rack::ExtensionDevice<NotePlayer> getDevice() { return getExtensionDevice<NotePlayer>(); }

  NotePlayer *operator->() { return getDevice().operator->(); };
  NotePlayer const *operator->() const { return getDevice().operator->(); };
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