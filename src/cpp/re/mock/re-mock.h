/*
 * Copyright (c) 2022 pongasoft
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

#ifndef RE_MOCK_RE_MOCK_H
#define RE_MOCK_RE_MOCK_H

// Contains the DeviceTester classes like InstrumentTester, StudioEffectTester
#include "DeviceTesters.h"

// Contains the Extension / ExtensionDevice (typed version) api
#include "Extension.h"

// Contains the Rack and definitions of the various Duration
#include "Rack.h"

// Contains the definition of the Exception thrown by the framework
#include "Errors.h"

// Contains the definition of the various kinds of resources (file, string, blob, etc...)
#include "Resources.h"

// Contains the mock devices used by testers
#include "MockDevices.h"

#endif //RE_MOCK_RE_MOCK_H
