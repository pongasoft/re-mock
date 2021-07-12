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

#include "MockSrc.h"

namespace re::mock {

//------------------------------------------------------------------------
// MockSource::Config
//------------------------------------------------------------------------
const Rack::Extension::Configuration MockSrc::Config = [](auto &def, auto &rtc, auto &rt) {
  // It is a source so it only produces audio
  def.audio_outputs[MockSrc::LEFT_SOCKET] = jbox.audio_output();
  def.audio_outputs[MockSrc::RIGHT_SOCKET] = jbox.audio_output();

  // use default bindings
  rtc = RealtimeController::byDefault();

  // rt
  rt = Realtime::byDefault<MockSrc>();
};

//------------------------------------------------------------------------
// MockSource::MockSource
//------------------------------------------------------------------------
MockSrc::MockSrc(int) :
  fLeftSocketRef{JBox_GetMotherboardObjectRef(fmt::printf("/audio_outputs/%s", LEFT_SOCKET).c_str())},
  fRightSocketRef{JBox_GetMotherboardObjectRef(fmt::printf("/audio_outputs/%s", RIGHT_SOCKET).c_str())}
{
}

//------------------------------------------------------------------------
// MockSource::renderBatch
//------------------------------------------------------------------------
void MockSrc::renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32)
{
  copyBuffer(fLeftBuffer, fLeftSocketRef);
  copyBuffer(fRightBuffer, fRightSocketRef);
}

//------------------------------------------------------------------------
// MockSource::copyBuffer
//------------------------------------------------------------------------
void MockSrc::copyBuffer(buffer_type const &iFromBuffer, TJBox_ObjectRef &iToSocket)
{
  if(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(iToSocket, "connected"))))
  {
    auto dspBuffer = JBox_LoadMOMProperty(JBox_MakePropertyRef(iToSocket, "buffer"));
    JBox_SetDSPBufferData(dspBuffer, 0, iFromBuffer.size(), iFromBuffer.data());
  }
}

}