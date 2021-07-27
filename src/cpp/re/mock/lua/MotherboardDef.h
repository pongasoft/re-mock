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

namespace re::mock {
class Motherboard;
}

namespace re::mock::lua {

struct jbox_object : public std::enable_shared_from_this<jbox_object>
{
  using map_t = std::map<std::string, std::shared_ptr<jbox_object>>;

  enum class Type
  {
    UNKNOWN,
    IGNORED,
    NATIVE_OBJECT,
    BOOLEAN,
    NUMBER,
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

  template<typename T>
  std::shared_ptr<T> withType()
  {
    auto ptr = std::dynamic_pointer_cast<T>(shared_from_this());
    RE_MOCK_ASSERT(ptr != nullptr, "can't convert to requested type");
    return ptr;
  }
};

struct jbox_ignored : public jbox_object {
  Type getType() override { return Type::IGNORED; }
};

struct jbox_property : public jbox_object {
  int property_tag{};
  virtual TJBox_Value getDefaultValue() const;
  virtual TJBox_Value computeDefaultValue(Motherboard *iMotherboard) const { return getDefaultValue(); }
};

struct jbox_native_object : public jbox_property {

  Type getType() override { return Type::NATIVE_OBJECT; }
  struct {
    std::string operation;
    std::vector<TJBox_Value> params;
  } default_value{};
  TJBox_Value computeDefaultValue(Motherboard *iMotherboard) const override;
};

struct jbox_boolean_property : public jbox_property {
  Type getType() override { return Type::BOOLEAN; }
  bool default_value{};
  TJBox_Value getDefaultValue() const override;
};

struct jbox_number_property : public jbox_property {
  Type getType() override { return Type::NUMBER; }
  TJBox_Float64 default_value{};
  TJBox_Value getDefaultValue() const override;
};

struct jbox_property_set {
  std::map<std::string, std::shared_ptr<jbox_object>> document_owner{};
  std::map<std::string, std::shared_ptr<jbox_object>> rtc_owner{};
  std::map<std::string, std::shared_ptr<jbox_object>> rt_owner{};
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

class MotherboardDef : public MockJBox
{
public:
  MotherboardDef();

  int luaIgnored();
  int luaNativeObject();
  int luaBoolean();
  int luaNumber();
  int luaSocket(jbox_object::Type iSocketType);
  int luaPropertySet();

  template<typename T>
  std::shared_ptr<T> getGlobal(std::string const &iName)
  {
    lua_getglobal(L, iName.c_str());
    return std::dynamic_pointer_cast<T>(getObjectOnTopOfStack());
  }

  std::unique_ptr<jbox_property_set> getCustomProperties();

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
  int addObjectOnTopOfStack(std::unique_ptr<jbox_object> iObject);

  std::shared_ptr<jbox_object> getObjectOnTopOfStack();

  void luaPropertySet(char const *iKey, jbox_object::map_t &oMap);

  /**
   * Assumes lua table is at the top of the stack. Removes the map from the stack! */
  void populateMapFromLuaTable(jbox_object::map_t &oMap);

  std::unique_ptr<jbox_sockets> getSockets(char const *iSocketName,
                                           jbox_object::Type iSocketsType,
                                           jbox_object::Type iSocketType);

  void populatePropertyTag(jbox_property *iProperty);

private:
  ObjectManager<std::shared_ptr<jbox_object>> fObjects{};
};

}

#endif //__Pongasoft_re_mock_lua_motherboard_def_h__