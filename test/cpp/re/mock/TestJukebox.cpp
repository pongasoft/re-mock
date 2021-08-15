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

#include <re/mock/Rack.h>
#include <gtest/gtest.h>
#include <algorithm>

namespace re::mock::Test {

using namespace mock;

// Jukebox.Basic
TEST(Jukebox, Basic)
{
  Rack rack{};

  struct Gain
  {
    TJBox_Float64 fVolume{};
  };

  auto rtc = R"(
rtc_bindings = {
  { source = "/custom_properties/prop_volume_ro", dest = "/global_rtc/new_gain_ro" },
  { source = "/custom_properties/prop_volume_rw", dest = "/global_rtc/new_gain_rw" },
}

global_rtc = {
  new_gain_ro = function(source_property_path, new_value)
    local new_no = jbox.make_native_object_ro("Gain", { new_value })
    jbox.store_property("/custom_properties/prop_gain_ro", new_no)
  end,

  new_gain_rw = function(source_property_path, new_value)
    local new_no = jbox.make_native_object_rw("Gain", { new_value })
    jbox.store_property("/custom_properties/prop_gain_rw", new_no)
  end,
}
)";

  auto c = Config::fromSkeleton()
    .mdef(Config::document_owner_property("prop_number_default", lua::jbox_number_property{}))
    .mdef(Config::document_owner_property("prop_float", lua::jbox_number_property{}.property_tag(100).default_value(0.7)))
    .mdef(Config::document_owner_property("prop_bool_default", lua::jbox_boolean_property{}))
    .mdef(Config::document_owner_property("prop_bool", lua::jbox_boolean_property{}.default_value(true)))
    .mdef(Config::document_owner_property("prop_string_default", lua::jbox_string_property{}))
    .mdef(Config::document_owner_property("prop_string", lua::jbox_string_property{}.default_value("abcd")))
    .mdef(Config::document_owner_property("prop_volume_ro", lua::jbox_number_property{}.default_value(0.8)))
    .mdef(Config::document_owner_property("prop_volume_rw", lua::jbox_number_property{}.default_value(0.9)))

    .mdef(Config::rtc_owner_property("prop_gain_default", lua::jbox_native_object{ }))
    .mdef(Config::rtc_owner_property("prop_gain",
                                     lua::jbox_native_object{}.default_value("Gain", { 0.7 })))
    .mdef(Config::rtc_owner_property("prop_gain_ro", lua::jbox_native_object{ }))
    .mdef(Config::rtc_owner_property("prop_gain_rw", lua::jbox_native_object{ }))

    .mdef(Config::rt_owner_property("prop_rt_string", lua::jbox_string_property{}.max_size(100)))

    .rtc_string(rtc)
    .rt([](Realtime &rt) {
      rt.create_native_object = [](const char iOperation[], const TJBox_Value iParams[], TJBox_UInt32 iCount) -> void * {
        if(std::strcmp(iOperation, "Gain") == 0)
          return new Gain{JBox_GetNumber(iParams[0])};

        return nullptr;
      };

      rt.destroy_native_object = Realtime::destroyer<Gain>("Gain");
    });

  auto re = rack.newExtension(c);

