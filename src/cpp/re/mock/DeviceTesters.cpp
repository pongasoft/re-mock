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

#include "DeviceTesters.h"

namespace re::mock {

//------------------------------------------------------------------------
// DeviceTester::DeviceTester
//------------------------------------------------------------------------
DeviceTester::DeviceTester(Config const &iDeviceConfig) : fDevice{fRack.newExtension(iDeviceConfig)}
{
}

//------------------------------------------------------------------------
// ExtensionEffectTester::ExtensionEffectTester
//------------------------------------------------------------------------
ExtensionEffectTester::ExtensionEffectTester(Config const &iDeviceConfig) :
  DeviceTester(iDeviceConfig),
  fSrc{fRack.newDevice<MAUSrc>(MAUSrc::Config)},
  fDst{fRack.newDevice<MAUDst>(MAUDst::Config)}
{
}

//------------------------------------------------------------------------
// ExtensionEffectTester::setMainIn
//------------------------------------------------------------------------
void ExtensionEffectTester::setMainIn(std::string const &iLeftInSocketName, std::string const &iRightInSocketName)
{
  MockAudioDevice::wire(fRack, fSrc, fDevice.getStereoAudioInSocket(iLeftInSocketName, iRightInSocketName));
}

//------------------------------------------------------------------------
// ExtensionEffectTester::setMainOut
//------------------------------------------------------------------------
void ExtensionEffectTester::setMainOut(std::string const &iLeftOutSocketName, std::string const &iRightOutSocketName)
{
  MockAudioDevice::wire(fRack, fDevice.getStereoAudioOutSocket(iLeftOutSocketName, iRightOutSocketName), fDst);
}

//------------------------------------------------------------------------
// ExtensionEffectTester::nextFrame
//------------------------------------------------------------------------
MockAudioDevice::StereoBuffer ExtensionEffectTester::nextFrame(MockAudioDevice::StereoBuffer const &iInputBuffer)
{
  MockAudioDevice::StereoBuffer output{};
  nextFrame(iInputBuffer, output);
  return output;
}

//------------------------------------------------------------------------
// ExtensionEffectTester::nextFrame
//------------------------------------------------------------------------
void ExtensionEffectTester::nextFrame(MockAudioDevice::StereoBuffer const &iInputBuffer,
                                      MockAudioDevice::StereoBuffer &oOutputBuffer)
{
  fSrc->fBuffer = iInputBuffer;
  fRack.nextFrame();
  oOutputBuffer = fDst->fBuffer;
}

//------------------------------------------------------------------------
// ExtensionInstrumentTester::ExtensionInstrumentTester
//------------------------------------------------------------------------
ExtensionInstrumentTester::ExtensionInstrumentTester(Config const &iDeviceConfig) :
  DeviceTester(iDeviceConfig),
  fDst{fRack.newDevice<MAUDst>(MAUDst::Config)}
{
}

//------------------------------------------------------------------------
// ExtensionEffectTester::setMainOut
//------------------------------------------------------------------------
void ExtensionInstrumentTester::setMainOut(std::string const &iLeftOutSocketName, std::string const &iRightOutSocketName)
{
  MockAudioDevice::wire(fRack, fDevice.getStereoAudioOutSocket(iLeftOutSocketName, iRightOutSocketName), fDst);
}

//------------------------------------------------------------------------
// ExtensionInstrumentTester::nextFrame
//------------------------------------------------------------------------
MockAudioDevice::StereoBuffer ExtensionInstrumentTester::nextFrame()
{
  MockAudioDevice::StereoBuffer output{};
  nextFrame(output);
  return output;
}

//------------------------------------------------------------------------
// ExtensionInstrumentTester::nextFrame
//------------------------------------------------------------------------
void ExtensionInstrumentTester::nextFrame(MockAudioDevice::StereoBuffer &oOutputBuffer)
{
  fRack.nextFrame();
  oOutputBuffer = fDst->fBuffer;
}


}