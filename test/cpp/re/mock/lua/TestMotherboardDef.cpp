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
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <re_mock_build.h>
#include <Jukebox.h>

namespace re::mock::lua::Test {

using namespace testing;

std::string getResourceFile(std::string iFilename)
{
  return fmt::path(RE_MOCK_PROJECT_DIR, "test", "resources", "re", "mock", "lua", iFilename);
}

// MotherboardDef.Empty
TEST(MotherboardDef, Empty)
{
  auto def = MotherboardDef::fromString("print('hello from lua')");

  ASSERT_EQ(def->getStackString(), "<empty>");

  ASSERT_EQ(0, def->getAudioInputs()->names.size());
  ASSERT_EQ(0, def->getAudioOutputs()->names.size());
  ASSERT_EQ(0, def->getCVInputs()->names.size());
  ASSERT_EQ(0, def->getCVOutputs()->names.size());
  auto customProperties = def->getCustomProperties();
  ASSERT_EQ(0, customProperties->document_owner.size());
  ASSERT_EQ(0, customProperties->rtc_owner.size());
  ASSERT_EQ(0, customProperties->rt_owner.size());

  ASSERT_EQ(def->getStackString(), "<empty>");
}

// MotherboardDef.BlankEffect
TEST(MotherboardDef, BlankEffect)
{
  // file generated from re-quickstart for a blank effect
  auto def = MotherboardDef::fromFile(getResourceFile("blank_effect-motherboard_def.lua"));

  ASSERT_EQ(def->getStackString(), "<empty>");

  ASSERT_THAT(def->getAudioInputs()->names, UnorderedElementsAre("MainInLeft", "MainInRight"));
  ASSERT_THAT(def->getAudioOutputs()->names, UnorderedElementsAre("MainOutLeft", "MainOutRight"));
  ASSERT_EQ(0, def->getCVInputs()->names.size());
  ASSERT_EQ(0, def->getCVOutputs()->names.size());
  auto customProperties = def->getCustomProperties();
  ASSERT_EQ(0, customProperties->document_owner.size());
  ASSERT_EQ(1, customProperties->rtc_owner.size());
  ASSERT_EQ(customProperties->rtc_owner["instance"]->getType(), jbox_object::Type::NATIVE_OBJECT);
  ASSERT_EQ(0, customProperties->rt_owner.size());

  ASSERT_EQ(def->getStackString(), "<empty>");
}

// MotherboardDef.All
TEST(MotherboardDef, All)
{
  // file generated from re-quickstart for a blank effect
  auto def = MotherboardDef::fromFile(getResourceFile("all-motherboard_def.lua"));

  ASSERT_EQ(def->getStackString(), "<empty>");

  ASSERT_THAT(def->getAudioInputs()->names, UnorderedElementsAre("au_in"));
  ASSERT_THAT(def->getAudioOutputs()->names, UnorderedElementsAre("au_out"));
  ASSERT_THAT(def->getCVInputs()->names, UnorderedElementsAre("cv_in"));
  ASSERT_THAT(def->getCVOutputs()->names, UnorderedElementsAre("cv_out"));
  auto customProperties = def->getCustomProperties();

  // document_owner
  ASSERT_EQ(2, customProperties->document_owner.size());
  {
    auto ptr = customProperties->document_owner["doc_boolean"]->withType<jbox_boolean_property>();
    ASSERT_EQ(ptr->getType(), jbox_object::Type::BOOLEAN);
    ASSERT_EQ(ptr->property_tag, 100);
    ASSERT_TRUE(ptr->default_value);
    ASSERT_TRUE(JBox_GetBoolean(ptr->getDefaultValue()));
  }
  {
    auto ptr = customProperties->document_owner["doc_number"]->withType<jbox_number_property>();
    ASSERT_EQ(ptr->getType(), jbox_object::Type::NUMBER);
    ASSERT_EQ(ptr->property_tag, 101);
    ASSERT_FLOAT_EQ(ptr->default_value, 3);
    ASSERT_FLOAT_EQ(JBox_GetNumber(ptr->getDefaultValue()), 3);
  }

  // rtc_owner
  ASSERT_EQ(2, customProperties->rtc_owner.size());
  {
    auto ptr = customProperties->rtc_owner["instance"]->withType<jbox_native_object>();
    ASSERT_EQ(ptr->getType(), jbox_object::Type::NATIVE_OBJECT);
    ASSERT_EQ(ptr->default_value.operation, "");
    ASSERT_EQ(ptr->default_value.params.size(), 0);
  }
  {
    auto ptr = customProperties->rtc_owner["instance_with_default"]->withType<jbox_native_object>();
    ASSERT_EQ(ptr->getType(), jbox_object::Type::NATIVE_OBJECT);
    ASSERT_EQ(ptr->default_value.operation, "Operation");
    ASSERT_EQ(ptr->default_value.params.size(), 3);
    ASSERT_FLOAT_EQ(JBox_GetNumber(ptr->default_value.params[0]), 0.5);
    ASSERT_EQ(JBox_GetBoolean(ptr->default_value.params[1]), true);
    ASSERT_FLOAT_EQ(JBox_GetNumber(ptr->default_value.params[2]), 48000);
  }

  // rt_owner
  ASSERT_EQ(1, customProperties->rt_owner.size());
  {
    auto ptr = customProperties->rt_owner["rt_number"]->withType<jbox_number_property>();
    ASSERT_EQ(ptr->getType(), jbox_object::Type::NUMBER);
    ASSERT_EQ(ptr->property_tag, 102);
    ASSERT_EQ(ptr->default_value, 0);
    ASSERT_FLOAT_EQ(JBox_GetNumber(ptr->getDefaultValue()), 0);
  }

  ASSERT_EQ(def->getStackString(), "<empty>");
}

// MotherboardDef.Multiple
TEST(MotherboardDef, Multiple)
{
  MotherboardDef def{};
  def.loadString("print('first load')");
  def.loadString("audio_outputs={}");
  def.loadString(R"(audio_outputs["au_out1"] = jbox.audio_output { })");
  def.loadString(R"(audio_outputs["au_out2"] = jbox.audio_output { })");
  ASSERT_THAT(def.getAudioOutputs()->names, UnorderedElementsAre("au_out1", "au_out2"));
}

}
