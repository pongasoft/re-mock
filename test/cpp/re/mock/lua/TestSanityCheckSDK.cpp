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

#include <re/mock/lua/MotherboardDef.h>
#include <re/mock/lua/RealtimeController.h>
#include <re/mock/lua/InfoLua.h>
#include <gtest/gtest.h>
#include <re_mock_build.h>
#include <re/mock/PatchParser.h>

namespace re::mock::lua::Test {

using namespace testing;

static auto SDK_EXAMPLES = {
  "ColumnDisplayDevice",
  "CustomDisplayDevice",
  "HideableWidgetsDevice",
  "PopupAndModifierKeys",
  "RE2DAllWidgets",
  "SilenceDetectionEffect",
  "SimpleInstrument",
  "SimplePatternPlayer",
  "SimplePlayer",
  "VerySimpleSampler"
};

/**
 * The purpose of this test is to make sure that all the examples provided with the SDK can be
 * at least loaded/parsed with `MotherboardDef::fromFile` and `RealtimeController::fromFile`
 */
TEST(SanityCheck, SDKExamples)
{
  if(std::string(RE_SDK_ROOT).empty())
  {
    RE_MOCK_LOG_WARNING("Cannot run this test when the full SDK is not installed!");
    return;
  }

  for(auto &p: SDK_EXAMPLES)
  {
    {
      auto path = fs::path(RE_SDK_ROOT) / "Examples" / p / "info.lua";


      auto def = InfoLua::fromFile(path);

      int patchPropertiesCount = 0;

      if(def->supports_patches())
      {
        auto defaultPatch = fs::path(def->default_patch(), fs::path::format::generic_format).relative_path();
        auto patchFile = fs::path(RE_SDK_ROOT) / "Examples" / p / "Resources" / defaultPatch;
        auto patch = PatchParser::from(resource::File{patchFile});
        patchPropertiesCount = patch->fProperties.size();
      }

      std::cout << p << "/info.lua: "
                << "device_type=" << def->device_type()
                << ";supports_patches=" << def->supports_patches()
                << ";default_patch=" << def->default_patch()
                << ";patch_properties_count=" << patchPropertiesCount
                << std::endl;
    }

    {
      auto path = fs::path(RE_SDK_ROOT) / "Examples" / p / "motherboard_def.lua";

      auto def = MotherboardDef::fromFile(path);
      auto customProperties = def->getCustomProperties();
      std::cout << p << "/motherboard_def.lua: "
                << "audio_inputs=" << def->getAudioInputs()->fNames.size()
                << ";audio_outputs=" << def->getAudioOutputs()->fNames.size()
                << ";cv_inputs=" << def->getCVInputs()->fNames.size()
                << ";cv_outputs=" << def->getCVOutputs()->fNames.size()
                << ";document_owner=" << customProperties->document_owner.size()
                << ";rtc_owner=" << customProperties->rtc_owner.size()
                << ";rt_owner=" << customProperties->rt_owner.size()
                << ";patterns=" << def->getNumPatterns()
                << std::endl;
    }

    {
      auto path = fs::path(RE_SDK_ROOT) / "Examples" / p / "realtime_controller.lua";

      auto def = RealtimeController::fromFile(path);
      std::cout << p << "/realtime_controller.lua: "
                << "rtc_bindings=" << def->getBindings().size()
                << ";rt_input_setup=" << def->getRTInputSetupNotify().size()
                << std::endl;
    }
  }
}

}