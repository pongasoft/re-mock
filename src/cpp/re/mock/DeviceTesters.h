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

struct Duration
{
  struct Time { float fMilliseconds{}; };
  struct SampleFrames { long fCount{}; };
  struct RackFrames { long fCount{}; };

  using Type = std::variant<Time, SampleFrames, RackFrames>;

  static RackFrames toRackFrames(Type iType, int iSampleRate);
  static SampleFrames toSampleFrames(Type iType, int iSampleRate);
};

/**
 * This class represents the base class for all the testers provided by this framework:
 *
 * - HelperTester
 * - StudioEffectTester
 * - CreativeEffectTester
 * - InstrumentTester
 * - NotePlayerTester
 *
 * Check the documentation for more details https://github.com/pongasoft/re-mock/blob/master/docs/Documentation.md
 */
class DeviceTester
{
public:
  explicit DeviceTester(Config const &iDeviceConfig, int iSampleRate);

  inline Rack &rack() { return fRack; }
  inline Rack const &rack() const { return fRack; }

  Rack::ExtensionDevice<MAUSrc> &wire(Rack::ExtensionDevice<MAUSrc> &iSrc,
                                      std::optional<std::string> iLeftInSocketName = std::nullopt,
                                      std::optional<std::string> iRightInSocketName = std::nullopt);

  Rack::ExtensionDevice<MAUSrc> wireNewAudioSrc(std::optional<std::string> iLeftInSocketName = std::nullopt,
                                                std::optional<std::string> iRightInSocketName = std::nullopt);

  void unwire(Rack::ExtensionDevice<MAUSrc> &iSrc);

  Rack::ExtensionDevice<MAUDst> &wire(Rack::ExtensionDevice<MAUDst> &iDst,
                                      std::optional<std::string> iLeftOutSocketName = std::nullopt,
                                      std::optional<std::string> iRightOutSocketName = std::nullopt);

  Rack::ExtensionDevice<MAUDst> wireNewAudioDst(std::optional<std::string> iLeftOutSocketName = std::nullopt,
                                                std::optional<std::string> iRightOutSocketName = std::nullopt);

  void unwire(Rack::ExtensionDevice<MAUDst> &iDst);

  Rack::ExtensionDevice<MCVSrc> &wire(Rack::ExtensionDevice<MCVSrc> &iSrc, std::string const &iCVInSocketName);

  Rack::ExtensionDevice<MCVSrc> wireNewCVSrc(std::optional<std::string> iCVInSocketName = std::nullopt);

  void unwire(Rack::ExtensionDevice<MCVSrc> &iSrc);

  Rack::ExtensionDevice<MCVDst> &wire(Rack::ExtensionDevice<MCVDst> &iDst, std::string const &iCVOutSocketName);

  Rack::ExtensionDevice<MCVDst> wireNewCVDst(std::optional<std::string> iCVOutSocketName = std::nullopt);

  void unwire(Rack::ExtensionDevice<MCVDst> &iDst);

  Rack::ExtensionDevice<MNPSrc> wireNewNotePlayerSrc();

  void unwire(Rack::ExtensionDevice<MNPSrc> &iSrc);

  Rack::ExtensionDevice<MNPDst> wireNewNotePlayerDst();

  void unwire(Rack::ExtensionDevice<MNPDst> &iDst);

  void nextFrames(Duration::Type iDuration);

  MockAudioDevice::Sample loadSample(ConfigFile const &iSampleFile) const;
  MockAudioDevice::Sample loadSample(std::string const &iSampleResource) const;

protected:
  template<typename Device>
  Rack::ExtensionDevice<Device> getExtensionDevice();

  Rack fRack;
  Rack::Extension fDevice;
  Config fDeviceConfig;
};

/**
 * Tester class to test a helper/utility (`device_type="helper"` in `info.lua`).
 *
 * Since utilities can be pretty much anything, this tester does not provide any kind of default devices. Use the
 * HelperTester::rack() method to access the rack and add/wire any other devices necessary for testing the helper.
 */
