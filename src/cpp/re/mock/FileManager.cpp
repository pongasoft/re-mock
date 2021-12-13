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

#if RE_MOCK_SUPPORT_FOR_AUDIO_FILE
#include <sndfile.hh>
#endif

namespace re::mock {

namespace impl {

//------------------------------------------------------------------------
// loadFile
//------------------------------------------------------------------------
template<typename Container, size_t BUFFER_SIZE = 1024>
long loadFile(ConfigFile const &iFile, Container &oBuffer)
{
  std::ifstream ifs{iFile.fFilename, std::fstream::binary};
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
      RE_MOCK_LOG_ERROR("Error while reading file %s", iFile.fFilename);
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
std::ifstream::pos_type FileManager::fileSize(ConfigFile const &iFile)
{
  std::ifstream in(iFile.fFilename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

//------------------------------------------------------------------------
// FileManager::fileExists
//------------------------------------------------------------------------
bool FileManager::fileExists(ConfigFile const &iFile)
{
  return std::ifstream{iFile.fFilename}.is_open();
}

//------------------------------------------------------------------------
// FileManager::loadBlob
//------------------------------------------------------------------------
std::optional<Resource::Blob> FileManager::loadBlob(ConfigFile const &iFile)
{
  auto size = fileSize(iFile);
  if(size > -1)
  {
    Resource::Blob blob{};
    blob.fData.reserve(size);
    auto readSize = impl::loadFile(iFile, blob.fData);
    RE_MOCK_ASSERT(readSize == size);
    return blob;
  }
  return std::nullopt;
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

}

//------------------------------------------------------------------------
// FileManager::loadSample
//------------------------------------------------------------------------
std::optional<Resource::Sample> FileManager::loadSample(ConfigFile const &iFile)
{
  if(!FileManager::fileExists(iFile))
    return std::nullopt;

  SndfileHandle sndFile(iFile.fFilename.c_str());

  if(!sndFile.rawHandle())
  {
    RE_MOCK_LOG_ERROR("Error opening sample file [%s] %d/%s",
                      iFile.fFilename.c_str(),
                      sndFile.error(),
                      sndFile.strError());
    return std::nullopt;
  }

  Resource::Sample sample{};
  sample.fData.reserve(sndFile.frames() * sndFile.channels());

  if(impl::loadSample(sndFile, sample.fData) >= 0)
  {
    sample.fSampleRate = sndFile.samplerate();
    sample.fChannels = sndFile.channels();
    return sample;
  }
  else
  {
    RE_MOCK_LOG_ERROR("Error reading sample file [%s] %d/%s",
                      iFile.fFilename.c_str(),
                      sndFile.error(),
                      sndFile.strError());
    return std::nullopt;
  }
}

#else
//------------------------------------------------------------------------
// FileManager::loadSample
//------------------------------------------------------------------------
std::optional<Resource::Sample> FileManager::loadSample(ConfigFile const &iFile)
{
  RE_MOCK_ASSERT(false,
                 "Loading sample is disabled by default. To enable, define option(RE_MOCK_SUPPORT_FOR_AUDIO_FILE \"\" ON) "
                 "before including re-mock  in your CMakeLists.txt (and make sure you do a fresh cmake reconfigure).");
  return std::nullopt;
}
#endif


}