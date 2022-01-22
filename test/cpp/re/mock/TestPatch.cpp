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

#include <re/mock/PatchParser.h>
#include <re/mock/Rack.h>
#include <re/mock/MockDevices.h>
#include <re/mock/MockJukebox.h>
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
    <Samples>
        <SampleReference trueName="Bell.wav"  userFriendlyName="Bell"  type="sample" >
            <JukeboxDevice internalID="com.acme.Kooza"  uiName="Kooza"  version="1.0.0d1" />
            <DatabasePath>
                <ReFillName/>
                <Path pathKind="jukebox" >
                    /Rack extensions/com.acme.Kooza/Public/Samples/Bell.wav
                </Path>
            </DatabasePath>
        </SampleReference>
        <SampleReference trueName="Eastern Gold Voice 01.aif"  userFriendlyName="Eastern Gold Voice 01.aif"  type="sample" >
            <AbsolutePath>
                <Volume name=""  type="unknown" />
                <Path partOfReFill="no" >
                    /tmp/Music/Samples/Sample 01.aif
                </Path>
                <NativeURL mode="OSXFile" >
                    file:///tmp/Music/Samples/Sample%2001.aif
                </NativeURL>
            </AbsolutePath>
            <RelativePath>
                <Path>
                    /Music/Samples/Sample 01.aif
                </Path>
                <StepUp>
                    1
                </StepUp>
            </RelativePath>
        </SampleReference>
    </Samples>
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
        <Object name="user_samples/0" >
            <Value property="item"  type="sample" >
                0
            </Value>
        </Object>
        <Object name="user_samples/1" >
            <Value property="item"  type="sample" >
                1
            </Value>
        </Object>
    </Properties>
</JukeboxPatch>
)";

  auto patch = PatchParser::from(resource::String{patchString});

  ASSERT_FLOAT_EQ(0.5, std::get<resource::Patch::number_property>(patch->fProperties["/custom_properties/prop_number"]).fValue);
  ASSERT_TRUE(std::get<resource::Patch::boolean_property>(patch->fProperties["/custom_properties/prop_boolean"]).fValue);
  ASSERT_EQ("ABC", std::get<resource::Patch::string_property>(patch->fProperties["/custom_properties/prop_string"]).fValue);
  ASSERT_EQ("/Public/Samples/Bell.wav", std::get<resource::Patch::sample_property>(patch->fProperties["/user_samples/0/item"]).fValue);
  ASSERT_EQ("/tmp/Music/Samples/Sample 01.aif", std::get<resource::Patch::sample_property>(patch->fProperties["/user_samples/1/item"]).fValue);
}

