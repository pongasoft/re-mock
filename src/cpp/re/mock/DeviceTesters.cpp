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
void DeviceTester::nextBatches(Duration iDuration)
{
  auto duration = fRack.toRackDuration(iDuration);

  for(int i = 0; i < duration.fBatches; i++)
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
// DeviceTester::saveSample
//------------------------------------------------------------------------
void DeviceTester::saveSample(MockAudioDevice::Sample const &iSample, ConfigFile const &iToFile) const
{
  FileManager::saveSample(iSample.fChannels, iSample.fSampleRate, iSample.fData, iToFile);
}

//------------------------------------------------------------------------
// DeviceTester::loadMidi
//------------------------------------------------------------------------
void DeviceTester::loadMidi(ConfigFile const &iMidiFile, int iTrack, bool iImportTempo)
{
  auto midiFile = FileManager::loadMidi(iMidiFile).value();

  // import tempo from midi file
  if(iImportTempo)
  {
    for(int track = 0; track < midiFile.size(); track++)
    {
      auto &events = midiFile[track];
      for(int i = 0; i < events.size(); i++)
      {
        auto &event = events[i];
        if(event.isTempo())
        {
          fRack.setTransportTempo(event.getTempoBPM());
        }
      }
    }
  }

  if(iTrack == -1)
  {
    midiFile.joinTracks();
    iTrack = 0;
  }

  RE_MOCK_ASSERT(iTrack < midiFile.size(), "Cannot read track [%d]: not enough tracks in midifile (%ld)", iTrack, midiFile.size());

  fDevice.loadMidiNotes(midiFile[iTrack]);
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
                                                             std::optional<tester::Timeline> iTimeline)
{
  size_t tailInFrames = 0;

  if(iTail)
    tailInFrames = fRack.toSampleDuration(*iTail).fFrames;

  if(!iTimeline)
    iTimeline = newTimeline();

  MockAudioDevice::Sample res{};
  res.fChannels = iSample.fChannels;
  res.fSampleRate = fRack.getSampleRate();
  res.fData.reserve(iSample.fData.size() + tailInFrames);

  MockAudioDevice::StereoBuffer input{};
  MockAudioDevice::StereoBuffer output{};

  auto numFramesToProcess = iSample.fData.size() / iSample.fChannels;

  auto totalNumFrames = numFramesToProcess + tailInFrames;
  auto ptr = iSample.fData.data();

  iTimeline->onEveryBatch([this, &totalNumFrames, &numFramesToProcess, &input, &output, &ptr, stereo = iSample.isStereo(), &res]() {
    auto numFramesInThisBatch = std::min<size_t>(totalNumFrames, constants::kBatchSize);
    auto numFramesToProcessInThisBatch = std::min<size_t>(numFramesToProcess, numFramesInThisBatch);

    if(numFramesToProcessInThisBatch > 0)
    {
      if(numFramesToProcessInThisBatch < constants::kBatchSize)
        input.fill(0, 0);

      // fill the input buffer
      for(size_t i = 0; i < numFramesToProcessInThisBatch; i++)
      {
        input.fLeft[i] = *ptr++;
        if(stereo)
          input.fRight[i] = *ptr++;
      }
    }
    else
      input.fill(0, 0);

    // process this batch
    nextBatch(input, output);

    // fill the output buffer
    for(size_t i = 0; i < numFramesInThisBatch; i++)
    {
      res.fData.emplace_back(output.fLeft[i]);
      if(stereo)
        res.fData.emplace_back(output.fRight[i]);
    }

    totalNumFrames -= numFramesInThisBatch;
    numFramesToProcess -= numFramesToProcessInThisBatch;
  });

  iTimeline->execute(sample::Duration{static_cast<long>(totalNumFrames)});

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
MockAudioDevice::StereoBuffer ExtensionInstrumentTester::nextBatch(MockDevice::NoteEvents iNoteEvents)
{
  setNoteEvents(iNoteEvents);
  fRack.nextBatch();
  return fDst->fBuffer;
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

namespace tester {

const Timeline::Event Timeline::kNoOp = [](long iAtBatch) { return true; };
const Timeline::Event Timeline::kEnd  = [](long iAtBatch) { return false; };

//------------------------------------------------------------------------
// Timeline::wrap
//------------------------------------------------------------------------
Timeline::Event Timeline::wrap(tester::Timeline::SimpleEvent iEvent)
{
  if(iEvent)
    return [event = std::move(iEvent)](long iAtBatch) {
      event();
      return true;
    };
  else
    return kNoOp;
}

//------------------------------------------------------------------------
// Timeline::after
//------------------------------------------------------------------------
Timeline &Timeline::after(Duration iDuration)
{
  fCurrentBath += fTester->rack().toRackDuration(iDuration).fBatches;
  return *this;
}

//------------------------------------------------------------------------
// Timeline::transportStart
//------------------------------------------------------------------------
Timeline &Timeline::transportStart()
{
  event([this]() { fTester->transportStart(); });
  return *this;
}

//------------------------------------------------------------------------
// Timeline::transportStop
//------------------------------------------------------------------------
Timeline &Timeline::transportStop()
{
  event([this]() { fTester->transportStop(); });
  return *this;
}

//------------------------------------------------------------------------
// Timeline::event
//------------------------------------------------------------------------
Timeline &Timeline::event(long iAtBatch, Timeline::Event iEvent)
{
  if(!iEvent)
    iEvent = kNoOp;

  auto id = fLastEventId++;

  if(fSorted && !fEvents.empty())
    fSorted = fEvents[fEvents.size() - 1].fAtBatch <= iAtBatch;

  fEvents.emplace_back(EventImpl{id, iAtBatch, std::move(iEvent)});

  return *this;
}

//------------------------------------------------------------------------
// Timeline::ensureSorted
//------------------------------------------------------------------------
void Timeline::ensureSorted() const
{
  if(!fSorted)
  {
    // Implementation note: technically the fEvents vector should be sorted all the time, but it is more
    // efficient to keep a flag (fSorted) which remains true for as long as events are added in order
    // and only sort once when required. This is why we have to remove const here: from the outside the
    // events are sorted
    auto nonConstThis = const_cast<Timeline *>(this);

    // we sort the timeline so that events are in the proper fAtBatch
    std::sort(nonConstThis->fEvents.begin(), nonConstThis->fEvents.end(),
              [](EventImpl const &l, EventImpl const &r) {
                if(l.fAtBatch == r.fAtBatch)
                  return l.fId < r.fId;
                else
                  return l.fAtBatch < r.fAtBatch;
              });
  }

  fSorted = true;
}

//------------------------------------------------------------------------
// Timeline::execute
//------------------------------------------------------------------------
void Timeline::execute(std::optional<Duration> iDuration) const
{
  auto &rack = fTester->rack();

  auto batches = iDuration ? rack.toRackDuration(*iDuration).fBatches : (fEvents.size() > 0 ? fCurrentBath + 1 : fCurrentBath);

  auto const &events = getEvents();

  auto sortedEvents = events.begin();

  for(int batch = 0; batch < batches; batch++)
  {
    for(auto &event: fOnEveryBatchEvents)
    {
      if(!event(batch))
        return; // terminate execution if returns false
    }

    while(sortedEvents != events.end() && sortedEvents->fAtBatch == batch)
    {
      if(!sortedEvents->fEvent(batch))
        return; // terminate execution if event returns false

      sortedEvents++;
    }

    rack.nextBatch();
  }

}

//------------------------------------------------------------------------
// Timeline::play
//------------------------------------------------------------------------
void Timeline::play(std::optional<Duration> iDuration) const
{
  fTester->transportStart();
  execute(iDuration);
  fTester->transportStop();
}

//------------------------------------------------------------------------
// Timeline::notes
//------------------------------------------------------------------------
Timeline &Timeline::notes(MockDevice::NoteEvents iNoteEvents)
{
  event([this, iNoteEvents]() { fTester->fDevice.setNoteInEvents(iNoteEvents.events());} );
  return *this;
}

//------------------------------------------------------------------------
// Timeline::note
//------------------------------------------------------------------------
Timeline &Timeline::note(TJBox_UInt8 iNoteNumber,
                         Duration iDuration,
                         TJBox_UInt8 iNoteVelocity)
{
  // note on
  event([this, iNoteNumber, iNoteVelocity]() { fTester->fDevice.setNoteInEvent(iNoteNumber, iNoteVelocity);} );

  // note off after duration
  event(fCurrentBath + fTester->rack().toRackDuration(iDuration).fBatches,
        [this, iNoteNumber]() { fTester->fDevice.setNoteInEvent(iNoteNumber, 0); } );

  return *this;
}

//------------------------------------------------------------------------
// Timeline::onEveryBatch
//------------------------------------------------------------------------
Timeline &Timeline::onEveryBatch(Timeline::Event iEvent)
{
  fOnEveryBatchEvents.emplace_back(std::move(iEvent));
  return *this;
}

}



}