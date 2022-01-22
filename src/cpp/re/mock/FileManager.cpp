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

#include "FileManager.h"
#include "Constants.h"

#if RE_MOCK_SUPPORT_FOR_AUDIO_FILE
#include <sndfile.hh>
#endif

namespace re::mock {

namespace impl {

//------------------------------------------------------------------------
// loadFile
//------------------------------------------------------------------------
template<typename Container, size_t BUFFER_SIZE = 1024>
long loadFile(resource::File const &iFile, Container &oBuffer)
{
  std::ifstream ifs{iFile.fFilePath, std::fstream::binary};
  if(!ifs)
    return -1;

  char buf[BUFFER_SIZE];

  bool complete = false;

  size_t fileSize = 0;

  while(!complete)
  {
    ifs.read(buf, BUFFER_SIZE);

    if(ifs.bad())
    {
      RE_MOCK_LOG_ERROR("Error while reading file %s", iFile.fFilePath);
      return -1;
    }

    if(ifs.gcount() > 0)
    {
      std::copy(std::begin(buf), std::begin(buf) + ifs.gcount(), std::back_inserter(oBuffer));
      fileSize += ifs.gcount();
    }

    complete = ifs.eof();
  }

  return fileSize;
}

}

//------------------------------------------------------------------------
// FileManager::fileSize
//------------------------------------------------------------------------
std::ifstream::pos_type FileManager::fileSize(resource::File const &iFile)
{
  std::ifstream in(iFile.fFilePath, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

//------------------------------------------------------------------------
// FileManager::fileExists
//------------------------------------------------------------------------
bool FileManager::fileExists(resource::File const &iFile)
{
  return std::ifstream{iFile.fFilePath}.is_open();
}

//------------------------------------------------------------------------
// FileManager::loadBlob
//------------------------------------------------------------------------
std::unique_ptr<resource::Blob> FileManager::loadBlob(resource::File const &iFile)
{
  auto size = fileSize(iFile);
  if(size > -1)
  {
    auto blob = std::make_unique<resource::Blob>();
    blob->fData.reserve(size);
    auto readSize = impl::loadFile(iFile, blob->fData);
    RE_MOCK_ASSERT(readSize == size);
    return blob;
  }
  return nullptr;
}

//------------------------------------------------------------------------
// FileManager::loadMidi
//------------------------------------------------------------------------
std::unique_ptr<smf::MidiFile> FileManager::loadMidi(resource::File const &iFile, bool iConvertToReasonPPQ)
{
  auto midiFile = std::make_unique<smf::MidiFile>();
  midiFile->read(iFile.fFilePath);
  if(!midiFile->status())
  {
    RE_MOCK_LOG_ERROR("Error opening midi file [%s]", iFile.fFilePath);
    return nullptr;
  }

  // convert to RE native PPQ resolution
  if(iConvertToReasonPPQ && midiFile->getTicksPerQuarterNote() != constants::kPPQResolution)
  {
    RE_MOCK_ASSERT(midiFile->getTicksPerQuarterNote() != 0);

    auto f = constants::kPPQResolution / midiFile->getTicksPerQuarterNote();

    for(int track = 0; track < midiFile->size(); track++)
    {
      auto &events = (*midiFile)[track];
      for(int i = 0; i < events.size(); i++)
      {
        events[i].tick *= f;
      }
    }

    midiFile->setTicksPerQuarterNote(constants::kPPQResolution);
  }
  return midiFile;
}

//------------------------------------------------------------------------
// ostream << smf::MidiFile
//------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, smf::MidiFile const &iMidiFile)
{
  os << fmt::printf("smt::MidiFile | numTracks = %ld, ppqResolution=%ld\n", iMidiFile.size(), iMidiFile.getTicksPerQuarterNote());

  for(int track = 0; track < iMidiFile.size(); track++)
  {
    os << fmt::printf("Track | %d\n", track);
    auto &events = iMidiFile[track];
    for(int i = 0; i < events.size(); i++)
    {
      auto &event = events[i];
      if(event.isTempo())
      {
        os << fmt::printf("[%d@%d] => tempo=%f\n", event.seq, event.tick, event.getTempoBPM());
      } else if(event.isNoteOn())
      {
        os << fmt::printf("[%d@%d] => noteOn=%d/%d\n", event.seq, event.tick, event.getKeyNumber(), event.getVelocity());
      } else if(event.isNoteOff())
      {
        os << fmt::printf("[%d@%d] => noteOff=%d\n", event.seq, event.tick, event.getKeyNumber());
      }
      else
      {
        os << fmt::printf("[%d@%d] => CMD=%d | %s\n", event.seq, event.tick, event.getCommandByte(), event.getMetaContent());
      }
    }
  }
  return os;
}

#if RE_MOCK_SUPPORT_FOR_AUDIO_FILE

namespace impl {
//------------------------------------------------------------------------
// loadSample
//------------------------------------------------------------------------
template<typename Container, size_t BUFFER_SIZE = 1024>
long loadSample(SndfileHandle &iFileHandle, Container &oBuffer)
{
  const auto frameCount = iFileHandle.frames();
  const auto channelCount = iFileHandle.channels();

  std::vector<TJBox_AudioSample> interleavedBuffer(static_cast<unsigned long>(channelCount * BUFFER_SIZE));

  auto expectedFrames = frameCount;
  bool complete = false;

  while(!complete)
  {
    // read up to BUFFER_SIZE frames
    auto frameCountRead = iFileHandle.readf(interleavedBuffer.data(), BUFFER_SIZE);

    // handle error
    if(frameCountRead == 0)
      return -1;

    std::copy(std::begin(interleavedBuffer), std::begin(interleavedBuffer) + (channelCount * frameCountRead),
              std::back_inserter(oBuffer));

    // adjust number of frames to read
    expectedFrames -= frameCountRead;
    complete = expectedFrames == 0;
  }

  return frameCount;
}

//------------------------------------------------------------------------
// loadSample
//------------------------------------------------------------------------
template<typename Container, size_t BUFFER_SIZE = 1024>
bool saveSample(Container const &iBuffer, SndfileHandle &iFileHandle)
{
  const auto channelCount = iFileHandle.channels();

  std::vector<TJBox_AudioSample> interleavedBuffer(static_cast<unsigned long>(channelCount * BUFFER_SIZE));

  auto framesToWrite = iBuffer.size() / channelCount;
  auto samplesWritten = 0;
  bool complete = false;

  while(!complete)
  {
    // fill interleaved buffer
    auto numFrames = std::min(framesToWrite, static_cast<unsigned long>(BUFFER_SIZE));
    auto numSamplesWrite = numFrames * channelCount;

    std::copy(std::begin(iBuffer) + samplesWritten,
              std::begin(iBuffer) + samplesWritten + numSamplesWrite,
              std::begin(interleavedBuffer));

    auto writeCount = iFileHandle.writef(interleavedBuffer.data(), numFrames);
    if(writeCount != numFrames)
    {
      return false;
    }

    samplesWritten += numSamplesWrite;
    framesToWrite -= numFrames;
    complete = framesToWrite == 0;
  }

  return true;
}

}

//------------------------------------------------------------------------
// FileManager::loadSample
//------------------------------------------------------------------------
std::unique_ptr<resource::Sample> FileManager::loadSample(resource::File const &iFile)
{
  if(!FileManager::fileExists(iFile))
    return nullptr;

  SndfileHandle sndFile(iFile.fFilePath.c_str());

  if(!sndFile.rawHandle())
  {
    RE_MOCK_LOG_ERROR("Error opening sample file [%s] %d/%s",
                      iFile.fFilePath.c_str(),
                      sndFile.error(),
                      sndFile.strError());
    return nullptr;
  }

  auto sample = std::make_unique<resource::Sample>();
  sample->fData.reserve(sndFile.frames() * sndFile.channels());

  if(impl::loadSample(sndFile, sample->fData) >= 0)
  {
    sample->fSampleRate = sndFile.samplerate();
    sample->fChannels = sndFile.channels();
    return sample;
  }
  else
  {
    RE_MOCK_LOG_ERROR("Error reading sample file [%s] %d/%s",
                      iFile.fFilePath.c_str(),
                      sndFile.error(),
                      sndFile.strError());
    return nullptr;
  }
}

//------------------------------------------------------------------------
// FileManager::saveSample
//------------------------------------------------------------------------
void FileManager::saveSample(TJBox_UInt32 iChannels,
                             TJBox_UInt32 iSampleRate,
                             std::vector<TJBox_AudioSample> const &iData,
                             resource::File const &iToFile)
{
  SndfileHandle sndFile(iToFile.fFilePath.c_str(),
                        SFM_WRITE, // open for writing
                        SF_FORMAT_WAV | SF_FORMAT_PCM_32,
                        iChannels,
                        static_cast<int>(iSampleRate));

  if(!sndFile.rawHandle())
  {
    RE_MOCK_LOG_ERROR("Error opening sample file [%s] %d/%s",
                      iToFile.fFilePath.c_str(),
                      sndFile.error(),
                      sndFile.strError());
    return;
  }

  if(!impl::saveSample(iData, sndFile))
  {
    RE_MOCK_LOG_ERROR("Error writing sample file [%s] %d/%s",
                      iToFile.fFilePath.c_str(),
                      sndFile.error(),
                      sndFile.strError());
  }
}


#else
//------------------------------------------------------------------------
// FileManager::loadSample
//------------------------------------------------------------------------
std::optional<resource::Sample> FileManager::loadSample(resource::File const &iFile)
{
  RE_MOCK_ASSERT(false,
                 "Loading sample is disabled by default. To enable, define option(RE_MOCK_SUPPORT_FOR_AUDIO_FILE \"\" ON) "
                 "before including re-mock  in your CMakeLists.txt (and make sure you do a fresh cmake reconfigure).");
  return std::nullopt;
}

//------------------------------------------------------------------------
// FileManager::saveSample
//------------------------------------------------------------------------
void FileManager::saveSample(TJBox_UInt32 iChannels,
                             TJBox_UInt32 iSampleRate,
                             std::vector<TJBox_AudioSample> const &iData,
                             resource::File const &iToFile)
{
  RE_MOCK_ASSERT(false,
                "Saving a sample is disabled by default. To enable, define option(RE_MOCK_SUPPORT_FOR_AUDIO_FILE \"\" ON) "
                "before including re-mock  in your CMakeLists.txt (and make sure you do a fresh cmake reconfigure).");

}
#endif


}