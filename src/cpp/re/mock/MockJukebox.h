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

#ifndef RE_MOCK_MOCKJUKEBOX_H
#define RE_MOCK_MOCKJUKEBOX_H

#include "JukeboxTypes.h"
#include <string>

/**
 * This file contains calls that can be made from within test code for various convenient APIS not provided by
 * `Jukebox.h` */
namespace re::mock {

/**
 * Returns `true` if the 2 `TJBox_Value` provided represent the same underlying values. */
bool JBox_IsSameValue(TJBox_Value const &lhs, TJBox_Value const &rhs);

/**
 * Returns a string representation of the value. It can be formatted by providing an optional format.
 * For example, if `iValue` is a number, `JBox_toString(v, "%.2f");` would return the number representation with
 * 2 digits (`0.78`). */
std::string JBox_toString(TJBox_Value const &iValue, char const *iFormat = nullptr);

/**
 * Returns the full path of the property ref (ex: `/custom_properties/my_prop`) */
std::string JBox_toString(TJBox_PropertyRef const &iPropertyRef);

}

#endif //RE_MOCK_MOCKJUKEBOX_H
