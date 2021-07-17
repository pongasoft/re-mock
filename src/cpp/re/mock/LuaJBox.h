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
#ifndef __PongasoftCommon_re_mock_lua_jbox_h__
#define __PongasoftCommon_re_mock_lua_jbox_h__

#include <JukeboxTypes.h>
#include <Jukebox.h>
#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace re::mock {

class Motherboard;

struct jbox_property {
  int property_tag{};
  TJBox_Value default_value{JBox_MakeNil()};
  TJBox_Value getDefaultValue() const { return default_value; }
  std::function<TJBox_Value (Motherboard *)> computeDefaultValue{[this] (Motherboard *) { return getDefaultValue(); }};
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

struct jbox_native_object {
  int property_tag{};
  struct {
    std::string operation;
    std::vector<TJBox_Value> params;
  } default_value{};
};

struct LuaJbox {
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

  std::unique_ptr<jbox_property> native_object(jbox_native_object iNativeObject = {});

  TJBox_Value load_property(std::string const &iPropertyPath);
  TJBox_Value make_native_object_rw(std::string const &iOperation, std::vector<TJBox_Value> const &iParams);
  TJBox_Value make_native_object_ro(std::string const &iOperation, std::vector<TJBox_Value> const &iParams);
  void store_property(std::string const &iPropertyPath, TJBox_Value const &iValue);

private:
  template<typename P>
  std::unique_ptr<jbox_property> createProperty(P const &iProperty)
  {
    auto p = std::make_unique<jbox_property>();
    p->property_tag = iProperty.property_tag;
    p->default_value = iProperty.getDefaultValue();
    return p;
  }
};

}

#endif //__PongasoftCommon_re_mock_lua_jbox_h__