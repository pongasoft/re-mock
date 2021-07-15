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

#include "MockDevices.h"
#include "stl.h"

namespace re::mock {

//------------------------------------------------------------------------
// MockAudioDevice::MockAudioDevice
//------------------------------------------------------------------------
MockAudioDevice::MockAudioDevice(int iSampleRate) : fSampleRate{iSampleRate} {}

//------------------------------------------------------------------------
// MockAudioDevice::copyBuffer
//------------------------------------------------------------------------
void MockAudioDevice::copyBuffer(MockAudioDevice::StereoSocket const &iFromSocket,
                                 MockAudioDevice::StereoBuffer &iToBuffer)
{
  copyBuffer(iFromSocket.fLeft, iToBuffer.fLeft);
  copyBuffer(iFromSocket.fRight, iToBuffer.fRight);
}

//------------------------------------------------------------------------
// MockAudioDevice::copyBuffer
//------------------------------------------------------------------------
void MockAudioDevice::copyBuffer(MockAudioDevice::StereoBuffer const &iFromBuffer,
                                 MockAudioDevice::StereoSocket const &iToSocket)
{
  copyBuffer(iFromBuffer.fLeft, iToSocket.fLeft);
  copyBuffer(iFromBuffer.fRight, iToSocket.fRight);

}

//------------------------------------------------------------------------
// MockAudioDevice::copyBuffer
//------------------------------------------------------------------------
void MockAudioDevice::copyBuffer(TJBox_ObjectRef const &iFromSocket, MockAudioDevice::buffer_type &iToBuffer)
{
  if(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(iFromSocket, "connected"))))
  {
    auto dspFromBuffer = JBox_LoadMOMProperty(JBox_MakePropertyRef(iFromSocket, "buffer"));
    JBox_GetDSPBufferData(dspFromBuffer, 0, iToBuffer.size(), iToBuffer.data());
  }
}

//------------------------------------------------------------------------
// MockAudioDevice::copyBuffer
//------------------------------------------------------------------------
void MockAudioDevice::copyBuffer(buffer_type const &iFromBuffer, TJBox_ObjectRef const &iToSocket)
{
  if(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(iToSocket, "connected"))))
  {
    auto dspToBuffer = JBox_LoadMOMProperty(JBox_MakePropertyRef(iToSocket, "buffer"));
    JBox_SetDSPBufferData(dspToBuffer, 0, iFromBuffer.size(), iFromBuffer.data());
  }
}

//------------------------------------------------------------------------
// MockAudioDevice::wire
//------------------------------------------------------------------------
void MockAudioDevice::wire(Rack &iRack, std::shared_ptr<Rack::Extension> iFromExtension, std::shared_ptr<Rack::Extension> iToExtension)
{
  iRack.wire(iFromExtension->getAudioOutSocket(LEFT_SOCKET), iToExtension->getAudioInSocket(LEFT_SOCKET));
  iRack.wire(iFromExtension->getAudioOutSocket(RIGHT_SOCKET), iToExtension->getAudioInSocket(RIGHT_SOCKET));
}

//------------------------------------------------------------------------
// MockAudioDevice::StereoBuffer::fill
//------------------------------------------------------------------------
void MockAudioDevice::StereoBuffer::fill(TJBox_AudioSample iLeftSample, TJBox_AudioSample iRightSample)
{
  fLeft.fill(iLeftSample);
  fRight.fill(iRightSample);
}

//------------------------------------------------------------------------
// MockAudioDevice::StereoBuffer::check
//------------------------------------------------------------------------
bool MockAudioDevice::StereoBuffer::check(TJBox_AudioSample iLeftSample, TJBox_AudioSample iRightSample) const
{
  return stl::all_item(fLeft, iLeftSample) && stl::all_item(fRight, iRightSample);
}

//------------------------------------------------------------------------
// MockAudioDevice::StereoSocket::input
//------------------------------------------------------------------------
MockAudioDevice::StereoSocket MockAudioDevice::StereoSocket::input()
{
  return {
    .fLeft = JBox_GetMotherboardObjectRef(fmt::printf("/audio_inputs/%s", LEFT_SOCKET).c_str()),
    .fRight = JBox_GetMotherboardObjectRef(fmt::printf("/audio_inputs/%s", RIGHT_SOCKET).c_str())
  };
}

