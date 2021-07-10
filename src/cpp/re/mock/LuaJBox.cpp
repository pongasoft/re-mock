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

#include "LuaJBox.h"
#include "Rack.h"

namespace re::mock {

//------------------------------------------------------------------------
// jbox instance to be accessible from all code
//------------------------------------------------------------------------
LuaJbox jbox{};

//------------------------------------------------------------------------
// LuaJbox::load_property
//------------------------------------------------------------------------
TJBox_Value LuaJbox::load_property(std::string const &iPropertyPath)
{
  return re::mock::Rack::currentMotherboard().getValue(iPropertyPath);
}

//------------------------------------------------------------------------
// LuaJbox::make_native_object_rw
//------------------------------------------------------------------------
TJBox_Value LuaJbox::make_native_object_rw(std::string const &iOperation, std::vector<TJBox_Value> const &iParams)
{
  return re::mock::Rack::currentMotherboard().makeNativeObjectRW(iOperation, iParams);
}

//------------------------------------------------------------------------
// LuaJbox::store_property
//------------------------------------------------------------------------
void LuaJbox::store_property(std::string const &iPropertyPath, TJBox_Value const &iValue)
{
  return re::mock::Rack::currentMotherboard().setValue(iPropertyPath, iValue);
}

}