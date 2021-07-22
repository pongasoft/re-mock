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

#pragma once
#ifndef __Pongasoft_re_mock_motherboard_impl_h__
#define __Pongasoft_re_mock_motherboard_impl_h__

namespace re::mock {

class Motherboard;

namespace impl {

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

struct JboxProperty
{
  JboxProperty(TJBox_PropertyRef const &iPropertyRef, std::string const &iPropertyPath, PropertyOwner iOwner, TJBox_Value const &iInitialValue, TJBox_Tag iTag);

  inline TJBox_Value loadValue() const { return fValue; };
  std::optional<TJBox_PropertyDiff> storeValue(TJBox_Value const &iValue);

  TJBox_PropertyDiff watchForChange();

  const TJBox_PropertyRef fPropertyRef;
  const std::string fPropertyPath;
  const PropertyOwner fOwner;
  const TJBox_Tag fTag;

protected:
  TJBox_Value fValue;
  bool fWatched{};
};

struct JboxObject
{
  explicit JboxObject(std::string const &iObjectPath, TJBox_ObjectRef iObjectRef);

  ~JboxObject() = default;

  TJBox_Value loadValue(std::string const &iPropertyName) const;
  TJBox_Value loadValue(TJBox_Tag iPropertyTag) const;
  std::optional<TJBox_PropertyDiff> storeValue(std::string const &iPropertyName, TJBox_Value const &iValue);
  std::optional<TJBox_PropertyDiff> storeValue(TJBox_Tag iPropertyTag, TJBox_Value const &iValue);
  TJBox_PropertyDiff watchPropertyForChange(std::string const &iPropertyName);

  const std::string fObjectPath;
  const TJBox_ObjectRef fObjectRef;

  friend class re::mock::Motherboard;

protected:
  void addProperty(std::string iPropertyName, PropertyOwner iOwner, TJBox_Value const &iInitialValue, TJBox_Tag iPropertyTag);
  JboxProperty *getProperty(std::string const &iPropertyName) const;
  JboxProperty *getProperty(TJBox_Tag iPropertyTag) const;

protected:
  std::map<std::string, std::unique_ptr<JboxProperty>> fProperties{};
};

struct NativeObject
{
  enum AccessMode { kReadOnly, kReadWrite };

  void *fNativeObject{};
  std::string fOperation{};
  std::vector<TJBox_Value> fParams{};
  Realtime::destroy_native_object_t fDeleter{};
  AccessMode fAccessMode{kReadOnly};

  ~NativeObject() {
    if(fDeleter)
      fDeleter(fOperation.c_str(), fParams.data(), fParams.size(), fNativeObject);
  }
};

}
}

#endif //__Pongasoft_re_mock_motherboard_impl_h__
