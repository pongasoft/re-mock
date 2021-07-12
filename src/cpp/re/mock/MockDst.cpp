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

#include "MockDst.h"

namespace re::mock {

//------------------------------------------------------------------------
// MockSource::Config
//------------------------------------------------------------------------
const Rack::Extension::Configuration MockDst::Config = [](auto &def, auto &rtc, auto &rt) {
  // It is a source so it only produces audio
  def.audio_inputs[MockDst::LEFT_SOCKET] = jbox.audio_input();
  def.audio_inputs[MockDst::RIGHT_SOCKET] = jbox.audio_input();

  // use default bindings
  rtc = RealtimeController::byDefault();

  // rt
  rt = Realtime::byDefault<MockDst>();
};

//------------------------------------------------------------------------
// MockSource::MockSource
//------------------------------------------------------------------------
MockDst::MockDst(int) :
  fLeftSocketRef{JBox_GetMotherboardObjectRef(fmt::printf("/audio_inputs/%s", LEFT_SOCKET).c_str())},
  fRightSocketRef{JBox_GetMotherboardObjectRef(fmt::printf("/audio_inputs/%s", RIGHT_SOCKET).c_str())}
{
}

//------------------------------------------------------------------------
// MockSource::renderBatch
//------------------------------------------------------------------------
void MockDst::renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32)
{
  copyBuffer(fLeftSocketRef, fLeftBuffer);
  copyBuffer(fRightSocketRef, fRightBuffer);
}

//------------------------------------------------------------------------
// MockSource::copyBuffer
//------------------------------------------------------------------------
void MockDst::copyBuffer(TJBox_ObjectRef const &iFromSocket, buffer_type &iToBuffer)
{
  if(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(iFromSocket, "connected"))))
  {
    auto dspBuffer = JBox_LoadMOMProperty(JBox_MakePropertyRef(iFromSocket, "buffer"));
    JBox_GetDSPBufferData(dspBuffer, 0, iToBuffer.size(), iToBuffer.data());
  }
}

}