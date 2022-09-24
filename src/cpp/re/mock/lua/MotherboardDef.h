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
#include <set>
#include <vector>
#include <variant>
#include <optional>

namespace re::mock {
class Motherboard;
}

namespace re::mock::lua {

enum class EPersistence
{
  kPatch, kSong, kNone
};

struct jbox_native_object {
  using param_t = std::variant<TJBox_Float64, bool, std::string>;

  int fPropertyTag{};

  TJBox_ValueType value_type() const { return kJBox_NativeObject; }

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

  std::optional<EPersistence> fPersistence{};
};

struct jbox_boolean_property {
  int fPropertyTag{};
  bool fDefaultValue{};
  std::optional<EPersistence> fPersistence{};

  TJBox_ValueType value_type() const { return kJBox_Boolean; }

  jbox_boolean_property &property_tag(int iTag) { fPropertyTag = iTag; return *this; }
  jbox_boolean_property &default_value(bool iValue) { fDefaultValue = iValue; return *this;}
  jbox_boolean_property &persistence(EPersistence iPersistence) { fPersistence = iPersistence; return *this;}
};

struct jbox_number_property {
  int fPropertyTag{};
  TJBox_Float64 fDefaultValue{};
  std::optional<int> fSteps{};
  std::optional<EPersistence> fPersistence{};

  TJBox_ValueType value_type() const { return kJBox_Number; }

  jbox_number_property &property_tag(int iTag) { fPropertyTag = iTag; return *this; }
  jbox_number_property &default_value(TJBox_Float64 iValue) { fDefaultValue = iValue; return *this;}
  jbox_number_property &steps(int iSteps) { fSteps = iSteps; return *this;}
  jbox_number_property &persistence(EPersistence iPersistence) { fPersistence = iPersistence; return *this;}
};

struct jbox_string_property {
  int fPropertyTag{};
  std::string fDefaultValue{};
  int fMaxSize{};
  std::optional<EPersistence> fPersistence{};

  TJBox_ValueType value_type() const { return kJBox_String; }

  jbox_string_property &property_tag(int iTag) { fPropertyTag = iTag; return *this; }
  jbox_string_property &default_value(std::string iValue) { fDefaultValue = std::move(iValue); return *this; }
  jbox_string_property &max_size(int iMaxSize) { fMaxSize = iMaxSize; return *this; }
  jbox_string_property &persistence(EPersistence iPersistence) { fPersistence = iPersistence; return *this;}
};

struct jbox_blob_property {
  int fPropertyTag{};
  std::optional<std::string> fDefaultValue{};
  std::optional<EPersistence> fPersistence{};

  TJBox_ValueType value_type() const { return kJBox_BLOB; }

  jbox_blob_property &property_tag(int iTag) { fPropertyTag = iTag; return *this; }
  jbox_blob_property &default_value(std::string iValue) { fDefaultValue = std::move(iValue); return *this; }
};

struct jbox_sample_property {
  int fPropertyTag{};
  std::optional<std::string> fDefaultValue{};
  std::optional<EPersistence> fPersistence{};

  TJBox_ValueType value_type() const { return kJBox_Sample; }

  jbox_sample_property &property_tag(int iTag) { fPropertyTag = iTag; return *this; }
  jbox_sample_property &default_value(std::string iValue) { fDefaultValue = std::move(iValue); return *this; }
};

struct jbox_user_sample_property {
  std::optional<std::string> fName{};
  std::set<std::string> fSampleParameters{};
  std::optional<EPersistence> fPersistence{};

  jbox_user_sample_property &persistence(EPersistence iPersistence) { fPersistence = iPersistence; return *this;}
  jbox_user_sample_property &name(std::string s) { fName = std::move(s); return *this;}
  jbox_user_sample_property &sample_parameter(std::string s) { fSampleParameters.emplace(s); return *this; }
  jbox_user_sample_property &all_sample_parameters() {
    sample_parameter("root_key");
    sample_parameter("tune_cents");
    sample_parameter("play_range_start");
    sample_parameter("play_range_end");
    sample_parameter("loop_range_start");
    sample_parameter("loop_range_end");
    sample_parameter("loop_mode");
    sample_parameter("preview_volume_level");
    return *this;
  }
};

struct jbox_performance_property {
  enum class Type { UNKNOWN, MOD_WHEEL, PITCH_BEND, SUSTAIN_PEDAL, EXPRESSION, BREATH_CONTROL, AFTERTOUCH };

  int fPropertyTag{};
  std::optional<EPersistence> fPersistence{}; // only for MOD_WHEEL and EXPRESSION
  Type fType{Type::UNKNOWN};

  TJBox_ValueType value_type() const { return kJBox_Number; }

  jbox_performance_property &property_tag(int iTag) { fPropertyTag = iTag; return *this; }
  jbox_performance_property &persistence(EPersistence iPersistence) { fPersistence = iPersistence; return *this;}
  jbox_performance_property &type(Type iType) { fType = iType; return *this;}
};

struct jbox_sockets {
  enum class Type { UNKNOWN, AUDIO_INPUT, AUDIO_OUTPUT, CV_INPUT, CV_OUTPUT };

