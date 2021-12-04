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

#include "Config.h"
#include <functional>

namespace re::mock {

class PatchParser
{
public:
  using sample_reference_resolver = std::function<std::string(int)>;

public:
  static Resource::Patch from(ConfigFile iPatchFile, sample_reference_resolver iSampleResolver = {});
  static Resource::Patch from(ConfigString iPatchString, sample_reference_resolver iSampleResolver = {});
};

}

#endif //RE_MOCK_PATCHLOADER_H