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
#include <jbox.h>
#include <stdexcept>

namespace re::mock {

enum class PropertyOwner {
  kRTOwner,
  kRTCOwner,
  kDocOwner,
  kGUIOwner
};

class Motherboard;

namespace impl {

struct JboxProperty
{
  JboxProperty(jbox::PropertyPath const &iPropertyPath, PropertyOwner iOwner, TJBox_ValueType iValueType);
  JboxProperty(jbox::PropertyPath const &iPropertyPath, PropertyOwner iOwner, TJBox_Value const &iInitialValue);

  inline TJBox_Value loadValue() const { return fValue; };
  void storeValue(TJBox_Value const &iValue);

  const jbox::PropertyPath fPropertyPath;
  const PropertyOwner fOwner;
  const TJBox_ValueType fValueType;

protected:
  TJBox_Value fValue{JBox_MakeNil()};
};

struct JboxObject
{
  explicit JboxObject(jbox::ObjectPath const &iObjectPath);

  ~JboxObject() = default;

  TJBox_Value loadValue(jbox::PropertyName const &iPropertyName) const;
  void storeValue(jbox::PropertyName const &iPropertyName, TJBox_Value const &iValue);

  const jbox::ObjectPath fObjectPath;
  const TJBox_ObjectRef fObjectRef;

  friend class re::mock::Motherboard;

protected:
  void addProperty(jbox::PropertyName iPropertyName, PropertyOwner iOwner, TJBox_ValueType iValueType);
  void addProperty(jbox::PropertyName iPropertyName, PropertyOwner iOwner, TJBox_Value const &iInitialValue);

protected:
  std::map<jbox::PropertyName, std::unique_ptr<JboxProperty>> fProperties{};
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

class Motherboard
{
public:
  struct Config
  {
    friend class Motherboard;

    std::map<std::string, std::string> audio_inputs{};
    std::map<std::string, std::unique_ptr<jbox_property>> document_owner_properties{};

  private:
    Config(Motherboard &iMom) : fMom{iMom} {}
    Motherboard &fMom;
  };

public: // used by regular code
  static std::unique_ptr<Motherboard> init(std::function<void (Config &)> iConfigFunction);

  ~Motherboard();

  void connectSocket(jbox::ObjectPath const &iSocketPath);

public: // used by Jukebox.cpp (need to be public)
  static Motherboard &instance();
  TJBox_ObjectRef getObjectRef(jbox::ObjectPath const &iObjectPath) const;
  TJBox_Value loadProperty(TJBox_PropertyRef iProperty);
  void storeProperty(TJBox_PropertyRef iProperty, TJBox_Value const &iValue);

  Motherboard(Motherboard const &iOther) = delete;
  Motherboard &operator=(Motherboard const &iOther) = delete;

protected:
  Motherboard();

  impl::JboxObject *addObject(jbox::ObjectPath const &iObjectPath);
  impl::JboxObject *getObject(jbox::ObjectPath const &iObjectPath) const;

  void addAudioInput(std::string const &iSocketName, std::string iSocketDesc);
  void addProperty(std::string const &iPropertyPath, PropertyOwner iOwner, jbox_property const &iProperty);

protected:
  std::map<TJBox_ObjectRef, std::unique_ptr<impl::JboxObject>> fJboxObjects{};
  std::map<jbox::ObjectPath, TJBox_ObjectRef> fJboxObjectRefs{};
};

// Error handling
struct Error : public std::logic_error {
  Error(char const *s) : std::logic_error(s) {}
};

}

#endif //__PongasoftCommon_re_mock_motherboard_h__