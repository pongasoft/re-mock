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

#include <re/mock/lua/RealtimeController.h>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

namespace re::mock::lua::Test {

using namespace testing;

std::string getResourceFile(std::string iFilename);

// MotherboardDef.Empty
TEST(RealtimeController, Empty)
{
  auto def = RealtimeController::fromString("print('hello from lua')");

  ASSERT_EQ(def->getStackString(), "<empty>");

  ASSERT_EQ(def->getBindings().size(), 0);
  ASSERT_EQ(def->getRTInputSetupNotify().size(), 0);


  ASSERT_EQ(def->getStackString(), "<empty>");
}

// MotherboardDef.BlankEffect
TEST(RealtimeController, BlankEffect)
{
  // file generated from re-quickstart for a blank effect
  auto def = RealtimeController::fromFile(getResourceFile("blank_effect-realtime_controller.lua"));

  ASSERT_EQ(def->getStackString(), "<empty>");

  ASSERT_EQ(def->getBindings().size(), 1);
  ASSERT_EQ("init_instance", def->getBindings()["/environment/system_sample_rate"]);

  ASSERT_EQ(def->getRTInputSetupNotify().size(), 4);
  ASSERT_THAT(def->getRTInputSetupNotify(), UnorderedElementsAre("/audio_inputs/MainInLeft/connected",
                                                                 "/audio_inputs/MainInRight/connected",
                                                                 "/audio_outputs/MainOutLeft/connected",
                                                                 "/audio_outputs/MainOutRight/connected"));


  ASSERT_EQ(def->getStackString(), "<empty>");
}

}
