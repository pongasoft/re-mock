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

#include <miniaudio.h>

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
  midiFile->read(iFile.fFilePath.string().c_str());
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

//------------------------------------------------------------------------
// FileManager::loadSample
//------------------------------------------------------------------------
std::unique_ptr<resource::Sample> FileManager::loadSample(resource::File const &iFile)
{
  if(!FileManager::fileExists(iFile))
    return nullptr;

  ma_decoder decoder;
  ma_decoder_config config = ma_decoder_config_init_default();
  config.format = ma_format_f32;
  ma_result result = ma_decoder_init_file(iFile.fFilePath.string().c_str(), &config, &decoder);
  if(result != MA_SUCCESS)
  {
    RE_MOCK_LOG_ERROR("Error opening sample file [%s] %d/%s",
                      iFile.fFilePath.c_str(),
                      result,
                      ma_result_description(result));
    return nullptr;
  }
  auto decoderUninit = stl::defer([&decoder] { ma_decoder_uninit(&decoder); });

  ma_format format;
  ma_uint32 channelCount;
  ma_uint32 sampleRate;
  result = ma_decoder_get_data_format(&decoder, &format, &channelCount, &sampleRate, nullptr, 0);
  if(result != MA_SUCCESS)
  {
    RE_MOCK_LOG_ERROR("Error reading sample file [%s] %d/%s",
                      iFile.fFilePath.c_str(),
                      result,
                      ma_result_description(result));
    return nullptr;
  }

  ma_uint64 frameCount;
  result = ma_data_source_get_length_in_pcm_frames(&decoder, &frameCount);
  if(result != MA_SUCCESS)
  {
    RE_MOCK_LOG_ERROR("Error reading sample file [%s] %d/%s",
                      iFile.fFilePath.c_str(),
                      result,
                      ma_result_description(result));
    return nullptr;
  }

  auto sample = std::make_unique<resource::Sample>();
  sample->fData.resize(frameCount * channelCount);

  ma_uint64 framesRead;
  result = ma_data_source_read_pcm_frames(&decoder, sample->fData.data(), frameCount, &framesRead);
  if(result != MA_SUCCESS)
  {
    RE_MOCK_LOG_ERROR("Error reading sample file [%s] %d/%s",
                      iFile.fFilePath.c_str(),
                      result,
                      ma_result_description(result));
    return nullptr;
  }
  RE_MOCK_INTERNAL_ASSERT(frameCount == framesRead);

  sample->fSampleRate = sampleRate;
  sample->fChannels = channelCount;
  return sample;
}

//------------------------------------------------------------------------
// FileManager::saveSample
//------------------------------------------------------------------------
void FileManager::saveSample(TJBox_UInt32 iChannels,
                             TJBox_UInt32 iSampleRate,
                             std::vector<TJBox_AudioSample> const &iData,
                             resource::File const &iToFile)
{
  ma_encoder_config config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, iChannels, iSampleRate);
  ma_encoder encoder;
  ma_result result = ma_encoder_init_file(iToFile.fFilePath.string().c_str(), &config, &encoder);
  if(result != MA_SUCCESS)
  {
    RE_MOCK_LOG_ERROR("Error writing sample file [%s] %d/%s",
                      iToFile.fFilePath.c_str(),
                      result,
                      ma_result_description(result));
    return;
  }
  auto encoderUninit = stl::defer([&encoder] { ma_encoder_uninit(&encoder); });

  ma_uint64 framesToWrite = iData.size() / iChannels;
  ma_uint64 framesWritten;
  result = ma_encoder_write_pcm_frames(&encoder, iData.data(), framesToWrite, &framesWritten);
  if(result != MA_SUCCESS)
  {
    RE_MOCK_LOG_ERROR("Error writing sample file [%s] %d/%s",
                      iToFile.fFilePath.c_str(),
                      result,
                      ma_result_description(result));
    return;
  }
  RE_MOCK_INTERNAL_ASSERT(framesToWrite == framesWritten);
}


}