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
DeviceTester::DeviceTester(Config const &iDeviceConfig, int iSampleRate) :
  fRack{iSampleRate}, fDevice{fRack.newExtension(iDeviceConfig)}
{
}

//------------------------------------------------------------------------
// DeviceTester::wire
//------------------------------------------------------------------------
Rack::ExtensionDevice<MAUSrc> &DeviceTester::wire(Rack::ExtensionDevice<MAUSrc> &iSrc,
                                                  std::optional<std::string> iLeftInSocketName,
                                                  std::optional<std::string> iRightInSocketName)
{
  if(iLeftInSocketName)
    fRack.wire(iSrc.getAudioOutSocket(MAUSrc::LEFT_SOCKET), fDevice.getAudioInSocket(iLeftInSocketName.value()));
  if(iRightInSocketName)
    fRack.wire(iSrc.getAudioOutSocket(MAUSrc::RIGHT_SOCKET), fDevice.getAudioInSocket(iRightInSocketName.value()));

  return iSrc;
}


//------------------------------------------------------------------------
// DeviceTester::wireNewAudioSrc
//------------------------------------------------------------------------
Rack::ExtensionDevice<MAUSrc> DeviceTester::wireNewAudioSrc(std::optional<std::string> iLeftInSocketName,
                                                            std::optional<std::string> iRightInSocketName)
{
  auto src = fRack.newDevice<MAUSrc>(MAUSrc::CONFIG);

  return wire(src, iLeftInSocketName, iRightInSocketName);
}

//------------------------------------------------------------------------
// DeviceTester::unwire
//------------------------------------------------------------------------
void DeviceTester::unwire(Rack::ExtensionDevice<MAUSrc> &iSrc)
{
  fRack.unwire(iSrc.getStereoAudioOutSocket(MAUSrc::LEFT_SOCKET, MAUSrc::RIGHT_SOCKET));
}

//------------------------------------------------------------------------
// DeviceTester::wire
//------------------------------------------------------------------------
Rack::ExtensionDevice<MAUDst> &
DeviceTester::wire(Rack::ExtensionDevice<MAUDst> &iDst,
                   std::optional<std::string> iLeftOutSocketName,
                   std::optional<std::string> iRightOutSocketName)
{
  if(iLeftOutSocketName)
    fRack.wire(fDevice.getAudioOutSocket(iLeftOutSocketName.value()), iDst.getAudioInSocket(MAUDst::LEFT_SOCKET));
  if(iRightOutSocketName)
    fRack.wire(fDevice.getAudioOutSocket(iRightOutSocketName.value()), iDst.getAudioInSocket(MAUDst::RIGHT_SOCKET));

  return iDst;
}

//------------------------------------------------------------------------
// DeviceTester::wireNewAudioDst
//------------------------------------------------------------------------
Rack::ExtensionDevice<MAUDst> DeviceTester::wireNewAudioDst(std::optional<std::string> iLeftOutSocketName,
                                                            std::optional<std::string> iRightOutSocketName)
{
  auto dst = fRack.newDevice<MAUDst>(MAUDst::CONFIG);
  wire(dst, iLeftOutSocketName, iRightOutSocketName);
  return dst;
}

//------------------------------------------------------------------------
// DeviceTester::unwire
//------------------------------------------------------------------------
void DeviceTester::unwire(Rack::ExtensionDevice<MAUDst> &iDst)
{
  fRack.unwire(iDst.getAudioInSocket(MAUDst::LEFT_SOCKET));
  fRack.unwire(iDst.getAudioInSocket(MAUDst::RIGHT_SOCKET));
}

//------------------------------------------------------------------------
// DeviceTester::wire
//------------------------------------------------------------------------
Rack::ExtensionDevice<MCVSrc> &
DeviceTester::wire(Rack::ExtensionDevice<MCVSrc> &iSrc, std::string const &iCVInSocketName)
{
  fRack.wire(iSrc.getCVOutSocket(MCVSrc::SOCKET), fDevice.getCVInSocket(iCVInSocketName));
  return iSrc;
}

//------------------------------------------------------------------------
// DeviceTester::wireNewCVSrc
//------------------------------------------------------------------------
Rack::ExtensionDevice<MCVSrc> DeviceTester::wireNewCVSrc(std::optional<std::string> iCVInSocketName)
{
  auto src = fRack.newDevice<MCVSrc>(MCVSrc::CONFIG);
  if(iCVInSocketName)
    wire(src, iCVInSocketName.value());
  return src;
}

//------------------------------------------------------------------------
// DeviceTester::unwire
//------------------------------------------------------------------------
void DeviceTester::unwire(Rack::ExtensionDevice<MCVSrc> &iSrc)
{
  fRack.unwire(iSrc.getCVOutSocket(MCVSrc::SOCKET));
}

//------------------------------------------------------------------------
// DeviceTester::wire
//------------------------------------------------------------------------
Rack::ExtensionDevice<MCVDst> &
DeviceTester::wire(Rack::ExtensionDevice<MCVDst> &iDst, std::string const &iCVOutSocketName)
{
  fRack.wire(fDevice.getCVOutSocket(iCVOutSocketName), iDst.getCVInSocket(MCVDst::SOCKET));
  return iDst;
}

//------------------------------------------------------------------------
// DeviceTester::wireNewCVDst
//------------------------------------------------------------------------
Rack::ExtensionDevice<MCVDst> DeviceTester::wireNewCVDst(std::optional<std::string> iCVOutSocketName)
{
  auto dst = fRack.newDevice<MCVDst>(MCVDst::CONFIG);
  if(iCVOutSocketName)
    wire(dst, iCVOutSocketName.value());
  return dst;
}

