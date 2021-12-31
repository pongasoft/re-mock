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

#ifndef RE_MOCK_CONSTANTS_H
#define RE_MOCK_CONSTANTS_H

#include <JukeboxTypes.h>

namespace re::mock::constants {

constexpr TJBox_UInt64 kNumTicksPer16th = 240;
constexpr TJBox_UInt64 kPPQResolution = 15360;
constexpr TJBox_UInt64 kPPT16thResolution = kPPQResolution / 4;
constexpr TJBox_UInt64 kPPTickResolution = kPPT16thResolution / kNumTicksPer16th;
constexpr int kBatchSize = 64;
constexpr TJBox_Float32 kSilentThreshold = 2.0e-8f;

}

#endif //RE_MOCK_CONSTANTS_H
