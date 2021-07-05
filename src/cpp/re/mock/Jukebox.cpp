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

#include <algorithm>
#include <logging/logging.h>
#include "Jukebox.h"
#include "Motherboard.h"

namespace re::mock::impl {

template<typename T>
union JboxSecretInternal {
  T fValue;
  TJBox_UInt8 fSecret[15];
};

template<typename T>
TJBox_Value jbox_make_value(TJBox_ValueType iValueType, T iValue)
{
  TJBox_Value res;

  res.fSecret[0] = iValueType;
  JboxSecretInternal<T> secret{};
  secret.fValue = iValue;
  std::copy(std::begin(secret.fSecret), std::end(secret.fSecret), std::begin(res.fSecret) + 1);

  return res;
};

template<typename T>
T jbox_get_value(TJBox_ValueType iValueType, TJBox_Value const &iJboxValue)
{
  CHECK_F(iJboxValue.fSecret[0] == iValueType);
  JboxSecretInternal<T> secret{};
  std::copy(std::begin(iJboxValue.fSecret) + 1, std::end(iJboxValue.fSecret), std::begin(secret.fSecret));
  return secret.fValue;
}

}

TJBox_ValueType JBox_GetType(TJBox_Value iValue)
{
  return static_cast<TJBox_ValueType>(iValue.fSecret[0]);
}

TJBox_Value JBox_MakeNil()
{
  TJBox_Value res;
  res.fSecret[0] = TJBox_ValueType::kJBox_Nil;
  return res;
}

TJBox_Bool JBox_GetBoolean(TJBox_Value iValue)
{
  return re::mock::impl::jbox_get_value<TJBox_Bool>(TJBox_ValueType::kJBox_Boolean, iValue);
}

TJBox_Value JBox_MakeBoolean(TJBox_Bool iBoolean)
{
  return re::mock::impl::jbox_make_value<TJBox_Bool>(TJBox_ValueType::kJBox_Boolean, iBoolean);
}

TJBox_Float64 JBox_GetNumber(TJBox_Value iValue)
{
  return re::mock::impl::jbox_get_value<TJBox_Float64>(TJBox_ValueType::kJBox_Number, iValue);
}

TJBox_Value JBox_MakeNumber(TJBox_Float64 iNumber)
{
  return re::mock::impl::jbox_make_value<TJBox_Float64>(TJBox_ValueType::kJBox_Number, iNumber);
}


TJBox_PropertyRef JBox_MakePropertyRef(TJBox_ObjectRef iObject, const TJBox_PropertyKey iKey)
{
  TJBox_PropertyRef res{};
  res.fObject = iObject;
  // implementation note: for some reason I don't understand, std::begin(iKey) and std::end(iKey) do not work
  std::copy(iKey, iKey + kJBox_MaxPropertyNameLen + 1, std::begin(res.fKey));
  return res;
}

TJBox_ObjectRef JBox_GetMotherboardObjectRef(const TJBox_ObjectName iMOMPath)
{
  return re::mock::Motherboard::instance().getObjectRef(iMOMPath);
}

TJBox_Value JBox_LoadMOMProperty(TJBox_PropertyRef iProperty)
{
  return re::mock::Motherboard::instance().loadProperty(iProperty);
}

void JBox_StoreMOMProperty(TJBox_PropertyRef iProperty, TJBox_Value iValue)
{
  return re::mock::Motherboard::instance().storeProperty(iProperty, iValue);
}
