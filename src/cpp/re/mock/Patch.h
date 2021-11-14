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

#ifndef RE_MOCK_PATCHLOADER_H
#define RE_MOCK_PATCHLOADER_H

#include "lua/MotherboardDef.h"
#include "Config.h"

namespace re::mock {

struct patch_boolean_property { bool fValue{}; };
struct patch_number_property { TJBox_Float64 fValue{}; };
struct patch_string_property { std::string fValue{}; };

class Patch
{
public:
  using patch_property_type = std::variant<
    patch_boolean_property,
    patch_number_property,
    patch_string_property
  >;

  static Patch from(ConfigFile iPatchFile);
  static Patch from(ConfigString iPatchString);

  Patch &value(std::string iName, bool iValue) { fProperties[iName] = patch_boolean_property { iValue }; return *this; };
  Patch &value(std::string iName, TJBox_Float64 iValue) { fProperties[iName] = patch_number_property { iValue }; return *this; };
  Patch &value(std::string iName, std::string const &iValue) { fProperties[iName] = patch_string_property { iValue }; return *this; };

public:
  std::map<std::string, patch_property_type> fProperties{};

};

}

#endif //RE_MOCK_PATCHLOADER_H