//------------------------------------------------------------------------
// MockAudioDevice::StereoSocket::output
//------------------------------------------------------------------------
MockAudioDevice::StereoSocket MockAudioDevice::StereoSocket::output()
{
  return {
    .fLeft = JBox_GetMotherboardObjectRef(fmt::printf("/audio_outputs/%s", LEFT_SOCKET).c_str()),
    .fRight = JBox_GetMotherboardObjectRef(fmt::printf("/audio_outputs/%s", RIGHT_SOCKET).c_str())
  };
}

//------------------------------------------------------------------------
// MAUSrc::Config
//------------------------------------------------------------------------
const Rack::Extension::Configuration MAUSrc::Config = [](auto &def, auto &rtc, auto &rt) {
  // It is a source so it only produces audio
  def.audio_outputs[LEFT_SOCKET] = jbox.audio_output();
  def.audio_outputs[RIGHT_SOCKET] = jbox.audio_output();

  // use default bindings
  rtc = RealtimeController::byDefault();

  // rt
  rt = Realtime::byDefault<MAUSrc>();
};

//------------------------------------------------------------------------
// MAUSrc::MAUSrc
//------------------------------------------------------------------------
MAUSrc::MAUSrc(int iSampleRate) :
  MockAudioDevice(iSampleRate), fOutSocket{StereoSocket::output()}
{
}

//------------------------------------------------------------------------
// MAUSrc::renderBatch
//------------------------------------------------------------------------
void MAUSrc::renderBatch(TJBox_PropertyDiff const *, TJBox_UInt32)
{
  copyBuffer(fBuffer, fOutSocket);
}

//------------------------------------------------------------------------
// MAUDst::Config
//------------------------------------------------------------------------
const Rack::Extension::Configuration MAUDst::Config = [](auto &def, auto &rtc, auto &rt) {
  // It is a destination so it only receives audio
  def.audio_inputs[LEFT_SOCKET] = jbox.audio_input();
  def.audio_inputs[RIGHT_SOCKET] = jbox.audio_input();

  // use default bindings
  rtc = RealtimeController::byDefault();

  // rt
  rt = Realtime::byDefault<MAUDst>();
};

//------------------------------------------------------------------------
// MAUDst::MAUDst
//------------------------------------------------------------------------
MAUDst::MAUDst(int iSampleRate) :
  MockAudioDevice(iSampleRate), fInSocket{StereoSocket::input()}
{
}

//------------------------------------------------------------------------
// MAUDst::renderBatch
//------------------------------------------------------------------------
void MAUDst::renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32)
{
  copyBuffer(fInSocket, fBuffer);
}

//------------------------------------------------------------------------
// MAUPst::Config
//------------------------------------------------------------------------
const Rack::Extension::Configuration MAUPst::Config = [](auto &def, auto &rtc, auto &rt) {
  def.audio_outputs[LEFT_SOCKET] = jbox.audio_output();
  def.audio_outputs[RIGHT_SOCKET] = jbox.audio_output();
  def.audio_inputs[LEFT_SOCKET] = jbox.audio_input();
  def.audio_inputs[RIGHT_SOCKET] = jbox.audio_input();

  // use default bindings
  rtc = RealtimeController::byDefault();

  // rt
  rt = Realtime::byDefault<MAUPst>();
};

//------------------------------------------------------------------------
// MAUPst::MockAudioPassThrough
//------------------------------------------------------------------------
MAUPst::MAUPst(int iSampleRate) :
  MockAudioDevice(iSampleRate), fInSocket{StereoSocket::input()}, fOutSocket{StereoSocket::output()}
{

}

//------------------------------------------------------------------------
// MAUPst::renderBatch
//------------------------------------------------------------------------
void MAUPst::renderBatch(TJBox_PropertyDiff const *, TJBox_UInt32)
{
  copyBuffer(fInSocket, fBuffer);
  copyBuffer(fBuffer, fOutSocket);
}


//------------------------------------------------------------------------
// MockCVDevice::MockCVDevice
//------------------------------------------------------------------------
MockCVDevice::MockCVDevice(int iSampleRate) : fSampleRate{iSampleRate} {}

//------------------------------------------------------------------------
// MockCVDevice::loadValue
//------------------------------------------------------------------------
void MockCVDevice::loadValue(TJBox_ObjectRef const &iFromSocket)
{
  if(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(iFromSocket, "connected"))))
    fValue = JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(iFromSocket, "value")));
}

