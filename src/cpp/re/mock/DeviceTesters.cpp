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
#include "FileManager.h"

namespace re::mock {

//------------------------------------------------------------------------
// DeviceTester::DeviceTester
//------------------------------------------------------------------------
DeviceTester::DeviceTester(Config const &iDeviceConfig, int iSampleRate) :
  fRack{iSampleRate}, fDevice{fRack.newExtension(iDeviceConfig)}, fDeviceConfig{iDeviceConfig}
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
// DeviceTester::nextFrames
//------------------------------------------------------------------------
void DeviceTester::nextFrames(Duration::Type iDuration)
{
  auto numFrames = Duration::toRackFrames(iDuration, fRack.getSampleRate());

  for(int i = 0; i < numFrames.fCount; i++)
    fRack.nextFrame();
}

//------------------------------------------------------------------------
// DeviceTester::loadSample
//------------------------------------------------------------------------
MockAudioDevice::Sample DeviceTester::loadSample(ConfigFile const &iSampleFile) const
{
  auto sample = FileManager::loadSample(iSampleFile);
  RE_MOCK_ASSERT(sample != std::nullopt, "Could not load sample [%s]", iSampleFile.fFilename);
  return MockAudioDevice::Sample::from(*sample);
}

//------------------------------------------------------------------------
// DeviceTester::loadSample
//------------------------------------------------------------------------
MockAudioDevice::Sample DeviceTester::loadSample(std::string const &iSampleResource) const
{
  auto sample = fDeviceConfig.findSampleResource(iSampleResource);
  RE_MOCK_ASSERT(sample != std::nullopt, "Could not load sample resource [%s]", iSampleResource);
  return MockAudioDevice::Sample::from(*sample);
}

//------------------------------------------------------------------------
// ExtensionEffectTester::ExtensionEffectTester
//------------------------------------------------------------------------
ExtensionEffectTester::ExtensionEffectTester(Config const &iDeviceConfig, int iSampleRate) :
  DeviceTester(iDeviceConfig, iSampleRate),
  fSrc{fRack.newDevice<MAUSrc>(MAUSrc::CONFIG)},
  fDst{fRack.newDevice<MAUDst>(MAUDst::CONFIG)}
{
  RE_MOCK_ASSERT(iDeviceConfig.info().fDeviceType == DeviceType::kCreativeFX ||
                 iDeviceConfig.info().fDeviceType == DeviceType::kStudioFX);
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
// ExtensionEffectTester::processSample
//------------------------------------------------------------------------
MockAudioDevice::Sample ExtensionEffectTester::processSample(MockAudioDevice::Sample const &iSample,
                                                             optional_duration_t iTail,
                                                             before_frame_hook_t iBeforeFrameHook)
{
  size_t tailInSampleFrames = 0;

  if(iTail)
    tailInSampleFrames = Duration::toSampleFrames(*iTail, fRack.getSampleRate()).fCount;

  MockAudioDevice::Sample res{};
  res.fChannels = iSample.fChannels;
  res.fSampleRate = fRack.getSampleRate();
  res.fData.reserve(iSample.fData.size() + tailInSampleFrames);

  MockAudioDevice::StereoBuffer input{};
  MockAudioDevice::StereoBuffer output{};

  auto numSampleFramesToProcess = iSample.fData.size() / iSample.fChannels;

  auto totalNumSampleFrames = numSampleFramesToProcess + tailInSampleFrames;
  auto ptr = iSample.fData.data();

  int frameCount = 0;

  while(totalNumSampleFrames > 0)
  {
    auto numSamplesInThisRackFrame = std::min<size_t>(totalNumSampleFrames, MockAudioDevice::NUM_SAMPLES_PER_FRAME);
    auto numSamplesToProcessInThisRackFrame = std::min<size_t>(numSampleFramesToProcess, numSamplesInThisRackFrame);

    if(numSamplesToProcessInThisRackFrame > 0)
    {
      if(numSamplesToProcessInThisRackFrame < MockAudioDevice::NUM_SAMPLES_PER_FRAME)
        input.fill(0, 0);

      // fill the input buffer
      for(size_t i = 0; i < numSamplesToProcessInThisRackFrame; i++)
      {
        input.fLeft[i] = *ptr++;
        if(iSample.fChannels == 2)
          input.fRight[i] = *ptr++;
      }
    }
    else
      input.fill(0, 0);

    // allow outside code to modify the device prior to invoking nextFrame
    if(iBeforeFrameHook)
      iBeforeFrameHook(frameCount);

    // process this rack frame
    nextFrame(input, output);

    // fill the output buffer
    for(size_t i = 0; i < numSamplesInThisRackFrame; i++)
    {
      res.fData.emplace_back(output.fLeft[i]);
      if(iSample.fChannels == 2)
        res.fData.emplace_back(output.fRight[i]);
    }

    totalNumSampleFrames -= numSamplesInThisRackFrame;
    numSampleFramesToProcess -= numSamplesToProcessInThisRackFrame;
    frameCount++;
  }

  return res;
}

//------------------------------------------------------------------------
// ExtensionInstrumentTester::ExtensionInstrumentTester
//------------------------------------------------------------------------
ExtensionInstrumentTester::ExtensionInstrumentTester(Config const &iDeviceConfig, int iSampleRate) :
  DeviceTester(iDeviceConfig, iSampleRate),
  fDst{fRack.newDevice<MAUDst>(MAUDst::CONFIG)}
{
  RE_MOCK_ASSERT(iDeviceConfig.info().fDeviceType == DeviceType::kInstrument);
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
// ExtensionEffectTester::setNoteEvents
//------------------------------------------------------------------------
ExtensionInstrumentTester & ExtensionInstrumentTester::setNoteEvents(MockDevice::NoteEvents iNoteEvents)
{
  fDevice.setNoteInEvents(iNoteEvents.events());
  return *this;
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
  setNoteEvents(iNoteEvents);
  fRack.nextFrame();
  oOutputBuffer = fDst->fBuffer;
}

//------------------------------------------------------------------------
// ExtensionInstrumentTester::play
//------------------------------------------------------------------------
MockAudioDevice::Sample ExtensionInstrumentTester::play(Duration::Type iDuration, before_frame_hook_t iBeforeFrameHook)
{
  auto totalNumSampleFrames = Duration::toSampleFrames(iDuration, fRack.getSampleRate()).fCount;

  MockAudioDevice::Sample res{};
  res.fChannels = 2;
  res.fSampleRate = fRack.getSampleRate();
  res.fData.reserve(totalNumSampleFrames);

  int frameCount = 0;

  while(totalNumSampleFrames > 0)
  {
    // allow outside code to modify the device prior to invoking nextFrame
    if(iBeforeFrameHook)
      iBeforeFrameHook(frameCount);

    fRack.nextFrame();

    auto numSamplesInThisRackFrame = std::min<size_t>(totalNumSampleFrames, MockAudioDevice::NUM_SAMPLES_PER_FRAME);

    res.append(fDst->fBuffer, numSamplesInThisRackFrame);

    totalNumSampleFrames -= numSamplesInThisRackFrame;
    frameCount++;
  }

  return res;
}

//------------------------------------------------------------------------
// ExtensionNotePlayerTester::ExtensionNotePlayerTester
//------------------------------------------------------------------------
ExtensionNotePlayerTester::ExtensionNotePlayerTester(Config const &iDeviceConfig, int iSampleRate) :
  DeviceTester(iDeviceConfig, iSampleRate),
  fSrc{wireNewNotePlayerSrc()},
  fDst{wireNewNotePlayerDst()}
{
  RE_MOCK_ASSERT(iDeviceConfig.info().fDeviceType == DeviceType::kNotePlayer);
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

//------------------------------------------------------------------------
// Duration::toRackFrames
//------------------------------------------------------------------------
Duration::RackFrames Duration::toRackFrames(Duration::Type iType, int iSampleRate)
{
  struct visitor
  {
    RackFrames operator()(Duration::RackFrames d) { return d; }

    RackFrames operator()(Duration::Time d) {
      auto frames = std::ceil(d.fMilliseconds * fSampleRate / 1000.0 / MockAudioDevice::NUM_SAMPLES_PER_FRAME);
      return RackFrames { static_cast<long>(frames)};
    }

    RackFrames operator()(Duration::SampleFrames d) {
      return RackFrames{ static_cast<long>(std::ceil(d.fCount / static_cast<double>(MockAudioDevice::NUM_SAMPLES_PER_FRAME))) };
    }

    int fSampleRate;
  };

  return std::visit(visitor{iSampleRate}, iType);
}

//------------------------------------------------------------------------
// Duration::toSampleFrames
//------------------------------------------------------------------------
Duration::SampleFrames Duration::toSampleFrames(Duration::Type iType, int iSampleRate)
{
  struct visitor
  {
    SampleFrames operator()(Duration::RackFrames d) { return SampleFrames{d.fCount * MockAudioDevice::NUM_SAMPLES_PER_FRAME}; }

    SampleFrames operator()(Duration::Time d) {
      auto frames = std::ceil(d.fMilliseconds * fSampleRate / 1000.0);
      return SampleFrames { static_cast<long>(frames)};
    }

    SampleFrames operator()(Duration::SampleFrames d) { return d; }

    int fSampleRate;
  };

  return std::visit(visitor{iSampleRate}, iType);
}

}