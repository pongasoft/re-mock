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

#include "Errors.h"

namespace re::mock {

class Motherboard;

namespace impl {
struct String;
struct NativeObject;
struct Blob;
constexpr static size_t DSP_BUFFER_SIZE = 64;
using DSPBuffer = std::array<TJBox_AudioSample, DSP_BUFFER_SIZE>;
}

class JboxValue
{
public:

  friend class Motherboard;

  TJBox_ValueType getValueType() const { return fValueType; }
  TJBox_UInt64 getUniqueId() const { return reinterpret_cast<TJBox_UInt64>(this); }

  TJBox_Float64 getNumber() const { return std::get<TJBox_Float64>(fMotherboardValue); }
  bool getBoolean() const { return std::get<bool>(fMotherboardValue); }
  impl::String const &getString() const { return *std::get<std::unique_ptr<impl::String>>(fMotherboardValue); }
  impl::NativeObject const &getNativeObject() const { return *std::get<std::unique_ptr<impl::NativeObject>>(fMotherboardValue); }
  impl::Blob const &getBlob() const { return *std::get<std::unique_ptr<impl::Blob>>(fMotherboardValue); }
  impl::DSPBuffer const &getDSPBuffer() const { return *std::get<std::unique_ptr<impl::DSPBuffer>>(fMotherboardValue); }

private:
  struct Nil {};
  struct Incompatible {};

  using motherboard_value_t = std::variant<
    Nil,
    TJBox_Float64,
    bool,
    Incompatible,
    std::unique_ptr<impl::String>,
    std::unique_ptr<impl::NativeObject>,
    std::unique_ptr<impl::Blob>,
    std::unique_ptr<impl::DSPBuffer>
  >;

private:
  impl::DSPBuffer &getDSPBuffer() { return *std::get<std::unique_ptr<impl::DSPBuffer>>(fMotherboardValue); }
  impl::Blob &getBlob() { return *std::get<std::unique_ptr<impl::Blob>>(fMotherboardValue); }
  impl::String &getString() { return *std::get<std::unique_ptr<impl::String>>(fMotherboardValue); }

private:
  TJBox_ValueType fValueType{kJBox_Nil};
  motherboard_value_t fMotherboardValue{Nil{}};
};

namespace impl {

template<typename T>
union JboxSecretInternal {
  static_assert(sizeof(T) < sizeof(TJBox_UInt8) * 15);
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
}

template<typename T>
T jbox_get_value(TJBox_ValueType iValueType, TJBox_Value const &iJboxValue)
{
  RE_MOCK_ASSERT(iJboxValue.fSecret[0] == iValueType);
  JboxSecretInternal<T> secret{};
  std::copy(std::begin(iJboxValue.fSecret) + 1, std::end(iJboxValue.fSecret), std::begin(secret.fSecret));
  return secret.fValue;
}

struct JboxPropertyDiff
{
  std::shared_ptr<JboxValue> fPreviousValue;
  std::shared_ptr<JboxValue> fCurrentValue;
  TJBox_PropertyRef fPropertyRef;
  TJBox_Tag fPropertyTag;
  TJBox_UInt16 fAtFrameIndex;
  int fInsertIndex{};
};

struct JboxProperty
{
  JboxProperty(TJBox_PropertyRef const &iPropertyRef,
               std::string iPropertyPath,
               TJBox_ValueType iValueType,
               PropertyOwner iOwner,
               std::shared_ptr<JboxValue> iInitialValue,
               TJBox_Tag iTag,
               lua::EPersistence iPersistence);

  inline std::shared_ptr<const JboxValue> loadValue() const { return fValue; };
  inline std::shared_ptr<JboxValue> loadValue() { return fValue; };
  JboxPropertyDiff storeValue(std::shared_ptr<JboxValue> iValue);

  JboxPropertyDiff watchForChange();
  bool isWatched() const { return fWatched; }

  const TJBox_PropertyRef fPropertyRef;
  const std::string fPropertyPath;
  const PropertyOwner fOwner;
  const TJBox_Tag fTag;
  const lua::EPersistence fPersistence;

protected:
  const TJBox_ValueType fValueType;
  std::shared_ptr<JboxValue> fInitialValue;
  std::shared_ptr<JboxValue> fValue;
  bool fWatched{};
};

struct JboxObject
{
  explicit JboxObject(std::string const &iObjectPath, TJBox_ObjectRef iObjectRef);

  ~JboxObject() = default;

  std::shared_ptr<const JboxValue> loadValue(std::string const &iPropertyName) const;
  std::shared_ptr<const JboxValue> loadValue(TJBox_Tag iPropertyTag) const;
  std::shared_ptr<JboxValue> loadValue(std::string const &iPropertyName);
  std::shared_ptr<JboxValue> loadValue(TJBox_Tag iPropertyTag);
  impl::JboxPropertyDiff storeValue(std::string const &iPropertyName, std::shared_ptr<JboxValue> iValue);
  impl::JboxPropertyDiff storeValue(TJBox_Tag iPropertyTag, std::shared_ptr<JboxValue> iValue);
  std::vector<impl::JboxPropertyDiff> watchAllPropertiesForChange();
  impl::JboxPropertyDiff watchPropertyForChange(std::string const &iPropertyName);

  const std::string fObjectPath;
  const TJBox_ObjectRef fObjectRef;

  friend class re::mock::Motherboard;

protected:
  void addProperty(const std::string& iPropertyName,
                   PropertyOwner iOwner,
                   TJBox_ValueType iValueType,
                   std::shared_ptr<JboxValue> iInitialValue,
                   TJBox_Tag iPropertyTag,
                   lua::EPersistence iPersistence = lua::EPersistence::kNone);

  void addProperty(const std::string& iPropertyName,
                   PropertyOwner iOwner,
                   std::shared_ptr<JboxValue> iInitialValue,
                   TJBox_Tag iPropertyTag,
                   lua::EPersistence iPersistence = lua::EPersistence::kNone);

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
  Realtime::destroy_native_object_t fDeleter{};
  AccessMode fAccessMode{kReadOnly};

  ~NativeObject() {
    if(fDeleter)
    {
      fDeleter(fOperation.c_str(), fNativeObject);
    }
  }
};

struct String
{
  int fMaxSize{};
  std::string fValue{};
  inline bool isRTString() const { return fMaxSize > 0; }
};

struct Blob
{
  TJBox_SizeT fResidentSize{};
  std::vector<char> fData{};
};

}
}

#endif //__Pongasoft_re_mock_motherboard_impl_h__