// Patch.Load
TEST(Patch, Load)
{
  auto defaultPatchString = R"(
<?xml version="1.0"?>
<JukeboxPatch version="2.0"  deviceProductID="com.acme.Kooza"  deviceVersion="1.0.0d1" >
    <DeviceNameInEnglish>
        Kooza
    </DeviceNameInEnglish>
    <Samples>
        <SampleReference trueName="mono_sample.data"  userFriendlyName="mono_sample"  type="sample" >
            <DatabasePath>
                <ReFillName/>
                <Path pathKind="jukebox" >
                    /Rack extensions/com.acme.Kooza/Private/mono_sample.data
                </Path>
            </DatabasePath>
        </SampleReference>
        <SampleReference trueName="stereo_sample.data"  userFriendlyName="stereo_sample"  type="sample" >
            <DatabasePath>
                <ReFillName/>
                <Path pathKind="jukebox" >
                    /Rack extensions/com.acme.Kooza/Private/stereo_sample.data
                </Path>
            </DatabasePath>
        </SampleReference>
    </Samples>
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
        <Object name="user_samples/0" >
            <Value property="root_key"  type="number" >
                25
            </Value>
            <Value property="item"  type="sample" >
                0
            </Value>
        </Object>
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
      {
        auto diff = iPropertyDiffs[i];
        char const *f = nullptr;
        if(JBox_GetType(diff.fCurrentValue) == kJBox_Number)
          f = "%.1f";
        fDiffs[JBox_toString(diff.fPropertyRef)] =
          JBox_toString(diff.fPreviousValue, f) + "->" + JBox_toString(diff.fCurrentValue, f) + "@" + std::to_string(diff.fAtFrameIndex);
      }
    }

    std::map<std::string, std::string> fDiffs{};
  };

  std::vector<TJBox_AudioSample> sampleMonoData{0,1,2,3,4,5};
  std::vector<TJBox_AudioSample> sampleStereoData{1,10,2,20,3,30,4,40,5,50};

  auto c = DeviceConfig<Device>::fromSkeleton()
    .device_resources_dir(fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources"))
    .default_patch("/Public/default.repatch")
    .mdef(Config::gui_owner_property("gui_prop_float", lua::jbox_number_property{}.default_value(0.9))) // ignored
    .mdef(Config::document_owner_property("prop_float", lua::jbox_number_property{}.default_value(0.8)))
    .mdef(Config::document_owner_property("prop_bool", lua::jbox_boolean_property{}))
    .mdef(Config::document_owner_property("prop_string", lua::jbox_string_property{}.default_value("abcd")))
    .mdef(Config::user_sample(0, lua::jbox_user_sample_property{}.sample_parameter("root_key")))
    .mdef(Config::user_sample(1, lua::jbox_user_sample_property{}))
    .rtc(Config::rt_input_setup_notify("/custom_properties/prop_float"))
    .rtc(Config::rt_input_setup_notify("/custom_properties/prop_bool"))
    .rtc(Config::rt_input_setup_notify("/custom_properties/prop_string"))
    .rtc(Config::rt_input_setup_notify("/user_samples/0/*"))
    .rtc(Config::rt_input_setup_notify("/user_samples/1/*"))
    .rtc(Config::rt_input_setup_notify("/device_host/sample_context"))
    .sample_data("/Private/mono_sample.data", resource::Sample{}.sample_rate(44100).mono().data(sampleMonoData))
    .sample_data("/Private/stereo_sample.data", resource::Sample{}.sample_rate(48000).stereo().data(sampleStereoData))

    .patch_string("/Public/default.repatch", defaultPatchString)

    // relative to device_resources_dir
    .patch_file("/Public/patch1.repatch", "/re/mock/patches/Kooza_test1.repatch")

    .patch_data("/Public/patch2.repatch", resource::Patch{}.number("/device_host/sample_context", 1).sample("/user_samples/0/item", "/Private/stereo_sample.data"))
    ;

  auto re = rack.newDevice(c);

  auto defaultValuesPatch = re.getDefaultValuesPatch();

  ASSERT_EQ(7, defaultValuesPatch.fProperties.size());
  ASSERT_EQ(0.8, std::get<resource::Patch::number_property>(defaultValuesPatch.fProperties["/custom_properties/prop_float"]).fValue);
  ASSERT_FALSE(std::get<resource::Patch::boolean_property>(defaultValuesPatch.fProperties["/custom_properties/prop_bool"]).fValue);
  ASSERT_EQ("abcd", std::get<resource::Patch::string_property>(defaultValuesPatch.fProperties["/custom_properties/prop_string"]).fValue);
  ASSERT_EQ(0, std::get<resource::Patch::number_property>(defaultValuesPatch.fProperties["/device_host/sample_context"]).fValue);
  ASSERT_EQ("", std::get<resource::Patch::sample_property>(defaultValuesPatch.fProperties["/user_samples/0/item"]).fValue);
  ASSERT_EQ(60.0, std::get<resource::Patch::number_property>(defaultValuesPatch.fProperties["/user_samples/0/root_key"]).fValue);
  ASSERT_EQ("", std::get<resource::Patch::sample_property>(defaultValuesPatch.fProperties["/user_samples/1/item"]).fValue);

  // first batch: get default_value->patch_value
  rack.nextBatch();
  ASSERT_EQ(7, re->fDiffs.size());

  {
    std::map<std::string, std::string> expected{};
    expected["/custom_properties/prop_float"] = "0.8->0.5@0";
    expected["/custom_properties/prop_bool"] = "false->true@0";
    expected["/custom_properties/prop_string"] = "abcd->ABC@0";
    expected["/device_host/sample_context"] = "0.0->0.0@0";
    expected["/user_samples/0/item"] = "UserSample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=nil,.fSamplePath=[]}->"
                                       "UserSample{.fChannels=1,.fSampleRate=44100,.fFrameCount=6,.fResidentFrameCount=6,.fLoadStatus=resident,.fSamplePath=[/Private/mono_sample.data]}@0";
    expected["/user_samples/0/root_key"] = "60.0->25.0@0";
    expected["/user_samples/1/item"] = "UserSample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=nil,.fSamplePath=[]}->"
                                       "UserSample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=nil,.fSamplePath=[]}@0";

    ASSERT_EQ(expected, re->fDiffs);
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
        <Object name="user_samples/0" >
            <Value property="item"  type="sample"/>
        </Object>
    </Properties>
</JukeboxPatch>
)";

  re.loadPatch(resource::String{patchString});
  rack.nextBatch();
  ASSERT_EQ(1, re->fDiffs.size());

  {
    std::map<std::string, std::string> expected{};
    expected["/custom_properties/prop_string"] = "ABC->DEF@0";

    ASSERT_EQ(expected, re->fDiffs);
  }

  // load a patch file (via absolute path) (2 changes)
  re.loadPatch(*c.resource_file(resource::File{fmt::path("re", "mock", "patches", "Kooza_test0.repatch")}));
  rack.nextBatch();
  ASSERT_EQ(4, re->fDiffs.size());

  {
    std::map<std::string, std::string> expected{};
    expected["/custom_properties/prop_float"] = "0.5->1.0@0";
    expected["/custom_properties/prop_string"] = "DEF->Kooza?@0";
    expected["/user_samples/0/item"] = "UserSample{.fChannels=1,.fSampleRate=44100,.fFrameCount=6,.fResidentFrameCount=6,.fLoadStatus=resident,.fSamplePath=[/Private/mono_sample.data]}->"
                                       "UserSample{.fChannels=2,.fSampleRate=48000,.fFrameCount=5,.fResidentFrameCount=5,.fLoadStatus=resident,.fSamplePath=[/Private/stereo_sample.data]}@0";
    expected["/user_samples/0/root_key"] = "25.0->61.0@0";

    ASSERT_EQ(expected, re->fDiffs);
  }

  // load the same patch file (no change!)
  re.loadPatch(*c.resource_file(resource::File{fmt::path("re", "mock", "patches", "Kooza_test0.repatch")}));
  rack.nextBatch();
  ASSERT_EQ(0, re->fDiffs.size());

  // load another patch (via indirect mapping)
  re.loadPatch("/Public/patch1.repatch");
  rack.nextBatch();
  ASSERT_EQ(4, re->fDiffs.size());

  {
    std::map<std::string, std::string> expected{};
    expected["/custom_properties/prop_float"] = "1.0->0.2@0";
    expected["/device_host/sample_context"] = "0.0->1.0@0";
    expected["/user_samples/0/item"] = "UserSample{.fChannels=2,.fSampleRate=48000,.fFrameCount=5,.fResidentFrameCount=5,.fLoadStatus=resident,.fSamplePath=[/Private/stereo_sample.data]}->"
                                       "UserSample{.fChannels=1,.fSampleRate=44100,.fFrameCount=6,.fResidentFrameCount=6,.fLoadStatus=resident,.fSamplePath=[/Private/mono_sample.data]}@0";
    expected["/user_samples/0/root_key"] = "61.0->62.0@0";

    ASSERT_EQ(expected, re->fDiffs);
  }

  // invalid path
  ASSERT_THROW(re.loadPatch(resource::File{fmt::path("invalid", "path", "to", "patch")}), Exception);

  // we revert to the default values
  re.reset();

  rack.nextBatch();
  ASSERT_EQ(6, re->fDiffs.size());

  {
    std::map<std::string, std::string> expected{};
    expected["/custom_properties/prop_float"] = "0.2->0.8@0";
    expected["/custom_properties/prop_bool"] = "true->false@0";
    expected["/custom_properties/prop_string"] = "Kooza?->abcd@0";
    expected["/device_host/sample_context"] = "1.0->0.0@0";
    expected["/user_samples/0/item"] = "UserSample{.fChannels=1,.fSampleRate=44100,.fFrameCount=6,.fResidentFrameCount=6,.fLoadStatus=resident,.fSamplePath=[/Private/mono_sample.data]}->"
                                       "UserSample{.fChannels=1,.fSampleRate=1,.fFrameCount=0,.fResidentFrameCount=0,.fLoadStatus=nil,.fSamplePath=[]}@0";
    expected["/user_samples/0/root_key"] = "62.0->60.0@0";

    ASSERT_EQ(expected, re->fDiffs);
  }
}

}