  re.use([]() {
    auto customProperties = JBox_GetMotherboardObjectRef("/custom_properties");

    ASSERT_FLOAT_EQ(0, JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_number_default"))));

    ASSERT_FLOAT_EQ(0.7, JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_float"))));
    JBox_StoreMOMProperty(JBox_MakePropertyRef(customProperties, "prop_float"), JBox_MakeNumber(0.87));
    ASSERT_FLOAT_EQ(0.87, JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_float"))));
    ASSERT_FLOAT_EQ(0.87, JBox_GetNumber(JBox_LoadMOMPropertyByTag(customProperties, 100)));
    JBox_StoreMOMPropertyByTag(customProperties, 100, JBox_MakeNumber(0.92));
    ASSERT_FLOAT_EQ(0.92, JBox_GetNumber(JBox_LoadMOMPropertyByTag(customProperties, 100)));
    JBox_StoreMOMPropertyAsNumber(customProperties, 100, 0.78);
    ASSERT_FLOAT_EQ(0.78, JBox_LoadMOMPropertyAsNumber(customProperties, 100));
    ASSERT_EQ(100, JBox_GetPropertyTag(JBox_MakePropertyRef(customProperties, "prop_float")));
    ASSERT_EQ(customProperties, JBox_FindPropertyByTag(customProperties, 100).fObject);
    ASSERT_STREQ("prop_float", JBox_FindPropertyByTag(customProperties, 100).fKey);

    ASSERT_FALSE(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_bool_default"))));
    ASSERT_TRUE(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_bool"))));

    {
      auto sValue = JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_string_default"));
      ASSERT_EQ(0, JBox_GetStringLength(sValue));
      std::array<char, 1> a{'K'};
      JBox_GetSubstring(sValue, 0, 0, a.data());
      ASSERT_EQ(0, a[0]);
    }

    {
      auto sValue = JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_string"));
      ASSERT_EQ(4, JBox_GetStringLength(sValue));
      std::array<char, 5> a{'K', 'K', 'K', 'K', 'K'};
      JBox_GetSubstring(sValue, 0, 0, a.data());
      ASSERT_EQ(0, a[0]);
      JBox_GetSubstring(sValue, 1, 2, a.data());
      ASSERT_EQ('b', a[0]);
      ASSERT_EQ('c', a[1]);
      ASSERT_EQ('\0', a[2]);
    }

    auto s = std::array<TJBox_UInt8, 4>{'a', 'b', 0, 'c'};
    JBox_SetRTStringData(JBox_MakePropertyRef(customProperties, "prop_rt_string"), s.size(), s.data());

    ASSERT_TRUE(JBox_GetNativeObjectRO(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_gain_default"))) == nullptr);
    ASSERT_TRUE(JBox_GetNativeObjectRW(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_gain_default"))) == nullptr);

    {
      auto gain = reinterpret_cast<Gain *>(JBox_GetNativeObjectRW(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_gain"))));
      ASSERT_FLOAT_EQ(0.7, gain->fVolume);
      gain->fVolume = 0.75;
    }

    {
      auto gain = reinterpret_cast<Gain const *>(JBox_GetNativeObjectRO(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_gain"))));
      ASSERT_FLOAT_EQ(0.75, gain->fVolume);
    }

    {
      auto gain = reinterpret_cast<Gain const *>(JBox_GetNativeObjectRO(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_gain_ro"))));
      ASSERT_FLOAT_EQ(0.8, gain->fVolume);
      ASSERT_THROW(JBox_GetNativeObjectRW(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_gain_ro"))), Exception);
    }

    {
      auto gain = reinterpret_cast<Gain const *>(JBox_GetNativeObjectRO(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_gain_rw"))));
      ASSERT_FLOAT_EQ(0.9, gain->fVolume);
    }

    {
      auto gain = reinterpret_cast<Gain const *>(JBox_GetNativeObjectRW(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_gain_rw"))));
      ASSERT_FLOAT_EQ(0.9, gain->fVolume);
    }


    ASSERT_THROW(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "invalid")), Exception);
    ASSERT_THROW(JBox_LoadMOMPropertyByTag(customProperties, 200), Exception);
    ASSERT_THROW(JBox_LoadMOMPropertyAsNumber(customProperties, 200), Exception);

    ASSERT_THROW(JBox_StoreMOMProperty(JBox_MakePropertyRef(customProperties, "invalid"), JBox_MakeNumber(0.87)), Exception);
    ASSERT_THROW(JBox_StoreMOMPropertyByTag(customProperties, 200, JBox_MakeNumber(0.87)), Exception);
    ASSERT_THROW(JBox_StoreMOMPropertyAsNumber(customProperties, 200, 0.87), Exception);

    ASSERT_THROW(JBox_GetPropertyTag(JBox_MakePropertyRef(customProperties, "invalid")), Exception);
    ASSERT_THROW(JBox_FindPropertyByTag(customProperties, 200), Exception);

    auto tooBig = std::array<TJBox_UInt8, 101>{};
    ASSERT_THROW(JBox_SetRTStringData(JBox_MakePropertyRef(customProperties, "prop_rt_string"), tooBig.size(), tooBig.data()), Exception);
  });

  std::string expected = "ab";
  expected.push_back('\0');
  expected.push_back('c');
  ASSERT_EQ(expected, re.getRTString("/custom_properties/prop_rt_string"));

  ASSERT_THROW(rack.newExtension(
    Config::fromSkeleton()
      .mdef(Config::rt_owner_property("prop_rt_string", lua::jbox_string_property{}.max_size(2049)))),
               Exception);
}

constexpr size_t DSP_BUFFER_SIZE = 64;

// Jukebox.AudioSocket
TEST(Jukebox, AudioSocket)
{
  Rack rack{};

  auto c = Config::fromSkeleton()
    .mdef(Config::audio_in("input_1"))
    .mdef(Config::audio_out("output_1"));

  auto re = rack.newExtension(c);

  re.use([](auto &motherboard) {
    // testing input
    {
      auto input1Ref = JBox_GetMotherboardObjectRef("/audio_inputs/input_1");

      ASSERT_EQ(DSP_BUFFER_SIZE,
                JBox_GetDSPBufferInfo(JBox_LoadMOMPropertyByTag(input1Ref, kJBox_AudioInputBuffer)).fSampleCount);
      ASSERT_EQ(DSP_BUFFER_SIZE,
                JBox_GetDSPBufferInfo(JBox_LoadMOMProperty(JBox_MakePropertyRef(input1Ref, "buffer"))).fSampleCount);

      auto input1Dsp = JBox_LoadMOMPropertyByTag(input1Ref, kJBox_AudioInputBuffer);
      Motherboard::DSPBuffer input1Buffer{};
      input1Buffer.fill(100);
      ASSERT_TRUE(
        std::all_of(input1Buffer.begin(), input1Buffer.end(), [](auto s) { return s == 100; })); // sanity check
      // testing reading full buffer
      JBox_GetDSPBufferData(input1Dsp, 0, DSP_BUFFER_SIZE, input1Buffer.data());
      ASSERT_TRUE(std::all_of(input1Buffer.begin(), input1Buffer.end(), [](auto s) { return s == 0; }));

      // testing reading partial buffer [ ....., x, y, z, .... ] => [x, y, z, 100, 100...]
      Motherboard::DSPBuffer buf{};
      for(int i = 0; i < buf.size(); i++)
        buf[i] = i + 1;
      motherboard.setDSPBuffer("/audio_inputs/input_1", buf);

      input1Buffer.fill(100);
      JBox_GetDSPBufferData(input1Dsp, 10, 20, input1Buffer.data());
      for(int i = 0; i < 10; i++)
        ASSERT_FLOAT_EQ(buf[i + 10], input1Buffer[i]);
      ASSERT_TRUE(std::all_of(input1Buffer.begin() + 10, input1Buffer.end(), [](auto s) { return s == 100; }));
    }

    // testing output
    {
      auto output1Ref = JBox_GetMotherboardObjectRef("/audio_outputs/output_1");

      ASSERT_EQ(DSP_BUFFER_SIZE,
                JBox_GetDSPBufferInfo(JBox_LoadMOMPropertyByTag(output1Ref, kJBox_AudioOutputBuffer)).fSampleCount);
      ASSERT_EQ(DSP_BUFFER_SIZE,
                JBox_GetDSPBufferInfo(JBox_LoadMOMProperty(JBox_MakePropertyRef(output1Ref, "buffer"))).fSampleCount);
      auto output1Dsp = JBox_LoadMOMPropertyByTag(output1Ref, kJBox_AudioOutputBuffer);
      Motherboard::DSPBuffer output1Buffer{};
      output1Buffer.fill(100);

      // we make sure that the output buffer is properly initialized with 0
      auto o1 = motherboard.getDSPBuffer("/audio_outputs/output_1");
      ASSERT_TRUE(std::all_of(o1.begin(), o1.end(), [](auto s) { return s == 0; }));

      // testing writing full buffer
      JBox_SetDSPBufferData(output1Dsp, 0, DSP_BUFFER_SIZE, output1Buffer.data());
      o1 = motherboard.getDSPBuffer("/audio_outputs/output_1");
      ASSERT_TRUE(std::all_of(o1.begin(), o1.end(), [](auto s) { return s == 100; }));

      // testing writing partial buffer [x, y, z, ... ] -> [...., x, y, z, ...]
      for(int i = 0; i < output1Buffer.size(); i++)
        output1Buffer[i] = i + 1;
      JBox_SetDSPBufferData(output1Dsp, 10, 20, output1Buffer.data());

      o1 = motherboard.getDSPBuffer("/audio_outputs/output_1");
      ASSERT_TRUE(std::all_of(o1.begin(), o1.begin() + 10, [](auto s) { return s == 100; }));
      for(int i = 0; i < 10; i++)
        ASSERT_FLOAT_EQ(o1[i + 10], output1Buffer[i]);
      ASSERT_TRUE(std::all_of(o1.begin() + 20, o1.end(), [](auto s) { return s == 100; }));
    }
  });
}

}