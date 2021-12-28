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

#ifndef RE_MOCK_FILEMANAGER_H
#define RE_MOCK_FILEMANAGER_H

#include <fstream>
#include "Config.h"

namespace re::mock {

class FileManager
{
public:
  static std::optional<Resource::Blob> loadBlob(ConfigFile const &iFile);
  static std::optional<Resource::Sample> loadSample(ConfigFile const &iFile);
  static void saveSample(Resource::Sample const &iSample, ConfigFile const &iToFile) {
    saveSample(iSample.fChannels, iSample.fSampleRate, iSample.fData, iToFile);
  }
  static void saveSample(TJBox_UInt32 iChannels,
                         TJBox_UInt32 iSampleRate,
                         std::vector<TJBox_AudioSample> const &iData,
                         ConfigFile const &iToFile);

  static std::ifstream::pos_type fileSize(ConfigFile const &iFile);
  static bool fileExists(ConfigFile const &iFile);
};

}

#endif //RE_MOCK_FILEMANAGER_H