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
#include <gtest/gtest.h>

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

}