template<typename Helper>
class HelperTester : public DeviceTester
{
public:
  explicit HelperTester(DeviceConfig<Helper> const &iDeviceConfig, int iSampleRate = 44100) :
    DeviceTester(iDeviceConfig.getConfig(), iSampleRate),
    fHelper{getExtensionDevice<Helper>()}
    {
      RE_MOCK_ASSERT(iDeviceConfig.getConfig().info().fDeviceType == DeviceType::kHelper);
    }

  inline Rack::ExtensionDevice<Helper> &device() { return fHelper; }
  inline Rack::ExtensionDevice<Helper> const &device() const { return fHelper; }

  void nextFrame() { fRack.nextFrame(); }

protected:
  Rack::ExtensionDevice<Helper> fHelper;
};

class ExtensionEffectTester : public DeviceTester
{
public:
  using optional_duration_t = std::optional<Duration::Type>;
  using before_frame_hook_t = std::function<void(int)>;

public:
  explicit ExtensionEffectTester(Config const &iDeviceConfig, int iSampleRate);

  void wireMainIn(std::optional<std::string> iLeftInSocketName, std::optional<std::string> iRightInSocketName);
  void wireMainOut(std::optional<std::string> iLeftOutSocketName, std::optional<std::string> iRightOutSocketName);

  MockAudioDevice::StereoBuffer nextFrame(MockAudioDevice::StereoBuffer const &iInputBuffer);
  void nextFrame(MockAudioDevice::StereoBuffer const &iInputBuffer, MockAudioDevice::StereoBuffer &oOutputBuffer);

  MockAudioDevice::Sample processSample(MockAudioDevice::Sample const &iSample,
                                        optional_duration_t iTail = std::nullopt,
                                        before_frame_hook_t iBeforeFrameHook = {});

  MockAudioDevice::Sample processSample(ConfigFile const &iSampleFile,
                                        optional_duration_t iTail = std::nullopt,
                                        before_frame_hook_t iBeforeFrameHook = {}) {
    return processSample(loadSample(iSampleFile), iTail, std::move(iBeforeFrameHook));
  }
  MockAudioDevice::Sample processSample(std::string const &iSampleResource,
                                        optional_duration_t iTail = std::nullopt,
                                        before_frame_hook_t iBeforeFrameHook = {}) {
    return processSample(loadSample(iSampleResource), iTail, std::move(iBeforeFrameHook));
  }

  TJBox_OnOffBypassStates getBypassState() const { return fDevice.getEffectBypassState(); }
  void setBypassState(TJBox_OnOffBypassStates iState) { fDevice.setEffectBypassState(iState); }

  inline Rack::ExtensionDevice<MAUSrc> &src() { return fSrc; }
  inline Rack::ExtensionDevice<MAUSrc> const &src() const { return fSrc; }

  inline Rack::ExtensionDevice<MAUDst> &dst() { return fDst; }
  inline Rack::ExtensionDevice<MAUDst> const &dst() const { return fDst; }

protected:
  Rack::ExtensionDevice<MAUSrc> fSrc;
  Rack::ExtensionDevice<MAUDst> fDst;
};

template<typename Effect>
class StudioEffectTester : public ExtensionEffectTester
{
public:
  explicit StudioEffectTester(DeviceConfig<Effect> const &iDeviceConfig, int iSampleRate = 44100) :
    ExtensionEffectTester(iDeviceConfig.getConfig(), iSampleRate),
    fEffect{getExtensionDevice<Effect>()}
  {
    RE_MOCK_ASSERT(iDeviceConfig.getConfig().info().fDeviceType == DeviceType::kStudioFX);
  }

  inline Rack::ExtensionDevice<Effect> &device() { return fEffect; }
  inline Rack::ExtensionDevice<Effect> const &device() const { return fEffect; }

protected:
  Rack::ExtensionDevice<Effect> fEffect;
};

