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
struct Sample;
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
  impl::Sample const &getSample() const { return *std::get<std::unique_ptr<impl::Sample>>(fMotherboardValue); }
  impl::DSPBuffer const &getDSPBuffer() const { return *std::get<std::unique_ptr<impl::DSPBuffer>>(fMotherboardValue); }

  bool isNil() const { return std::holds_alternative<nil_t>(fMotherboardValue); }

private:
  struct nil_t {};
  struct incompatible_t {};

  using motherboard_value_t = std::variant<
    nil_t,
    TJBox_Float64,
    bool,
    incompatible_t,
    std::unique_ptr<impl::String>,
    std::unique_ptr<impl::NativeObject>,
    std::unique_ptr<impl::Blob>,
    std::unique_ptr<impl::Sample>,
    std::unique_ptr<impl::DSPBuffer>
  >;

private:
  impl::DSPBuffer &getDSPBuffer() { return *std::get<std::unique_ptr<impl::DSPBuffer>>(fMotherboardValue); }
  impl::Blob &getBlob() { return *std::get<std::unique_ptr<impl::Blob>>(fMotherboardValue); }
  impl::Sample &getSample() { return *std::get<std::unique_ptr<impl::Sample>>(fMotherboardValue); }
  impl::String &getString() { return *std::get<std::unique_ptr<impl::String>>(fMotherboardValue); }

private:
  TJBox_ValueType fValueType{kJBox_Nil};
  motherboard_value_t fMotherboardValue{nil_t{}};
};

struct JboxPropertyInfo
{
  TJBox_PropertyRef fPropertyRef;
  std::string fPropertyPath;
  TJBox_ValueType fValueType;
  int fStepCount;
  PropertyOwner fOwner;
  TJBox_Tag fTag;
  lua::EPersistence fPersistence;
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

using value_validator_t = std::function<bool(JboxValue const &)>;

struct JboxProperty
{
  JboxProperty(TJBox_PropertyRef const &iPropertyRef,
               std::string iPropertyPath,
               TJBox_ValueType iValueType,
               int iStepCount,
               PropertyOwner iOwner,
               std::shared_ptr<JboxValue> iInitialValue,
               TJBox_Tag iTag,
               value_validator_t iValueValidator,
               lua::EPersistence iPersistence);

  inline std::shared_ptr<const JboxValue> loadValue() const { return fValue; };
  inline std::shared_ptr<JboxValue> loadValue() { return fValue; };
  JboxPropertyDiff storeValue(std::shared_ptr<JboxValue> iValue);

  JboxPropertyDiff watchForChange();
  bool isWatched() const { return fWatched; }

  JboxPropertyInfo const &getInfo() const { return fInfo; }

  const JboxPropertyInfo fInfo;

protected:
  void validateValue(std::shared_ptr<JboxValue> const &iValue);

protected:
  std::shared_ptr<JboxValue> fInitialValue;
  std::shared_ptr<JboxValue> fValue;
  value_validator_t fValueValidator;
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
  bool hasProperty(TJBox_Tag iPropertyTag) const;

  const std::string fObjectPath;
  const TJBox_ObjectRef fObjectRef;

  friend class re::mock::Motherboard;

protected:
  void addProperty(const std::string &iPropertyName,
                   PropertyOwner iOwner,
                   TJBox_ValueType iValueType,
                   std::shared_ptr<JboxValue> iInitialValue,
                   TJBox_Tag iPropertyTag,
                   int iStepCount,
                   lua::EPersistence iPersistence = lua::EPersistence::kNone,
                   value_validator_t iValueValidator = {});

  void addProperty(const std::string &iPropertyName,
                   PropertyOwner iOwner,
                   std::shared_ptr<JboxValue> iInitialValue,
                   TJBox_Tag iPropertyTag,
                   int iStepCount = 0,
                   lua::EPersistence iPersistence = lua::EPersistence::kNone,
                   value_validator_t iValueValidator = {});

  JboxProperty *getProperty(std::string const &iPropertyName) const;
  JboxProperty *getProperty(TJBox_Tag iPropertyTag) const;

  std::vector<JboxPropertyInfo> getPropertyInfos() const;

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
  struct Info
  {
    TJBox_SizeT fSize{};
    resource::LoadingContext fLoadingContext{};

    TJBox_SizeT getSize() const { return fLoadingContext.isLoadOk() ? fSize : 0; }
    TJBox_SizeT getResidentSize() const { return fLoadingContext.fResidentSize; }

    TJBox_BLOBInfo to_TJBox_BLOBInfo() const;
  };

  TJBox_SizeT getSize() const { return fLoadingContext.isLoadOk() ? fData.size() : 0; }
  TJBox_SizeT getResidentSize() const { return fLoadingContext.fResidentSize; }

  std::vector<char> fData{};
  resource::LoadingContext fLoadingContext{};
  std::string fBlobPath{};
};

struct Sample
{
  struct Metadata
  {
    TJBox_SampleMetaData fMain;
    resource::LoadingContext fLoadingContext{};
    std::string fSamplePath{};

    std::string getLoopModeAsString() const;
    std::string getSampleName() const;
  };

  TJBox_SampleInfo getSampleInfo() const;
  bool isUserSample() const { return fSampleItem > 0; };

  TJBox_AudioFramePos getFrameCount() const { return fLoadingContext.isLoadOk() ? fData.size() / fChannels : 0; }
  TJBox_AudioFramePos getResidentFrameCount() const { return fLoadingContext.fResidentSize; }

  TJBox_UInt32 fChannels{1};
  TJBox_UInt32 fSampleRate{1};
  std::vector<TJBox_AudioSample> fData{};
  resource::LoadingContext fLoadingContext{};
  TJBox_ObjectRef fSampleItem{};
  std::string fSamplePath{};
};

}
}

#endif //__Pongasoft_re_mock_motherboard_impl_h__
