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
// DeviceTester::nextBatches
//------------------------------------------------------------------------
void DeviceTester::nextBatches(Duration::Type iDuration)
{
  auto numBatches = Duration::toBatches(iDuration, fRack.getSampleRate());

  for(int i = 0; i < numBatches.fCount; i++)
    fRack.nextBatch();
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
// ExtensionEffectTester::nextBatch
//------------------------------------------------------------------------
MockAudioDevice::StereoBuffer ExtensionEffectTester::nextBatch(MockAudioDevice::StereoBuffer const &iInputBuffer)
{
  MockAudioDevice::StereoBuffer output{};
  nextBatch(iInputBuffer, output);
  return output;
}

//------------------------------------------------------------------------
// ExtensionEffectTester::nextBatch
//------------------------------------------------------------------------
void ExtensionEffectTester::nextBatch(MockAudioDevice::StereoBuffer const &iInputBuffer,
                                      MockAudioDevice::StereoBuffer &oOutputBuffer)
{
  fSrc->fBuffer = iInputBuffer;
  fRack.nextBatch();
  oOutputBuffer = fDst->fBuffer;
}

//------------------------------------------------------------------------
// ExtensionEffectTester::processSample
//------------------------------------------------------------------------
MockAudioDevice::Sample ExtensionEffectTester::processSample(MockAudioDevice::Sample const &iSample,
                                                             optional_duration_t iTail,
                                                             before_frame_hook_t iBeforeFrameHook)
{
  size_t tailInFrames = 0;

  if(iTail)
    tailInFrames = Duration::toFrames(*iTail, fRack.getSampleRate()).fCount;

  MockAudioDevice::Sample res{};
  res.fChannels = iSample.fChannels;
  res.fSampleRate = fRack.getSampleRate();
  res.fData.reserve(iSample.fData.size() + tailInFrames);

  MockAudioDevice::StereoBuffer input{};
  MockAudioDevice::StereoBuffer output{};

  auto numFramesToProcess = iSample.fData.size() / iSample.fChannels;

  auto totalNumFrames = numFramesToProcess + tailInFrames;
  auto ptr = iSample.fData.data();

  int frameCount = 0;

  while(totalNumFrames > 0)
  {
    auto numFramesInThisBatch = std::min<size_t>(totalNumFrames, MockAudioDevice::NUM_SAMPLES_PER_BATCH);
    auto numFramesToProcessInThisBatch = std::min<size_t>(numFramesToProcess, numFramesInThisBatch);

    if(numFramesToProcessInThisBatch > 0)
    {
      if(numFramesToProcessInThisBatch < MockAudioDevice::NUM_SAMPLES_PER_BATCH)
        input.fill(0, 0);

      // fill the input buffer
      for(size_t i = 0; i < numFramesToProcessInThisBatch; i++)
      {
        input.fLeft[i] = *ptr++;
        if(iSample.fChannels == 2)
          input.fRight[i] = *ptr++;
      }
    }
    else
      input.fill(0, 0);

    // allow outside code to modify the device prior to invoking nextBatch
    if(iBeforeFrameHook)
      iBeforeFrameHook(frameCount);

    // process this batch
    nextBatch(input, output);

    // fill the output buffer
    for(size_t i = 0; i < numFramesInThisBatch; i++)
    {
      res.fData.emplace_back(output.fLeft[i]);
      if(iSample.fChannels == 2)
        res.fData.emplace_back(output.fRight[i]);
    }

    totalNumFrames -= numFramesInThisBatch;
    numFramesToProcess -= numFramesToProcessInThisBatch;
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
// ExtensionInstrumentTester::nextBatch
//------------------------------------------------------------------------
void ExtensionInstrumentTester::nextBatch(MockAudioDevice::StereoBuffer &oOutputBuffer)
{
  nextBatch({}, oOutputBuffer);
}

//------------------------------------------------------------------------
// ExtensionInstrumentTester::nextBatch
//------------------------------------------------------------------------
MockAudioDevice::StereoBuffer ExtensionInstrumentTester::nextBatch(MockDevice::NoteEvents iNoteEvents)
{
  MockAudioDevice::StereoBuffer output{};
  nextBatch(iNoteEvents, output);
  return output;
}

//------------------------------------------------------------------------
// ExtensionInstrumentTester::nextBatch
//------------------------------------------------------------------------
void ExtensionInstrumentTester::nextBatch(MockDevice::NoteEvents iNoteEvents,
                                          MockAudioDevice::StereoBuffer &oOutputBuffer)
{
  setNoteEvents(iNoteEvents);
  fRack.nextBatch();
  oOutputBuffer = fDst->fBuffer;
}

//------------------------------------------------------------------------
// ExtensionInstrumentTester::play
//------------------------------------------------------------------------
MockAudioDevice::Sample ExtensionInstrumentTester::play(Duration::Type iDuration, before_frame_hook_t iBeforeFrameHook)
{
  auto totalNumFrames = Duration::toFrames(iDuration, fRack.getSampleRate()).fCount;

  MockAudioDevice::Sample res{};
  res.fChannels = 2;
  res.fSampleRate = fRack.getSampleRate();
  res.fData.reserve(totalNumFrames);

  int frameCount = 0;

  while(totalNumFrames > 0)
  {
    // allow outside code to modify the device prior to invoking nextBatch
    if(iBeforeFrameHook)
      iBeforeFrameHook(frameCount);

    fRack.nextBatch();

    auto numFramesInThisBatch = std::min<size_t>(totalNumFrames, MockAudioDevice::NUM_SAMPLES_PER_BATCH);

    res.append(fDst->fBuffer, numFramesInThisBatch);

    totalNumFrames -= numFramesInThisBatch;
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
// ExtensionNotePlayerTester::nextBatch
//------------------------------------------------------------------------
MockDevice::NoteEvents ExtensionNotePlayerTester::nextBatch(MockDevice::NoteEvents iSourceEvents)
{
  fSrc->fNoteEvents = std::move(iSourceEvents);
  fRack.nextBatch();
  return fDst->fNoteEvents;
}

//------------------------------------------------------------------------
// Duration::toRackFrames
//------------------------------------------------------------------------
Duration::Batches Duration::toBatches(Duration::Type iType, int iSampleRate)
{
  struct visitor
  {
    Batches operator()(Duration::Batches d) { return d; }

    Batches operator()(Duration::Time d) {
      auto batches = std::ceil(d.fMilliseconds * fSampleRate / 1000.0 / MockAudioDevice::NUM_SAMPLES_PER_BATCH);
      return Batches { static_cast<long>(batches)};
    }

    Batches operator()(Duration::Frames d) {
      return Batches{ static_cast<long>(std::ceil(d.fCount / static_cast<double>(MockAudioDevice::NUM_SAMPLES_PER_BATCH))) };
    }

    int fSampleRate;
  };

  return std::visit(visitor{iSampleRate}, iType);
}

//------------------------------------------------------------------------
// Duration::toSampleFrames
//------------------------------------------------------------------------
Duration::Frames Duration::toFrames(Duration::Type iType, int iSampleRate)
{
  struct visitor
  {
    Frames operator()(Duration::Batches d) { return Frames{d.fCount * MockAudioDevice::NUM_SAMPLES_PER_BATCH}; }

    Frames operator()(Duration::Time d) {
      auto frames = std::ceil(d.fMilliseconds * fSampleRate / 1000.0);
      return Frames { static_cast<long>(frames)};
    }

    Frames operator()(Duration::Frames d) { return d; }

    int fSampleRate;
  };

  return std::visit(visitor{iSampleRate}, iType);
}

}