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

namespace re::mock::lua {

struct jbox_object
{
  using map_t = std::map<std::string, jbox_object *>;

  enum class Type
  {
    UNKNOWN,
    IGNORED,
    NATIVE_OBJECT,
    PROPERTY_SET,
    BOOLEAN,
    AUDIO_INPUT,
    AUDIO_INPUTS,
    AUDIO_OUTPUT,
    AUDIO_OUTPUTS,
    CV_INPUT,
    CV_INPUTS,
    CV_OUTPUT,
    CV_OUTPUTS
  };

  virtual ~jbox_object() = default;
  virtual Type getType() = 0;
};

struct jbox_ignored : public jbox_object {
  Type getType() override { return Type::IGNORED; }
};

struct jbox_property : public jbox_object {
  int property_tag{};
};

struct jbox_native_object : public jbox_property {

  Type getType() override { return Type::NATIVE_OBJECT; }
  struct {
    std::string operation;
    std::vector<TJBox_Value> params;
  } default_value{};
};

struct jbox_boolean_property : public jbox_property {
  Type getType() override { return Type::BOOLEAN; }
  bool default_value{};
};

struct jbox_property_set : public jbox_object {
  Type getType() override { return Type::PROPERTY_SET; }

  std::map<std::string, jbox_object *> document_owner{};
  std::map<std::string, jbox_object *> rtc_owner{};
  std::map<std::string, jbox_object *> rt_owner{};
};

struct jbox_socket : public jbox_object {
  Type getType() override { return type; }
  Type type{Type::UNKNOWN};
};

struct jbox_sockets : public jbox_object {
  Type getType() override { return type; }
  Type type{Type::UNKNOWN};
  std::vector<std::string> names{};
};

struct JBoxObjectUD
{
  jbox_object::Type fType;
  int fId;

  static JBoxObjectUD *New(lua_State *L)
  {
    return reinterpret_cast<JBoxObjectUD *>(lua_newuserdata(L, sizeof(JBoxObjectUD)));
  }
};

class MotherboardDef : public MockJBox
{
public:

  int luaIgnored();

  int luaNativeObject();

  int luaBoolean();

  int luaSocket(jbox_object::Type iSocketType);

  int luaPropertySet();

  template<typename T>
  T *getGlobal(std::string const &iName)
  {
    lua_getglobal(L, iName.c_str());
    return dynamic_cast<T *>(getObjectOnTopOfStack());
  }

  jbox_property_set *getCustomProperties()
  {
    return getGlobal<jbox_property_set>("custom_properties");
  }

  std::unique_ptr<jbox_sockets> getAudioInputs()
  {
    return getSockets("audio_inputs", jbox_object::Type::AUDIO_INPUTS, jbox_object::Type::AUDIO_INPUT);
  }

  std::unique_ptr<jbox_sockets> getAudioOutputs()
  {
    return getSockets("audio_outputs", jbox_object::Type::AUDIO_OUTPUTS, jbox_object::Type::AUDIO_OUTPUT);
  }

  std::unique_ptr<jbox_sockets> getCVInputs()
  {
    return getSockets("cv_inputs", jbox_object::Type::CV_INPUTS, jbox_object::Type::CV_INPUT);
  }

  std::unique_ptr<jbox_sockets> getCVOutputs()
  {
    return getSockets("cv_outputs", jbox_object::Type::CV_OUTPUTS, jbox_object::Type::CV_OUTPUT);
  }

  static MotherboardDef *loadFromRegistry(lua_State *L);

  static std::unique_ptr<MotherboardDef> fromFile(std::string const &iLuaFilename);

  static std::unique_ptr<MotherboardDef> fromString(std::string const &iLuaCode);

protected:
  MotherboardDef();

  int addObject(std::unique_ptr<jbox_object> iObject);

  jbox_object *getObjectOnTopOfStack();

  void luaPropertySet(char const *iKey, jbox_object::map_t &oMap);

  /**
   * Assumes lua table is at the top of the stack. Removes the map from the stack! */
  void populateMapFromLuaTable(jbox_object::map_t &oMap);

  std::unique_ptr<jbox_sockets> getSockets(char const *iSocketName,
                                           jbox_object::Type iSocketsType,
                                           jbox_object::Type iSocketType);

  void populatePropertyTag(jbox_property *iProperty);

private:
  ObjectManager<std::unique_ptr<jbox_object>> fObjects{};
};

}

#endif //__Pongasoft_re_mock_lua_motherboard_def_h__