template<typename Effect>
class CreativeEffectTester : public ExtensionEffectTester
{
public:
  explicit CreativeEffectTester(DeviceConfig<Effect> const &iDeviceConfig, int iSampleRate = 44100) :
    ExtensionEffectTester(iDeviceConfig.getConfig(), iSampleRate),
    fEffect{getExtensionDevice<Effect>()}
  {
    RE_MOCK_ASSERT(iDeviceConfig.getConfig().info().fDeviceType == DeviceType::kCreativeFX);
  }

  inline Rack::ExtensionDevice<Effect> &device() { return fEffect; }
  inline Rack::ExtensionDevice<Effect> const &device() const { return fEffect; }

protected:
  Rack::ExtensionDevice<Effect> fEffect;
};

class ExtensionInstrumentTester : public DeviceTester
{
public:
  using before_frame_hook_t = std::function<void(int)>;

public:
  explicit ExtensionInstrumentTester(Config const &iDeviceConfig, int iSampleRate);

  void wireMainOut(std::optional<std::string> iLeftOutSocketName, std::optional<std::string> iRightOutSocketName);

  ExtensionInstrumentTester &setNoteEvents(MockDevice::NoteEvents iNoteEvents);

  MockAudioDevice::StereoBuffer nextFrame(MockDevice::NoteEvents iNoteEvents = {});
  void nextFrame(MockDevice::NoteEvents iNoteEvents, MockAudioDevice::StereoBuffer &oOutputBuffer);
  void nextFrame(MockAudioDevice::StereoBuffer &oOutputBuffer);

  MockAudioDevice::Sample play(Duration::Type iDuration, before_frame_hook_t iBeforeFrameHook = {});

  inline Rack::ExtensionDevice<MAUDst> &dst() { return fDst; }
  inline Rack::ExtensionDevice<MAUDst> const &dst() const { return fDst; }

protected:
  Rack::ExtensionDevice<MAUDst> fDst;
};

template<typename Instrument>
class InstrumentTester : public ExtensionInstrumentTester
{
public:
  explicit InstrumentTester(DeviceConfig<Instrument> const &iDeviceConfig, int iSampleRate = 44100) :
    ExtensionInstrumentTester(iDeviceConfig.getConfig(), iSampleRate),
    fInstrument{getExtensionDevice<Instrument>()}
  {}

  inline Rack::ExtensionDevice<Instrument> &device() { return fInstrument; }
  inline Rack::ExtensionDevice<Instrument> const &device() const { return fInstrument; }

protected:
  Rack::ExtensionDevice<Instrument> fInstrument;
};

class ExtensionNotePlayerTester : public DeviceTester
{
public:
  explicit ExtensionNotePlayerTester(Config const &iDeviceConfig, int iSampleRate);

  MockDevice::NoteEvents nextFrame(MockDevice::NoteEvents iSourceEvents = {});

  bool isBypassed() const { return fDevice.isNotePlayerBypassed(); }
  void setBypassed(bool iBypassed) { fDevice.setNotePlayerBypassed(iBypassed); }

  inline Rack::ExtensionDevice<MNPSrc> &src() { return fSrc; }
  inline Rack::ExtensionDevice<MNPSrc> const &src() const { return fSrc; }

  inline Rack::ExtensionDevice<MNPDst> &dst() { return fDst; }
  inline Rack::ExtensionDevice<MNPDst> const &dst() const { return fDst; }

protected:
  Rack::ExtensionDevice<MNPSrc> fSrc;
  Rack::ExtensionDevice<MNPDst> fDst;
};

template<typename NotePlayer>
class NotePlayerTester : public ExtensionNotePlayerTester
{
public:
  explicit NotePlayerTester(DeviceConfig<NotePlayer> const &iDeviceConfig, int iSampleRate = 44100) :
    ExtensionNotePlayerTester(iDeviceConfig.getConfig(), iSampleRate),
    fNotePlayer{getExtensionDevice<NotePlayer>()}
  {}

  inline Rack::ExtensionDevice<NotePlayer> &device() { return fNotePlayer; }
  inline Rack::ExtensionDevice<NotePlayer> const &device() const { return fNotePlayer; }

protected:
  Rack::ExtensionDevice<NotePlayer> fNotePlayer;
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