//------------------------------------------------------------------------
// MockCVDevice::storeValue
//------------------------------------------------------------------------
void MockCVDevice::storeValue(TJBox_ObjectRef const &iToSocket)
{
  if(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(iToSocket, "connected"))))
    JBox_StoreMOMProperty(JBox_MakePropertyRef(iToSocket, "value"), JBox_MakeNumber(fValue));
}

//------------------------------------------------------------------------
// MockCVDevice::wire
//------------------------------------------------------------------------
void MockCVDevice::wire(Rack &iRack,
                        std::shared_ptr<Rack::Extension> iFromExtension,
                        std::shared_ptr<Rack::Extension> iToExtension)
{
  iRack.wire(iFromExtension->getCVOutSocket(SOCKET), iToExtension->getCVInSocket(SOCKET));
}

//------------------------------------------------------------------------
// MCVSrc::Config
//------------------------------------------------------------------------
const Rack::Extension::Configuration MCVSrc::Config = [](auto &def, auto &rtc, auto &rt) {
  // It is a source so it only produces cv
  def.cv_outputs[SOCKET] = jbox.cv_output();

  // use default bindings
  rtc = RealtimeController::byDefault();

  // rt
  rt = Realtime::byDefault<MCVSrc>();
};

//------------------------------------------------------------------------
// MCVSrc::MCVSrc
//------------------------------------------------------------------------
MCVSrc::MCVSrc(int iSampleRate) :
  MockCVDevice(iSampleRate), fOutSocket{JBox_GetMotherboardObjectRef(fmt::printf("/cv_outputs/%s", SOCKET).c_str())}
{
}

//------------------------------------------------------------------------
// MCVSrc::renderBatch
//------------------------------------------------------------------------
void MCVSrc::renderBatch(TJBox_PropertyDiff const *, TJBox_UInt32)
{
  storeValue(fOutSocket);
}

//------------------------------------------------------------------------
// MCVDst::Config
//------------------------------------------------------------------------
const Rack::Extension::Configuration MCVDst::Config = [](auto &def, auto &rtc, auto &rt) {
  // It is a destination so it only reads cv
  def.cv_inputs[SOCKET] = jbox.cv_input();

  // use default bindings
  rtc = RealtimeController::byDefault();

  // rt
  rt = Realtime::byDefault<MCVDst>();
};

//------------------------------------------------------------------------
// MCVDst::MCVDst
//------------------------------------------------------------------------
MCVDst::MCVDst(int iSampleRate) :
  MockCVDevice(iSampleRate), fInSocket{JBox_GetMotherboardObjectRef(fmt::printf("/cv_inputs/%s", SOCKET).c_str())}
{
}

//------------------------------------------------------------------------
// MCVDst::renderBatch
//------------------------------------------------------------------------
void MCVDst::renderBatch(TJBox_PropertyDiff const *, TJBox_UInt32)
{
  loadValue(fInSocket);
}

//------------------------------------------------------------------------
// MCVPst::Config
//------------------------------------------------------------------------
const Rack::Extension::Configuration MCVPst::Config = [](auto &def, auto &rtc, auto &rt) {
  def.cv_inputs[SOCKET] = jbox.cv_input();
  def.cv_outputs[SOCKET] = jbox.cv_output();

  // use default bindings
  rtc = RealtimeController::byDefault();

  // rt
  rt = Realtime::byDefault<MCVPst>();
};

//------------------------------------------------------------------------
// MCVPst::MCVPst
//------------------------------------------------------------------------
MCVPst::MCVPst(int iSampleRate) :
  MockCVDevice(iSampleRate),
  fInSocket{JBox_GetMotherboardObjectRef(fmt::printf("/cv_inputs/%s", SOCKET).c_str())},
  fOutSocket{JBox_GetMotherboardObjectRef(fmt::printf("/cv_outputs/%s", SOCKET).c_str())}
{
}

//------------------------------------------------------------------------
// MCVDst::renderBatch
//------------------------------------------------------------------------
void MCVPst::renderBatch(TJBox_PropertyDiff const *, TJBox_UInt32)
{
  loadValue(fInSocket);
  storeValue(fOutSocket);
}

}