//------------------------------------------------------------------------
// DeviceTester::unwire
//------------------------------------------------------------------------
void DeviceTester::unwire(Rack::ExtensionDevice<MCVDst> &iDst)
{
  fRack.unwire(iDst.getCVInSocket(MCVSrc::SOCKET));
}

//------------------------------------------------------------------------
// DeviceTester::wireNewNotePlayerSrc
//------------------------------------------------------------------------
Rack::ExtensionDevice<MNPSrc> DeviceTester::wireNewNotePlayerSrc()
{
  auto src = fRack.newDevice<MNPSrc>(MNPSrc::CONFIG);
  fRack.wire(src.getNoteOutSocket(), fDevice.getNoteInSocket());
  return src;
}

//------------------------------------------------------------------------
// DeviceTester::unwire
//------------------------------------------------------------------------
void DeviceTester::unwire(Rack::ExtensionDevice<MNPSrc> &iSrc)
{
  fRack.unwire(iSrc.getNoteOutSocket());
}

//------------------------------------------------------------------------
// DeviceTester::wireNewNotePlayerDst
//------------------------------------------------------------------------
Rack::ExtensionDevice<MNPDst> DeviceTester::wireNewNotePlayerDst()
{
  auto dst = fRack.newDevice<MNPDst>(MNPDst::CONFIG);
  fRack.wire(fDevice.getNoteOutSocket(), dst.getNoteInSocket());
  return dst;
}

//------------------------------------------------------------------------
// DeviceTester::unwire
//------------------------------------------------------------------------
void DeviceTester::unwire(Rack::ExtensionDevice<MNPDst> &iDst)
{
  fRack.unwire(iDst.getNoteInSocket());
}

//------------------------------------------------------------------------
// ExtensionEffectTester::ExtensionEffectTester
//------------------------------------------------------------------------
ExtensionEffectTester::ExtensionEffectTester(Config const &iDeviceConfig, int iSampleRate) :
  DeviceTester(iDeviceConfig, iSampleRate),
  fSrc{fRack.newDevice<MAUSrc>(MAUSrc::CONFIG)},
  fDst{fRack.newDevice<MAUDst>(MAUDst::CONFIG)}
{
}

//------------------------------------------------------------------------
// ExtensionEffectTester::wireMainIn
//------------------------------------------------------------------------
void ExtensionEffectTester::wireMainIn(std::optional<std::string> iLeftInSocketName,
                                       std::optional<std::string> iRightInSocketName)
{
  wire(fSrc, iLeftInSocketName, iRightInSocketName);
}


//------------------------------------------------------------------------
// ExtensionEffectTester::wireMainOut
//------------------------------------------------------------------------
void ExtensionEffectTester::wireMainOut(std::optional<std::string> iLeftOutSocketName,
                                        std::optional<std::string> iRightOutSocketName)
{
  wire(fDst, iLeftOutSocketName, iRightOutSocketName);
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
ExtensionInstrumentTester::ExtensionInstrumentTester(Config const &iDeviceConfig, int iSampleRate) :
  DeviceTester(iDeviceConfig, iSampleRate),
  fDst{fRack.newDevice<MAUDst>(MAUDst::CONFIG)}
{
}

//------------------------------------------------------------------------
// ExtensionEffectTester::wireMainOut
//------------------------------------------------------------------------
void ExtensionInstrumentTester::wireMainOut(std::optional<std::string> iLeftOutSocketName,
                                            std::optional<std::string> iRightOutSocketName)
{
  wire(fDst, iLeftOutSocketName, iRightOutSocketName);
}

//------------------------------------------------------------------------
// ExtensionInstrumentTester::nextFrame
//------------------------------------------------------------------------
void ExtensionInstrumentTester::nextFrame(MockAudioDevice::StereoBuffer &oOutputBuffer)
{
  nextFrame({}, oOutputBuffer);
}

//------------------------------------------------------------------------
// ExtensionInstrumentTester::nextFrame
//------------------------------------------------------------------------
MockAudioDevice::StereoBuffer ExtensionInstrumentTester::nextFrame(MockDevice::NoteEvents iNoteEvents)
{
  MockAudioDevice::StereoBuffer output{};
  nextFrame(iNoteEvents, output);
  return output;
}

//------------------------------------------------------------------------
// ExtensionInstrumentTester::nextFrame
//------------------------------------------------------------------------
void ExtensionInstrumentTester::nextFrame(MockDevice::NoteEvents iNoteEvents,
                                          MockAudioDevice::StereoBuffer &oOutputBuffer)
{
  fDevice.setNoteInEvents(iNoteEvents.events());
  fRack.nextFrame();
  oOutputBuffer = fDst->fBuffer;
}

//------------------------------------------------------------------------
// ExtensionNotePlayerTester::ExtensionNotePlayerTester
//------------------------------------------------------------------------
ExtensionNotePlayerTester::ExtensionNotePlayerTester(Config const &iDeviceConfig, int iSampleRate) :
  DeviceTester(iDeviceConfig, iSampleRate),
  fSrc{wireNewNotePlayerSrc()},
  fDst{wireNewNotePlayerDst()}
{
}

//------------------------------------------------------------------------
// ExtensionNotePlayerTester::nextFrame
//------------------------------------------------------------------------
MockDevice::NoteEvents ExtensionNotePlayerTester::nextFrame(MockDevice::NoteEvents iSourceEvents)
{
  fSrc->fNoteEvents = std::move(iSourceEvents);
  fRack.nextFrame();
  return fDst->fNoteEvents;
}

}