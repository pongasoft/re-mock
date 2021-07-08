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
#ifndef __PongasoftCommon_re_mock_motherboard_h__
#define __PongasoftCommon_re_mock_motherboard_h__

#include <JukeboxTypes.h>
#include <Jukebox.h>
#include <memory>
#include <functional>
#include <map>
#include <string>
#include <stdexcept>

namespace re::mock {

namespace fmt {

/*
 * Copied from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf */
template<typename ... Args>
std::string printf(const std::string& format, Args ... args )
{
  int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
  if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
  auto size = static_cast<size_t>( size_s );
  auto buf = std::make_unique<char[]>( size );
  std::snprintf( buf.get(), size, format.c_str(), args ... );
  return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

}

enum class PropertyOwner {
  kHostOwner,
  kRTOwner,
  kRTCOwner,
  kDocOwner,
  kGUIOwner
};

class Motherboard;

namespace impl {

struct JboxProperty
{
  JboxProperty(std::string const &iPropertyPath, PropertyOwner iOwner, TJBox_Value const &iInitialValue, TJBox_Tag iTag);

  inline TJBox_Value loadValue() const { return fValue; };
  void storeValue(TJBox_Value const &iValue);

  const std::string fPropertyPath;
  const PropertyOwner fOwner;
  const TJBox_Tag fTag;

protected:
  TJBox_Value fValue;
};

//struct JboxDspBufferProperty : public JboxProperty
//{
//
//};

struct JboxObject
{
  explicit JboxObject(std::string const &iObjectPath);

  ~JboxObject() = default;

  TJBox_Value loadValue(std::string const &iPropertyName) const;
  void storeValue(std::string const &iPropertyName, TJBox_Value const &iValue);

  const std::string fObjectPath;
  const TJBox_ObjectRef fObjectRef;

  friend class re::mock::Motherboard;

protected:
  void addProperty(std::string iPropertyName, PropertyOwner iOwner, TJBox_Value const &iInitialValue, TJBox_Tag iPropertyTag);

protected:
  std::map<std::string, std::unique_ptr<JboxProperty>> fProperties{};
};

}

struct jbox_property {
  int property_tag{};
  TJBox_Value default_value{JBox_MakeNil()};
  TJBox_Value getDefaultValue() const { return default_value; }
};

template<typename T = TJBox_Float64>
struct jbox_num_property {
  int property_tag{};
  T default_value{};
  TJBox_Value getDefaultValue() const { return JBox_MakeNumber(static_cast<TJBox_Float64>(default_value)); }
};

struct jbox_bool_property {
  int property_tag{};
  bool default_value{};
  TJBox_Value getDefaultValue() const { return JBox_MakeBoolean(default_value); }
};

struct jbox_audio_input{};
struct jbox_audio_output{};
struct jbox_cv_input{};
struct jbox_cv_output{};

struct {
  template<typename T = TJBox_Float64>
  inline std::unique_ptr<jbox_property> number(jbox_num_property<T> iProperty = jbox_num_property<T>{.default_value = 0}) {
    return createProperty(iProperty);
  }

  template<typename T = TJBox_Float64>
  inline std::unique_ptr<jbox_property> number(T iDefaultValue) {
    return createProperty(jbox_num_property<T>{.default_value = iDefaultValue});
  }

  inline std::unique_ptr<jbox_property> boolean(bool iDefaultValue) {
    return createProperty(jbox_bool_property{.default_value = iDefaultValue});
  }

  inline std::unique_ptr<jbox_property> boolean(jbox_bool_property iProperty = {.default_value = false}) {
    return createProperty(iProperty);
  }

  inline std::unique_ptr<jbox_property> property(TJBox_Value iDefaultValue) {
    return createProperty(jbox_property{.default_value = iDefaultValue});
  }

  inline std::unique_ptr<jbox_property> property(jbox_property iProperty = {.default_value = JBox_MakeNumber(0)}) {
    return createProperty(iProperty);
  }

  inline std::unique_ptr<jbox_audio_input> audio_input() { return std::make_unique<jbox_audio_input>(); }
  inline std::unique_ptr<jbox_audio_output> audio_output() { return std::make_unique<jbox_audio_output>(); }
  inline std::unique_ptr<jbox_cv_input> cv_input() { return std::make_unique<jbox_cv_input>(); }
  inline std::unique_ptr<jbox_cv_output> cv_output() { return std::make_unique<jbox_cv_output>(); }

private:
  template<typename P>
  std::unique_ptr<jbox_property> createProperty(P const &iProperty)
  {
    auto p = std::make_unique<jbox_property>();
    p->property_tag = iProperty.property_tag;
    p->default_value = iProperty.getDefaultValue();
    return p;
  }
} jbox;

struct MotherboardDef
{
  std::map<std::string, std::unique_ptr<jbox_audio_input>> audio_inputs{};
  std::map<std::string, std::unique_ptr<jbox_audio_output>> audio_outputs{};
  std::map<std::string, std::unique_ptr<jbox_cv_input>> cv_inputs{};
  std::map<std::string, std::unique_ptr<jbox_cv_output>> cv_outputs{};

  std::map<std::string, std::unique_ptr<jbox_property>> document_owner_properties{};
  std::map<std::string, std::unique_ptr<jbox_property>> rt_owner_properties{};
};

class Motherboard
{
public:

public: // used by regular code
  static std::unique_ptr<Motherboard> init(std::function<void (MotherboardDef &)> iConfigFunction);

  ~Motherboard();

  void connectSocket(std::string const &iSocketPath);

public: // used by Jukebox.cpp (need to be public)
  static Motherboard &instance();
  TJBox_ObjectRef getObjectRef(std::string const &iObjectPath) const;
  TJBox_Value loadProperty(TJBox_PropertyRef iProperty);
  void storeProperty(TJBox_PropertyRef iProperty, TJBox_Value const &iValue);

  Motherboard(Motherboard const &iOther) = delete;
  Motherboard &operator=(Motherboard const &iOther) = delete;

protected:
  Motherboard();

  impl::JboxObject *addObject(std::string const &iObjectPath);
  impl::JboxObject *getObject(std::string const &iObjectPath) const;

  void addAudioInput(std::string const &iSocketName);
  void addAudioOutput(std::string const &iSocketName);
  void addCVInput(std::string const &iSocketName);
  void addCVOutput(std::string const &iSocketName);
  void addProperty(TJBox_ObjectRef iParentObject, std::string const &iPropertyName, PropertyOwner iOwner, jbox_property const &iProperty);

  TJBox_PropertyRef getPropertyRef(std::string const &iPropertyPath) const;

protected:
  std::map<TJBox_ObjectRef, std::unique_ptr<impl::JboxObject>> fJboxObjects{};
  std::map<std::string, TJBox_ObjectRef> fJboxObjectRefs{};
  TJBox_ObjectRef fCustomPropertiesRef{};
};

// Error handling
struct Error : public std::logic_error {
  Error(char const *s) : std::logic_error(s) {}
};

}

#endif //__PongasoftCommon_re_mock_motherboard_h__