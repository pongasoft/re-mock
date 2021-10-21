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
#ifndef __Pongasoft_re_mock_lua_motherboard_def_h__
#define __Pongasoft_re_mock_lua_motherboard_def_h__

#include "MockJBox.h"
#include <re/mock/ObjectManager.hpp>
#include <JukeboxTypes.h>
#include <map>
#include <vector>
#include <variant>
#include <optional>

namespace re::mock {
class Motherboard;
}

namespace re::mock::lua {

struct jbox_native_object {
  using param_t = std::variant<TJBox_Float64, bool>;

  int fPropertyTag{};

  struct {
    std::string operation;
    std::vector<param_t> params;
  } fDefaultValue{};

  jbox_native_object &property_tag(int iTag) { fPropertyTag = iTag; return *this; }
  jbox_native_object &default_value(std::string iOperation, std::vector<param_t> const &iParams)
  {
    fDefaultValue.operation = iOperation;
    fDefaultValue.params = iParams;
    return *this;
  }

};

struct jbox_boolean_property {
  int fPropertyTag{};
  bool fDefaultValue{};

  jbox_boolean_property &property_tag(int iTag) { fPropertyTag = iTag; return *this; }
  jbox_boolean_property &default_value(bool iValue) { fDefaultValue = iValue; return *this;}
};

struct jbox_number_property {
  int fPropertyTag{};
  TJBox_Float64 fDefaultValue{};
  jbox_number_property &property_tag(int iTag) { fPropertyTag = iTag; return *this; }
  jbox_number_property &default_value(TJBox_Float64 iValue) { fDefaultValue = iValue; return *this;}
};

struct jbox_string_property {
  int fPropertyTag{};
  std::string fDefaultValue{};
  int fMaxSize{};
  jbox_string_property &property_tag(int iTag) { fPropertyTag = iTag; return *this; }
  jbox_string_property &default_value(std::string iValue) { fDefaultValue = std::move(iValue); return *this; }
  jbox_string_property &max_size(int iMaxSize) { fMaxSize = iMaxSize; return *this; }
};

struct jbox_sockets {
  enum Type { UNKNOWN, AUDIO_INPUT, AUDIO_OUTPUT, CV_INPUT, CV_OUTPUT };

  Type fType{UNKNOWN};
  std::vector<std::string> fNames{};
};

namespace impl {

struct jbox_ignored {};

struct jbox_property_set {
  int fPropertyTag{};
  int fCustomPropertiesRef{LUA_NOREF};

  jbox_property_set(lua_State *iLuaState);
  ~jbox_property_set();
private:
  lua_State *L;
};

struct jbox_socket {
  int fPropertyTag{};
  jbox_sockets::Type fType{jbox_sockets::Type::UNKNOWN};
};

using jbox_object = std::variant<
  std::shared_ptr<impl::jbox_ignored>,
  std::shared_ptr<jbox_native_object>,
  std::shared_ptr<jbox_boolean_property>,
  std::shared_ptr<jbox_number_property>,
  std::shared_ptr<jbox_string_property>,
  std::shared_ptr<impl::jbox_property_set>,
  std::shared_ptr<impl::jbox_socket>
  >;
}

using jbox_property = std::variant<
  std::shared_ptr<jbox_native_object>,
  std::shared_ptr<jbox_boolean_property>,
  std::shared_ptr<jbox_number_property>,
  std::shared_ptr<jbox_string_property>
  >;

struct JboxPropertySet {
  std::map<std::string, jbox_property> document_owner{};
  std::map<std::string, jbox_property> rtc_owner{};
  std::map<std::string, jbox_property> rt_owner{};
};

class MotherboardDef : public MockJBox
{
public:
  MotherboardDef();

  int luaIgnored();
  int luaNativeObject();
  int luaBoolean();
  int luaNumber();
  int luaNumberNoDefault();
  int luaString();
  int luaSocket(jbox_sockets::Type iSocketType);
  int luaPropertySet();

  template<typename T>
  std::shared_ptr<T> getGlobal(std::string const &iName)
  {
    lua_getglobal(L, iName.c_str());
    return std::dynamic_pointer_cast<T>(getObjectOnTopOfStack());
  }

  std::unique_ptr<JboxPropertySet> getCustomProperties();

  std::unique_ptr<jbox_sockets> getAudioInputs()
  {
    return getSockets("audio_inputs", jbox_sockets::Type::AUDIO_INPUT);
  }

  std::unique_ptr<jbox_sockets> getAudioOutputs()
  {
    return getSockets("audio_outputs", jbox_sockets::Type::AUDIO_OUTPUT);
  }

  std::unique_ptr<jbox_sockets> getCVInputs()
  {
    return getSockets("cv_inputs", jbox_sockets::Type::CV_INPUT);
  }

  std::unique_ptr<jbox_sockets> getCVOutputs()
  {
    return getSockets("cv_outputs", jbox_sockets::Type::CV_OUTPUT);
  }

  static MotherboardDef *loadFromRegistry(lua_State *L);

  static std::unique_ptr<MotherboardDef> fromFile(std::string const &iLuaFilename);

  static std::unique_ptr<MotherboardDef> fromString(std::string const &iLuaCode);


protected:
  using jbox_object_map_t = std::map<std::string, impl::jbox_object>;
  using jbox_property_map_t = std::map<std::string, jbox_property>;

  int addObjectOnTopOfStack(impl::jbox_object iObject);

  std::optional<impl::jbox_object> getObjectOnTopOfStack();

  void luaPropertySet(char const *iKey, jbox_object_map_t &oMap);

  /**
   * Assumes lua table is at the top of the stack. Removes the map from the stack! */
  void populateMapFromLuaTable(jbox_object_map_t &oMap);

  std::unique_ptr<jbox_sockets> getSockets(char const *iSocketName, jbox_sockets::Type iSocketType);

  void populatePropertyTag(jbox_property iProperty);

  void filter(jbox_object_map_t &iMap, jbox_property_map_t &oMap);

  jbox_native_object::param_t toParam(int idx = -1);

private:
  ObjectManager<impl::jbox_object> fObjects{};
};

}

#endif //__Pongasoft_re_mock_lua_motherboard_def_h__