  Type fType{Type::UNKNOWN};
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
  std::shared_ptr<jbox_blob_property>,
  std::shared_ptr<jbox_boolean_property>,
  std::shared_ptr<jbox_number_property>,
  std::shared_ptr<jbox_string_property>,
  std::shared_ptr<jbox_performance_property>,
  std::shared_ptr<jbox_sample_property>,
  std::shared_ptr<jbox_user_sample_property>,
  std::shared_ptr<impl::jbox_property_set>,
  std::shared_ptr<impl::jbox_socket>
  >;
}

using jbox_property = std::variant<
  std::shared_ptr<jbox_native_object>,
  std::shared_ptr<jbox_blob_property>,
  std::shared_ptr<jbox_boolean_property>,
  std::shared_ptr<jbox_number_property>,
  std::shared_ptr<jbox_string_property>,
  std::shared_ptr<jbox_performance_property>,
  std::shared_ptr<jbox_sample_property>
>;

using gui_jbox_property = std::variant<
  std::shared_ptr<jbox_boolean_property>,
  std::shared_ptr<jbox_number_property>,
  std::shared_ptr<jbox_string_property>
>;

using document_jbox_property = std::variant<
  std::shared_ptr<jbox_boolean_property>,
  std::shared_ptr<jbox_number_property>,
  std::shared_ptr<jbox_string_property>,
  std::shared_ptr<jbox_performance_property>
>;

using rtc_jbox_property = std::variant<
  std::shared_ptr<jbox_native_object>,
  std::shared_ptr<jbox_blob_property>,
  std::shared_ptr<jbox_sample_property>,
  std::shared_ptr<jbox_boolean_property>,
  std::shared_ptr<jbox_number_property>,
  std::shared_ptr<jbox_string_property>
>;

using rt_jbox_property = std::variant<
  std::shared_ptr<jbox_boolean_property>,
  std::shared_ptr<jbox_number_property>,
  std::shared_ptr<jbox_string_property>
>;

struct JboxPropertySet {
  std::map<std::string, gui_jbox_property> gui_owner{};
  std::map<std::string, document_jbox_property> document_owner{};
  std::map<std::string, rtc_jbox_property> rtc_owner{};
  std::map<std::string, rt_jbox_property> rt_owner{};
  std::vector<std::shared_ptr<jbox_user_sample_property>> user_samples{};
};

class MotherboardDef : public MockJBox
{
public:
  MotherboardDef();

  int luaIgnored();
  int luaNativeObject();
  int luaBlob();
  int luaBoolean();
  int luaNumber();
  int luaPerformance(jbox_performance_property::Type iType);
  int luaString();
  int luaSocket(jbox_sockets::Type iSocketType);
  int luaPropertySet();
  int luaSample();
  int luaUserSample();

  template<typename T>
  std::shared_ptr<T> getGlobal(std::string const &iName)
  {
    lua_getglobal(L, iName.c_str());
    return std::dynamic_pointer_cast<T>(getObjectOnTopOfStack());
  }

  std::shared_ptr<JboxPropertySet> getCustomProperties() const;

  std::unique_ptr<jbox_sockets> getAudioInputs() const
  {
    return getSockets("audio_inputs", jbox_sockets::Type::AUDIO_INPUT);
  }

  std::unique_ptr<jbox_sockets> getAudioOutputs() const
  {
    return getSockets("audio_outputs", jbox_sockets::Type::AUDIO_OUTPUT);
  }

  std::unique_ptr<jbox_sockets> getCVInputs() const
  {
    return getSockets("cv_inputs", jbox_sockets::Type::CV_INPUT);
  }

  std::unique_ptr<jbox_sockets> getCVOutputs() const
  {
    return getSockets("cv_outputs", jbox_sockets::Type::CV_OUTPUT);
  }

  int getNumPatterns();

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

  std::unique_ptr<jbox_sockets> getSockets(char const *iSocketName, jbox_sockets::Type iSocketType) const;
  std::unique_ptr<jbox_sockets> doGetSockets(char const *iSocketName, jbox_sockets::Type iSocketType);

  void populatePropertyTag(jbox_property iProperty);

  std::optional<EPersistence> getPersistence();

  template<typename jbox_property_type>
  void filter(jbox_object_map_t &iMap,
              std::map<std::string, jbox_property_type> &oMap,
              std::function<std::optional<jbox_property_type>(std::string, std::optional<impl::jbox_object>)> iFilter);

  jbox_native_object::param_t toParam(int idx = -1);

  std::shared_ptr<JboxPropertySet> doGetCustomProperties();

private:
  ObjectManager<impl::jbox_object> fObjects{};
  std::shared_ptr<JboxPropertySet> fCustomProperties{};
};

}

#endif //__Pongasoft_re_mock_lua_motherboard_def_h__