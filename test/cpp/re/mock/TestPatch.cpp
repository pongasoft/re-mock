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

#include <re/mock/Patch.h>
#include <re/mock/Rack.h>
#include <re/mock/MockDevices.h>
#include <gtest/gtest.h>
#include <re_mock_build.h>
#include <re/mock/stl.h>

namespace re::mock::Test {

using namespace mock;

// Patch.String
TEST(Patch, String)
{
  auto patchString = R"(
<?xml version="1.0"?>
<JukeboxPatch version="2.0"  deviceProductID="com.acme.Kooza"  deviceVersion="1.0.0d1" >
    <DeviceNameInEnglish>
        Kooza
    </DeviceNameInEnglish>
    <Properties>
        <Object name="custom_properties" >
            <Value property="prop_number"  type="number" >
                0.5
            </Value>
            <Value property="prop_boolean"  type="boolean" >
                true
            </Value>
            <Value property="prop_string"  type="string" >
                414243
            </Value>
        </Object>
        <Object name="transport" />
    </Properties>
</JukeboxPatch>
)";

  auto patch = Patch::from(ConfigString{patchString});

  ASSERT_FLOAT_EQ(0.5, std::get<patch_number_property>(patch.fProperties["prop_number"]).fValue);
  ASSERT_TRUE(std::get<patch_boolean_property>(patch.fProperties["prop_boolean"]).fValue);
  ASSERT_EQ("ABC", std::get<patch_string_property>(patch.fProperties["prop_string"]).fValue);
}

// Patch.LoadDefault
TEST(Patch, LoadDefault)
{
  auto defaultPatchString = R"(
<?xml version="1.0"?>
<JukeboxPatch version="2.0"  deviceProductID="com.acme.Kooza"  deviceVersion="1.0.0d1" >
    <DeviceNameInEnglish>
        Kooza
    </DeviceNameInEnglish>
    <Properties>
        <Object name="custom_properties" >
            <Value property="gui_prop_float"  type="number" >
                0.3
            </Value>
            <Value property="prop_float"  type="number" >
                0.5
            </Value>
            <Value property="prop_bool"  type="boolean" >
                true
            </Value>
            <Value property="prop_string"  type="string" >
                414243
            </Value>
        </Object>
        <Object name="transport" />
    </Properties>
</JukeboxPatch>
)";

  Rack rack{};

  struct Device : public MockDevice
  {
    Device(int iSampleRate) : MockDevice(iSampleRate) {}

    void renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount) override
    {
      fDiffs.clear();

      for(int i = 0; i < iDiffCount; i++)
        fDiffs.emplace_back(iPropertyDiffs[i]);
    }

    std::vector<TJBox_PropertyDiff> fDiffs{};
  };

  auto c = DeviceConfig<Device>::fromSkeleton()
    .device_resources_dir(fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources"))
    .default_patch(ConfigString{defaultPatchString})
    .mdef(Config::gui_owner_property("gui_prop_float", lua::jbox_number_property{}.default_value(0.9))) // ignored
    .mdef(Config::document_owner_property("prop_float", lua::jbox_number_property{}.default_value(0.8)))
    .mdef(Config::document_owner_property("prop_bool", lua::jbox_boolean_property{}))
    .mdef(Config::document_owner_property("prop_string", lua::jbox_string_property{}.default_value("abcd")))
    .rtc(Config::rt_input_setup_notify("/custom_properties/prop_float"))
    .rtc(Config::rt_input_setup_notify("/custom_properties/prop_bool"))
    .rtc(Config::rt_input_setup_notify("/custom_properties/prop_string"));

  auto re = rack.newDevice(c);

  auto computeDiffs = [&re]() {
    std::map<std::string, std::string> diffMap{};

    for(auto &diff: re->fDiffs)
      diffMap[re.toString(diff.fPropertyRef)] =
        re.toString(diff.fPreviousValue) + "->" + re.toString(diff.fCurrentValue) + "@" + std::to_string(diff.fAtFrameIndex);
    return diffMap;
  };

  // first frame: get default_value->patch_value
  rack.nextFrame();
  ASSERT_EQ(3, re->fDiffs.size());

  {
    auto diffMap = computeDiffs();

    std::map<std::string, std::string> expected{};
    expected["/custom_properties/prop_float"] = "0.800000->0.500000@0";
    expected["/custom_properties/prop_bool"] = "false->true@0";
    expected["/custom_properties/prop_string"] = "abcd->ABC@0";

    ASSERT_EQ(expected, diffMap);
  }

  // loads a patch string (with 1 change)
  auto patchString = R"(
<?xml version="1.0"?>
<JukeboxPatch version="2.0"  deviceProductID="com.acme.Kooza"  deviceVersion="1.0.0d1" >
    <DeviceNameInEnglish>
        Kooza
    </DeviceNameInEnglish>
    <Properties>
        <Object name="custom_properties" >
            <Value property="prop_string"  type="string" >
                444546
            </Value>
        </Object>
        <Object name="transport" />
    </Properties>
</JukeboxPatch>
)";

  re.loadPatch(ConfigString{patchString});
  rack.nextFrame();
  ASSERT_EQ(1, re->fDiffs.size());

  {
    auto diffMap = computeDiffs();

    std::map<std::string, std::string> expected{};
    expected["/custom_properties/prop_string"] = "ABC->DEF@0";

    ASSERT_EQ(expected, diffMap);
  }

  // load a patch file (2 changes)
  re.loadPatch(*c.resource_file(ConfigFile{fmt::path("re", "mock", "patches", "Kooza_test0.repatch")}));
  rack.nextFrame();
  ASSERT_EQ(2, re->fDiffs.size());

  {
    auto diffMap = computeDiffs();

    std::map<std::string, std::string> expected{};
    expected["/custom_properties/prop_float"] = "0.500000->1.000000@0";
    expected["/custom_properties/prop_string"] = "DEF->Kooza?@0";

    ASSERT_EQ(expected, diffMap);
  }

  // load the same patch file (no change!)
  re.loadPatch(*c.resource_file(ConfigFile{fmt::path("re", "mock", "patches", "Kooza_test0.repatch")}));
  rack.nextFrame();
  ASSERT_EQ(0, re->fDiffs.size());

  // invalid path
  ASSERT_THROW(re.loadPatch(ConfigFile{fmt::path("invalid", "path", "to", "patch")}), Exception);
}

}