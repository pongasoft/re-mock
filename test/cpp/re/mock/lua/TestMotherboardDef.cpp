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

namespace re::mock::lua::Test {

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

}
