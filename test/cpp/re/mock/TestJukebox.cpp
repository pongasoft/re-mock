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

#include <re/mock/Motherboard.h>
#include <gtest/gtest.h>

namespace re::mock::Test {

using namespace mock;

// Jukebox.Basic
TEST(Jukebox, Basic)
{
  auto motherboard = Motherboard::init([](auto &mdef, auto &rtc) {
    mdef.document_owner.properties["prop_number_default"]  = jbox.number();
    mdef.document_owner.properties["prop_float"]           = jbox.number<float>({.property_tag = 100, .default_value = 0.7});
    mdef.document_owner.properties["prop_float_2"]         = jbox.number<float>(0.8);
    mdef.document_owner.properties["prop_bool_default"]    = jbox.boolean();
    mdef.document_owner.properties["prop_bool"]            = jbox.boolean({.default_value = true});
    mdef.document_owner.properties["prop_bool_2"]          = jbox.boolean(true);
    mdef.document_owner.properties["prop_generic_default"] = jbox.property();
    mdef.document_owner.properties["prop_generic"]         = jbox.property(JBox_MakeNumber(0.2));
    mdef.document_owner.properties["prop_generic_2"]       = jbox.property({.default_value = JBox_MakeNumber(0.3)});
  });

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
}

}