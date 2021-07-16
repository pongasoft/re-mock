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

namespace re::mock::Test {

using namespace mock;

// Jukebox.Basic
TEST(Jukebox, Basic)
{
  Rack rack{};

  Config c = Config::with([](auto &def, auto &rtc, auto &rt) {
    def.document_owner.properties["prop_number_default"]  = jbox.number();
    def.document_owner.properties["prop_float"]           = jbox.number<float>({.property_tag = 100, .default_value = 0.7});
    def.document_owner.properties["prop_float_2"]         = jbox.number<float>(0.8);
    def.document_owner.properties["prop_bool_default"]    = jbox.boolean();
    def.document_owner.properties["prop_bool"]            = jbox.boolean({.default_value = true});
    def.document_owner.properties["prop_bool_2"]          = jbox.boolean(true);
    def.document_owner.properties["prop_generic_default"] = jbox.property();
    def.document_owner.properties["prop_generic"]         = jbox.property(JBox_MakeNumber(0.2));
    def.document_owner.properties["prop_generic_2"]       = jbox.property({.default_value = JBox_MakeNumber(0.3)});
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

    ASSERT_FLOAT_EQ(0.8, JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_float_2"))));

    ASSERT_FALSE(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_bool_default"))));
    ASSERT_TRUE(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_bool"))));

    ASSERT_FLOAT_EQ(0, JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_generic_default"))));
    ASSERT_FLOAT_EQ(0.2, JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_generic"))));
    ASSERT_FLOAT_EQ(0.3, JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "prop_generic_2"))));

    ASSERT_THROW(JBox_LoadMOMProperty(JBox_MakePropertyRef(customProperties, "invalid")), Error);
    ASSERT_THROW(JBox_LoadMOMPropertyByTag(customProperties, 200), Error);
    ASSERT_THROW(JBox_LoadMOMPropertyAsNumber(customProperties, 200), Error);

    ASSERT_THROW(JBox_StoreMOMProperty(JBox_MakePropertyRef(customProperties, "invalid"), JBox_MakeNumber(0.87)), Error);
    ASSERT_THROW(JBox_StoreMOMPropertyByTag(customProperties, 200, JBox_MakeNumber(0.87)), Error);
    ASSERT_THROW(JBox_StoreMOMPropertyAsNumber(customProperties, 200, 0.87), Error);

    ASSERT_THROW(JBox_GetPropertyTag(JBox_MakePropertyRef(customProperties, "invalid")), Error);
    ASSERT_THROW(JBox_FindPropertyByTag(customProperties, 200), Error);
  });
}

constexpr size_t DSP_BUFFER_SIZE = 64;

// Jukebox.AudioSocket
TEST(Jukebox, AudioSocket)
{
  Rack rack{};

  Config c = Config::with([](auto &def, auto &rtc, auto &rt) {
    def.audio_inputs["input_1"] = jbox.audio_input();
    def.audio_outputs["output_1"] = jbox.audio_output();
  });

  auto re = rack.newExtension(c);

  re.use([](auto motherboard) {
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
      motherboard->setDSPBuffer("/audio_inputs/input_1", buf);

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
      auto o1 = motherboard->getDSPBuffer("/audio_outputs/output_1");
      ASSERT_TRUE(std::all_of(o1.begin(), o1.end(), [](auto s) { return s == 0; }));

      // testing writing full buffer
      JBox_SetDSPBufferData(output1Dsp, 0, DSP_BUFFER_SIZE, output1Buffer.data());
      o1 = motherboard->getDSPBuffer("/audio_outputs/output_1");
      ASSERT_TRUE(std::all_of(o1.begin(), o1.end(), [](auto s) { return s == 100; }));

      // testing writing partial buffer [x, y, z, ... ] -> [...., x, y, z, ...]
      for(int i = 0; i < output1Buffer.size(); i++)
        output1Buffer[i] = i + 1;
      JBox_SetDSPBufferData(output1Dsp, 10, 20, output1Buffer.data());

      o1 = motherboard->getDSPBuffer("/audio_outputs/output_1");
      ASSERT_TRUE(std::all_of(o1.begin(), o1.begin() + 10, [](auto s) { return s == 100; }));
      for(int i = 0; i < 10; i++)
        ASSERT_FLOAT_EQ(o1[i + 10], output1Buffer[i]);
      ASSERT_TRUE(std::all_of(o1.begin() + 20, o1.end(), [](auto s) { return s == 100; }));
    }
  });
}

}