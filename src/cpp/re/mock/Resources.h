/*
 * Copyright (c) 2022 pongasoft
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

#ifndef RE_MOCK_RESOURCES_H
#define RE_MOCK_RESOURCES_H

#include <JukeboxTypes.h>
#include <string>
#include <map>
#include <vector>
#include <variant>

namespace re::mock::resource {

struct File { std::string fFilename{}; };
struct String { std::string fString{}; };

enum class LoadStatus { kNil, kPartiallyResident, kResident, kHasErrors, kMissing };

inline bool isLoadStatusOk(LoadStatus s) { return s == LoadStatus::kResident || s == LoadStatus::kPartiallyResident; }

struct LoadingContext
{
  LoadStatus fStatus{LoadStatus::kNil};
  size_t fResidentSize{};

  bool isLoadOk() const { return isLoadStatusOk(fStatus); }
  std::string getStatusAsString() const;
  int getStatusAsInt() const;

  LoadingContext &resident_size(size_t s) { fResidentSize = s; return *this; }
  LoadingContext &status(LoadStatus l) { fStatus = l; return *this; }
};

struct Patch {
  struct boolean_property { bool fValue{}; };
  struct number_property { TJBox_Float64 fValue{}; };
  struct string_property { std::string fValue{}; };
  struct sample_property { std::string fValue{}; };

  using patch_property = std::variant<
    boolean_property,
    number_property,
    string_property,
    sample_property
  >;

  Patch &boolean(std::string iPath, bool iValue) { fProperties[iPath] = boolean_property { iValue }; return *this; };
  Patch &number(std::string iPath, TJBox_Float64 iValue) { fProperties[iPath] = number_property { iValue }; return *this; };
  Patch &string(std::string iPath, std::string const &iValue) { fProperties[iPath] = string_property { iValue }; return *this; };
  Patch &sample(std::string iPath, std::string iValue) { fProperties[iPath] = sample_property { iValue }; return *this; };

  std::map<std::string, patch_property> fProperties{};
};

struct Blob { std::vector<char> fData{}; };

struct Sample
{
  TJBox_UInt32 fChannels{1};
  TJBox_UInt32 fSampleRate{1};
  std::vector<TJBox_AudioSample> fData{};

  Sample &channels(TJBox_UInt32 c) { fChannels = c; return *this; }
  Sample &mono() { return channels(1); }
  Sample &stereo() { return channels(2); }
  Sample &sample_rate(TJBox_UInt32 s) { fSampleRate = s; return *this; }
  Sample &data(std::vector<TJBox_AudioSample> d) { fData = std::move(d); return *this; }
};

}

#endif //RE_MOCK_RESOURCES_H
