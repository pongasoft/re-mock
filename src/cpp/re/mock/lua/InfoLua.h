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

#ifndef RE_MOCK_INFO_H
#define RE_MOCK_INFO_H

#include "MockJBox.h"

namespace re::mock::lua {

/*
-- Max 40 chars
long_name = "Simple Instrument"

-- Max 20 chars
medium_name = "Simple Instrument"

-- Max 10 chars
short_name = "SimpInstr"

product_id = "se.propellerheads.SimpleInstrument"
manufacturer = "Propellerhead Software"
version_number = "1.0.0d1"
device_type = "instrument"
supports_patches = true
default_patch = "/Public/Plain Sinus.repatch"
accepts_notes = true
auto_create_track = true
auto_create_note_lane = true
supports_performance_automation = true
device_height_ru = 2
automation_highlight_color = {r = 60, g = 255, b = 2}
*/

class InfoLua : public MockJBox
{
public:
  std::string long_name();
  std::string medium_name();
  std::string short_name();
  std::string product_id();
  std::string manufacturer();
  std::string version_number();
  std::string device_type();
  bool supports_patches();
  std::string default_patch();
  bool accepts_notes();
  bool auto_create_track();
  bool auto_create_note_lane();
  bool supports_performance_automation();
  int device_height_ru();

public:
  static std::unique_ptr<InfoLua> fromFile(std::string const &iLuaFilename);
  static std::unique_ptr<InfoLua> fromString(std::string const &iLuaCode);

};

}

#endif //RE_MOCK_INFO_H