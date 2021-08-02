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

  ASSERT_EQ(0, def->getAudioInputs()->fNames.size());
  ASSERT_EQ(0, def->getAudioOutputs()->fNames.size());
  ASSERT_EQ(0, def->getCVInputs()->fNames.size());
  ASSERT_EQ(0, def->getCVOutputs()->fNames.size());
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

  ASSERT_THAT(def->getAudioInputs()->fNames, UnorderedElementsAre("MainInLeft", "MainInRight"));
  ASSERT_THAT(def->getAudioOutputs()->fNames, UnorderedElementsAre("MainOutLeft", "MainOutRight"));
  ASSERT_EQ(0, def->getCVInputs()->fNames.size());
  ASSERT_EQ(0, def->getCVOutputs()->fNames.size());
  auto customProperties = def->getCustomProperties();
  ASSERT_EQ(0, customProperties->document_owner.size());
  ASSERT_EQ(1, customProperties->rtc_owner.size());
  ASSERT_TRUE(std::get<std::shared_ptr<lua::jbox_native_object>>(customProperties->rtc_owner["instance"]) != nullptr);
  ASSERT_EQ(0, customProperties->rt_owner.size());

  ASSERT_EQ(def->getStackString(), "<empty>");
}

// MotherboardDef.All
TEST(MotherboardDef, All)
{
  // file generated from re-quickstart for a blank effect
  auto def = MotherboardDef::fromFile(getResourceFile("all-motherboard_def.lua"));

  ASSERT_EQ(def->getStackString(), "<empty>");

  ASSERT_THAT(def->getAudioInputs()->fNames, UnorderedElementsAre("au_in"));
  ASSERT_THAT(def->getAudioOutputs()->fNames, UnorderedElementsAre("au_out"));
  ASSERT_THAT(def->getCVInputs()->fNames, UnorderedElementsAre("cv_in"));
  ASSERT_THAT(def->getCVOutputs()->fNames, UnorderedElementsAre("cv_out"));
  auto customProperties = def->getCustomProperties();

  // document_owner
  ASSERT_EQ(2, customProperties->document_owner.size());
  {
    auto ptr = std::get<std::shared_ptr<jbox_boolean_property>>(customProperties->document_owner["doc_boolean"]);
    ASSERT_EQ(ptr->fPropertyTag, 100);
    ASSERT_TRUE(ptr->fDefaultValue);
  }
  {
    auto ptr = std::get<std::shared_ptr<jbox_number_property>>(customProperties->document_owner["doc_number"]);
    ASSERT_EQ(ptr->fPropertyTag, 101);
    ASSERT_FLOAT_EQ(ptr->fDefaultValue, 3);
  }

  // rtc_owner
  ASSERT_EQ(2, customProperties->rtc_owner.size());
  {
    auto ptr = std::get<std::shared_ptr<jbox_native_object>>(customProperties->rtc_owner["instance"]);
    ASSERT_EQ(ptr->fDefaultValue.operation, "");
    ASSERT_EQ(ptr->fDefaultValue.params.size(), 0);
  }
  {
    auto ptr = std::get<std::shared_ptr<jbox_native_object>>(customProperties->rtc_owner["instance_with_default"]);
    ASSERT_EQ(ptr->fDefaultValue.operation, "Operation");
    ASSERT_EQ(ptr->fDefaultValue.params.size(), 3);
    ASSERT_FLOAT_EQ(std::get<TJBox_Float64>(ptr->fDefaultValue.params[0]), 0.5);
    ASSERT_EQ(std::get<bool>(ptr->fDefaultValue.params[1]), true);
    ASSERT_FLOAT_EQ(std::get<TJBox_Float64>(ptr->fDefaultValue.params[2]), 48000);
  }

  // rt_owner
  ASSERT_EQ(1, customProperties->rt_owner.size());
  {
    auto ptr = std::get<std::shared_ptr<jbox_number_property>>(customProperties->rt_owner["rt_number"]);
    ASSERT_EQ(ptr->fPropertyTag, 102);
    ASSERT_EQ(ptr->fDefaultValue, 0);
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
  ASSERT_THAT(def.getAudioOutputs()->fNames, UnorderedElementsAre("au_out1", "au_out2"));
}

}
