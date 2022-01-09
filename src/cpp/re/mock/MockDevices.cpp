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
// MockDevice::MockDevice
//------------------------------------------------------------------------
MockDevice::MockDevice(int iSampleRate) : fSampleRate{iSampleRate} {}

//------------------------------------------------------------------------
// MockAudioDevice::MockAudioDevice
//------------------------------------------------------------------------
MockAudioDevice::MockAudioDevice(int iSampleRate) : MockDevice{iSampleRate} {}

//------------------------------------------------------------------------
// MockAudioDevice::copyBuffer
//------------------------------------------------------------------------
bool MockAudioDevice::copyBuffer(MockAudioDevice::StereoSocket const &iFromSocket,
                                 MockAudioDevice::StereoBuffer &iToBuffer)
{
  auto res = false;

  res |= copyBuffer(iFromSocket.fLeft, iToBuffer.fLeft);
  res |= copyBuffer(iFromSocket.fRight, iToBuffer.fRight);

  return res;
}

//------------------------------------------------------------------------
// MockAudioDevice::copyBuffer
//------------------------------------------------------------------------
bool MockAudioDevice::copyBuffer(MockAudioDevice::StereoBuffer const &iFromBuffer,
                                 MockAudioDevice::StereoSocket const &iToSocket)
{
  auto res = false;

  res |= copyBuffer(iFromBuffer.fLeft, iToSocket.fLeft);
  res |= copyBuffer(iFromBuffer.fRight, iToSocket.fRight);

  return res;
}

//------------------------------------------------------------------------
// MockAudioDevice::copyBuffer
//------------------------------------------------------------------------
bool MockAudioDevice::copyBuffer(TJBox_ObjectRef const &iFromSocket, MockAudioDevice::buffer_type &iToBuffer)
{
  if(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(iFromSocket, "connected"))))
  {
    auto dspFromBuffer = JBox_LoadMOMProperty(JBox_MakePropertyRef(iFromSocket, "buffer"));
    JBox_GetDSPBufferData(dspFromBuffer, 0, iToBuffer.size(), iToBuffer.data());
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// MockAudioDevice::copyBuffer
//------------------------------------------------------------------------
bool MockAudioDevice::copyBuffer(buffer_type const &iFromBuffer, TJBox_ObjectRef const &iToSocket)
{
  if(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(iToSocket, "connected"))))
  {
    auto dspToBuffer = JBox_LoadMOMProperty(JBox_MakePropertyRef(iToSocket, "buffer"));
    JBox_SetDSPBufferData(dspToBuffer, 0, iFromBuffer.size(), iFromBuffer.data());
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// MockAudioDevice::copyBuffer
//------------------------------------------------------------------------
void MockAudioDevice::copyBuffer(MockAudioDevice::buffer_type const &iFromBuffer,
                                 MockAudioDevice::buffer_type &iToBuffer)
{
  iToBuffer = iFromBuffer;
}

//------------------------------------------------------------------------
// MockAudioDevice::copyBuffer
//------------------------------------------------------------------------
void MockAudioDevice::copyBuffer(MockAudioDevice::StereoBuffer const &iFromBuffer,
                                 MockAudioDevice::StereoBuffer &iToBuffer)
{
  copyBuffer(iFromBuffer.fLeft, iToBuffer.fLeft);
  copyBuffer(iFromBuffer.fRight, iToBuffer.fRight);
}

//------------------------------------------------------------------------
// MockAudioDevice::StereoBuffer::fill
//------------------------------------------------------------------------
MockAudioDevice::StereoBuffer &MockAudioDevice::StereoBuffer::fill(TJBox_AudioSample iLeftSample, TJBox_AudioSample iRightSample)
{
  fLeft.fill(iLeftSample);
  fRight.fill(iRightSample);
  return *this;
}

//------------------------------------------------------------------------
// MockAudioDevice::StereoBuffer::operator<<
//------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, MockAudioDevice::StereoBuffer const &iBuffer)
{
  return os << "{.fLeft=[" << stl::Join(iBuffer.fLeft, ", ")
            << "],.fRight=[" << stl::Join(iBuffer.fRight, ", ") << "]}";
}

//------------------------------------------------------------------------
// operator==(MockAudioDevice::StereoBuffer)
//------------------------------------------------------------------------
bool operator==(MockAudioDevice::StereoBuffer const &lhs, MockAudioDevice::StereoBuffer const &rhs)
{
  return MockAudioDevice::eq(lhs, rhs);
}

//------------------------------------------------------------------------
// operator!=(MockAudioDevice::StereoBuffer)
//------------------------------------------------------------------------
bool operator!=(MockAudioDevice::StereoBuffer const &lhs, MockAudioDevice::StereoBuffer const &rhs)
{
  return !(rhs == lhs);
}

//------------------------------------------------------------------------
// MockAudioDevice::eq
//------------------------------------------------------------------------
bool MockAudioDevice::eq(TJBox_AudioSample iSample1, TJBox_AudioSample iSample2)
{
  return stl::almost_equal<TJBox_AudioSample>(iSample1, iSample2);
}

//------------------------------------------------------------------------
// MockAudioDevice::eq
//------------------------------------------------------------------------
bool MockAudioDevice::eq(buffer_type const &iBuffer1, buffer_type const &iBuffer2)
{
  for(int i = 0; i < constants::kBatchSize; i++)
  {
    if(!eq(iBuffer1[i], iBuffer2[i]))
      return false;
  }

  return true;
}

//------------------------------------------------------------------------
// MockAudioDevice::eq
//------------------------------------------------------------------------
bool MockAudioDevice::eq(MockAudioDevice::StereoBuffer const &iBuffer1,
                         MockAudioDevice::StereoBuffer const &iBuffer2)
{
  return eq(iBuffer1.fLeft, iBuffer2.fLeft) && eq(iBuffer1.fRight, iBuffer2.fRight);
}

//------------------------------------------------------------------------
// MockAudioDevice::eqWithPrecision
//------------------------------------------------------------------------
bool MockAudioDevice::eqWithPrecision(TJBox_AudioSample iPrecision,
                                      TJBox_AudioSample iSample1,
                                      TJBox_AudioSample iSample2)
{
  return std::fabs(iSample1 - iSample2) <= iPrecision;
}

//------------------------------------------------------------------------
// MockAudioDevice::eq
//------------------------------------------------------------------------
bool MockAudioDevice::eqWithPrecision(TJBox_AudioSample iPrecision,
                                      buffer_type const &iBuffer1,
                                      buffer_type const &iBuffer2)
{
  for(int i = 0; i < constants::kBatchSize; i++)
  {
    if(!eqWithPrecision(iPrecision, iBuffer1[i], iBuffer2[i]))
      return false;
  }

  return true;
}

//------------------------------------------------------------------------
// MockAudioDevice::eqWithPrecision
//------------------------------------------------------------------------
bool MockAudioDevice::eqWithPrecision(TJBox_AudioSample iPrecision,
                                      MockAudioDevice::StereoBuffer const &iBuffer1,
                                      MockAudioDevice::StereoBuffer const &iBuffer2)
{
  return eqWithPrecision(iPrecision, iBuffer1.fLeft, iBuffer2.fLeft) &&
         eqWithPrecision(iPrecision, iBuffer1.fRight, iBuffer2.fRight);
}


//------------------------------------------------------------------------
// MockAudioDevice::buffer
//------------------------------------------------------------------------
MockAudioDevice::StereoBuffer MockAudioDevice::buffer(TJBox_AudioSample iLeftSample, TJBox_AudioSample iRightSample)
{
  return MockAudioDevice::StereoBuffer().fill(iLeftSample, iRightSample);
}

//------------------------------------------------------------------------
// MockAudioDevice::buffer
//------------------------------------------------------------------------
MockAudioDevice::StereoBuffer MockAudioDevice::buffer(buffer_type const &iLeftBuffer, buffer_type const &iRightBuffer)
{
  return { /* .fLeft = */ iLeftBuffer, /* .fRight = */ iRightBuffer };
}

//------------------------------------------------------------------------
// MockAudioDevice::StereoSocket::input
//------------------------------------------------------------------------
MockAudioDevice::StereoSocket MockAudioDevice::StereoSocket::input()
{
  return {
    /* .fLeft = */  JBox_GetMotherboardObjectRef(fmt::printf("/audio_inputs/%s", LEFT_SOCKET).c_str()),
    /* .fRight = */ JBox_GetMotherboardObjectRef(fmt::printf("/audio_inputs/%s", RIGHT_SOCKET).c_str())
  };
}

//------------------------------------------------------------------------
// MockAudioDevice::StereoSocket::output
//------------------------------------------------------------------------
MockAudioDevice::StereoSocket MockAudioDevice::StereoSocket::output()
{
  return {
    /* .fLeft = */  JBox_GetMotherboardObjectRef(fmt::printf("/audio_outputs/%s", LEFT_SOCKET).c_str()),
    /* .fRight = */ JBox_GetMotherboardObjectRef(fmt::printf("/audio_outputs/%s", RIGHT_SOCKET).c_str())
  };
}

//------------------------------------------------------------------------
// MAUSrc::CONFIG
//------------------------------------------------------------------------
const DeviceConfig<MAUSrc> MAUSrc::CONFIG = 
  DeviceConfig<MAUSrc>::fromSkeleton(DeviceType::kHelper).mdef(Config::stereo_audio_out());

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
const DeviceConfig<MAUDst> MAUDst::CONFIG = 
  DeviceConfig<MAUDst>::fromSkeleton(DeviceType::kHelper).mdef(Config::stereo_audio_in());

//------------------------------------------------------------------------
// MAUDst::MAUDst
//------------------------------------------------------------------------
MAUDst::MAUDst(int iSampleRate) :
  MockAudioDevice(iSampleRate), fInSocket{StereoSocket::input()}
{
  fSample.stereo().sample_rate(fSampleRate);
}

//------------------------------------------------------------------------
// MAUDst::renderBatch
//------------------------------------------------------------------------
void MAUDst::renderBatch(const TJBox_PropertyDiff *, TJBox_UInt32)
{
  fBuffer.fill(0, 0);

  if(copyBuffer(fInSocket, fBuffer))
    fSample.append(fBuffer);
}

//------------------------------------------------------------------------
// MAUPst::Config
//------------------------------------------------------------------------
const DeviceConfig<MAUPst> MAUPst::CONFIG = DeviceConfig<MAUPst>::fromSkeleton(DeviceType::kStudioFX)
  .mdef(Config::stereo_audio_out())
  .mdef(Config::stereo_audio_in());

//------------------------------------------------------------------------
// MAUPst::MockAudioPassThrough
//------------------------------------------------------------------------
MAUPst::MAUPst(int iSampleRate) :
  MockAudioDevice(iSampleRate),
  fCustomPropertiesRef{JBox_GetMotherboardObjectRef("/custom_properties")},
  fInSocket{StereoSocket::input()}, fOutSocket{StereoSocket::output()}
{
}

//------------------------------------------------------------------------
// MAUPst::getBypassState
//------------------------------------------------------------------------
TJBox_OnOffBypassStates MAUPst::getBypassState() const
{
  return static_cast<TJBox_OnOffBypassStates>(JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(fCustomPropertiesRef,
                                                                                                       "builtin_onoffbypass"))));
}

//------------------------------------------------------------------------
// MAUPst::setBypassState
//------------------------------------------------------------------------
void MAUPst::setBypassState(TJBox_OnOffBypassStates iState)
{
  JBox_StoreMOMProperty(JBox_MakePropertyRef(fCustomPropertiesRef, "builtin_onoffbypass"),
                        JBox_MakeNumber(iState));
}

//------------------------------------------------------------------------
// MAUPst::renderBatch
//------------------------------------------------------------------------
void MAUPst::renderBatch(TJBox_PropertyDiff const *, TJBox_UInt32)
{
  copyBuffer(fInSocket, fBuffer);

  if(getBypassState() != kJBox_EnabledOff) // On and Bypass are doing the same since MAUPst does not modify the signal...
    copyBuffer(fBuffer, fOutSocket);
}

//------------------------------------------------------------------------
// MockCVDevice::MockCVDevice
//------------------------------------------------------------------------
MockCVDevice::MockCVDevice(int iSampleRate) : MockDevice{iSampleRate} {}

//------------------------------------------------------------------------
// MockCVDevice::loadValue
//------------------------------------------------------------------------
bool MockCVDevice::loadValue(TJBox_ObjectRef const &iFromSocket, TJBox_Float64 &oValue)
{
  if(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(iFromSocket, "connected"))))
  {
    oValue = JBox_GetNumber(JBox_LoadMOMProperty(JBox_MakePropertyRef(iFromSocket, "value")));
    return true;
  }

  return false;
}

//------------------------------------------------------------------------
// MockCVDevice::storeValue
//------------------------------------------------------------------------
bool MockCVDevice::storeValue(TJBox_Float64 iValue, TJBox_ObjectRef const &iToSocket)
{
  if(JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(iToSocket, "connected"))))
  {
    JBox_StoreMOMProperty(JBox_MakePropertyRef(iToSocket, "value"), JBox_MakeNumber(iValue));
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// MockCVDevice::loadValue
//------------------------------------------------------------------------
bool MockCVDevice::loadValue(TJBox_ObjectRef const &iFromSocket)
{
  return loadValue(iFromSocket, fValue);
}

//------------------------------------------------------------------------
// MockCVDevice::storeValue
//------------------------------------------------------------------------
bool MockCVDevice::storeValue(TJBox_ObjectRef const &iToSocket)
{
  return storeValue(fValue, iToSocket);
}

//------------------------------------------------------------------------
// MockCVDevice::eq
//------------------------------------------------------------------------
bool MockCVDevice::eq(TJBox_Float64 iCV1, TJBox_Float64 iCV2)
{
  return stl::almost_equal<TJBox_Float64>(iCV1, iCV2);
}

//------------------------------------------------------------------------
// MCVSrc::CONFIG
//------------------------------------------------------------------------
const DeviceConfig<MCVSrc> MCVSrc::CONFIG =
  DeviceConfig<MCVSrc>::fromSkeleton(DeviceType::kHelper).mdef(Config::cv_out());

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
// MCVDst::CONFIG
//------------------------------------------------------------------------
const DeviceConfig<MCVDst> MCVDst::CONFIG =
  DeviceConfig<MCVDst>::fromSkeleton(DeviceType::kHelper).mdef(Config::cv_in());

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
  if(loadValue(fInSocket))
    fValues.emplace_back(fValue);
}

//------------------------------------------------------------------------
// MCVPst::CONFIG
//------------------------------------------------------------------------
const DeviceConfig<MCVPst> MCVPst::CONFIG = 
  DeviceConfig<MCVPst>::fromSkeleton(DeviceType::kHelper)
  .mdef(Config::cv_out())
  .mdef(Config::cv_in());

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

//------------------------------------------------------------------------
// MockNotePlayer::MockNotePlayer
//------------------------------------------------------------------------
MockNotePlayer::MockNotePlayer(int iSampleRate) : MockDevice{iSampleRate},
                                                  fEnvironmentRef{JBox_GetMotherboardObjectRef("/environment")}
{}

//------------------------------------------------------------------------
// MockNotePlayer::isBypassed
//------------------------------------------------------------------------
bool MockNotePlayer::isBypassed() const
{
  return JBox_GetBoolean(JBox_LoadMOMProperty(JBox_MakePropertyRef(fEnvironmentRef, "player_bypassed")));
}

//------------------------------------------------------------------------
// MockNotePlayer::setBypassed
//------------------------------------------------------------------------
void MockNotePlayer::setBypassed(bool iBypassed)
{
  JBox_StoreMOMProperty(JBox_MakePropertyRef(fEnvironmentRef, "player_bypassed"), JBox_MakeBoolean(iBypassed));
}

//------------------------------------------------------------------------
// MockNotePlayer::wire
//------------------------------------------------------------------------
void MockNotePlayer::wire(Rack &iRack,
                          Rack::Extension const &iFromExtension,
                          Rack::Extension const &iToExtension)
{
  iRack.wire(iFromExtension.getNoteOutSocket(), iToExtension.getNoteInSocket());
}

//------------------------------------------------------------------------
// MCVPst::CONFIG
//------------------------------------------------------------------------
const DeviceConfig<MNPSrc> MNPSrc::CONFIG = DeviceConfig<MNPSrc>::fromSkeleton(DeviceType::kNotePlayer);

//------------------------------------------------------------------------
// MNPSrc::MNPSrc
//------------------------------------------------------------------------
MNPSrc::MNPSrc(int iSampleRate) : MockNotePlayer{iSampleRate}
{}

//------------------------------------------------------------------------
// MNPSrc::renderBatch
//------------------------------------------------------------------------
void MNPSrc::renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount)
{
  if(!isBypassed())
  {
    for(auto &event: fNoteEvents.events())
      JBox_OutputNoteEvent(event);
  }

  fNoteEvents.clear();
}

//------------------------------------------------------------------------
// MNPDst::CONFIG
//------------------------------------------------------------------------
const DeviceConfig<MNPDst> MNPDst::CONFIG = DeviceConfig<MNPDst>::fromSkeleton(DeviceType::kNotePlayer)
  .accept_notes(true)
  .rtc(Config::rt_input_setup_notify_all_notes());

//------------------------------------------------------------------------
// MNPDst::MNPDst
//------------------------------------------------------------------------
MNPDst::MNPDst(int iSampleRate) : MockNotePlayer{iSampleRate},
                                  fNoteStatesRef{JBox_GetMotherboardObjectRef("/note_states")}
{}

//------------------------------------------------------------------------
// MNPDst::renderBatch
//------------------------------------------------------------------------
void MNPDst::renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount)
{
  fNoteEvents.clear();

  for(int i = 0; i < iDiffCount; i++)
  {
    auto &diff = iPropertyDiffs[i];
    if(diff.fPropertyRef.fObject == fNoteStatesRef)
      fNoteEvents.event(JBox_AsNoteEvent(diff));
  }
}

//------------------------------------------------------------------------
// MNPPst::CONFIG
//------------------------------------------------------------------------
const DeviceConfig<MNPPst> MNPPst::CONFIG = DeviceConfig<MNPPst>::fromSkeleton(DeviceType::kNotePlayer)
  .accept_notes(true)
  .rtc(Config::rt_input_setup_notify_all_notes());

//------------------------------------------------------------------------
// MNPPst::MNPPst
//------------------------------------------------------------------------
MNPPst::MNPPst(int iSampleRate) : MockNotePlayer{iSampleRate},
                                  fNoteStatesRef{JBox_GetMotherboardObjectRef("/note_states")}
{}

//------------------------------------------------------------------------
// MNPPst::renderBatch
//------------------------------------------------------------------------
void MNPPst::renderBatch(TJBox_PropertyDiff const *iPropertyDiffs, TJBox_UInt32 iDiffCount)
{
  fNoteEvents.clear();

  for(int i = 0; i < iDiffCount; i++)
  {
    auto &diff = iPropertyDiffs[i];
    if(diff.fPropertyRef.fObject == fNoteStatesRef)
    {
      auto noteEvent = JBox_AsNoteEvent(diff);
      fNoteEvents.event(noteEvent);
      if(!isBypassed())
        JBox_OutputNoteEvent(noteEvent);
    }
  }
}

//------------------------------------------------------------------------
// MockDevice::NoteEvents::events
//------------------------------------------------------------------------
MockDevice::NoteEvents &MockDevice::NoteEvents::events(Motherboard::NoteEvents const &iNoteEvents)
{
  for(auto &e: iNoteEvents)
    event(e);
  return *this;
}

//------------------------------------------------------------------------
// MockDevice::NoteEvents::event
//------------------------------------------------------------------------
MockDevice::NoteEvents &MockDevice::NoteEvents::event(TJBox_NoteEvent const &iNoteEvent)
{
  RE_MOCK_ASSERT(iNoteEvent.fNoteNumber >= Motherboard::FIRST_MIDI_NOTE && iNoteEvent.fNoteNumber <= Motherboard::LAST_MIDI_NOTE);
  RE_MOCK_ASSERT(iNoteEvent.fVelocity >= 0 && iNoteEvent.fVelocity <= 127);
  RE_MOCK_ASSERT(iNoteEvent.fAtFrameIndex >= 0 && iNoteEvent.fAtFrameIndex <= 63);
  fNoteEvents.emplace_back(iNoteEvent);
  return *this;
}

//------------------------------------------------------------------------
// MockDevice::NoteEvents::note
//------------------------------------------------------------------------
MockDevice::NoteEvents &MockDevice::NoteEvents::note(TJBox_UInt8 iNoteNumber, TJBox_UInt8 iVelocity, TJBox_UInt16 iAtFrameIndex)
{
  return event({iNoteNumber, iVelocity, iAtFrameIndex});
}


//------------------------------------------------------------------------
// MockDevice::NoteEvents::noteOn
//------------------------------------------------------------------------
MockDevice::NoteEvents &MockDevice::NoteEvents::noteOn(TJBox_UInt8 iNoteNumber, TJBox_UInt8 iVelocity, TJBox_UInt16 iAtFrameIndex)
{
  return event({iNoteNumber, iVelocity, iAtFrameIndex});
}

//------------------------------------------------------------------------
// MockDevice::NoteEvents::noteOn
//------------------------------------------------------------------------
MockDevice::NoteEvents &MockDevice::NoteEvents::noteOff(TJBox_UInt8 iNoteNumber, TJBox_UInt16 iAtFrameIndex)
{
  return event({iNoteNumber, 0, iAtFrameIndex});
}

//------------------------------------------------------------------------
// MockDevice::NoteEvents::allNotesOff
//------------------------------------------------------------------------
MockDevice::NoteEvents &MockDevice::NoteEvents::allNotesOff()
{
  for(int i = Motherboard::FIRST_MIDI_NOTE; i <= Motherboard::LAST_MIDI_NOTE; i++)
    noteOff(i);
  return *this;
}

//------------------------------------------------------------------------
// MockDevice::NoteEvents::clear
//------------------------------------------------------------------------
MockDevice::NoteEvents &MockDevice::NoteEvents::clear()
{
  fNoteEvents.clear();
  return *this;
}

//------------------------------------------------------------------------
// MockDevice::Sample::toString
//------------------------------------------------------------------------
std::string MockAudioDevice::Sample::toString(size_t iFrameCount) const
{
  auto sampleCount = std::min<size_t>(iFrameCount, getFrameCount()) * fChannels;

  std::ostringstream os{};

  if(fData.size() < sampleCount)
  {
    os << "{.fChannels=" << fChannels
       << ",.fSampleRate=" << fSampleRate
       << ",.fData[" << fData.size() << "]{" << stl::Join(fData, ", ") << "}}";
  }
  else
  {
    os << "{.fChannels=" << fChannels
       << ",.fSampleRate=" << fSampleRate
       << ",.fData[" << fData.size() << "]{";
    stl::join(std::begin(fData), std::begin(fData) + sampleCount, os, ",");
    os << ", ... }}";
  }

  return os.str();
}


//------------------------------------------------------------------------
// MockDevice::Sample::operator<<
//------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, MockAudioDevice::Sample const &iBuffer)
{
  constexpr auto MAX = 64;

  if(iBuffer.fData.size() < MAX)
  {
    return os << "{.fChannels=" << iBuffer.fChannels
              << ",.fSampleRate=" << iBuffer.fSampleRate
              << ",.fData[" << iBuffer.fData.size() << "]{" << stl::Join(iBuffer.fData, ", ") << "}}";
  }
  else
  {
    os << "{.fChannels=" << iBuffer.fChannels
       << ",.fSampleRate=" << iBuffer.fSampleRate
       << ",.fData[" << iBuffer.fData.size() << "]{";
    stl::join(std::begin(iBuffer.fData), std::begin(iBuffer.fData) + MAX, os, ",");
    return os << ", ... }}";
  }
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::operator==
//------------------------------------------------------------------------
bool operator==(MockAudioDevice::Sample const &lhs, MockAudioDevice::Sample const &rhs)
{
  if(lhs.fChannels != rhs.fChannels)
    return false;

  if(lhs.fSampleRate != rhs.fSampleRate)
    return false;

  if(lhs.fData.size() != rhs.fData.size())
    return false;

  for(int i = 0; i < lhs.fData.size(); i++)
  {
    if(!stl::almost_equal<TJBox_AudioSample>(lhs.fData[i], rhs.fData[i]))
      return false;
  }

  return true;
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::from
//------------------------------------------------------------------------
MockAudioDevice::Sample MockAudioDevice::Sample::from(resource::Sample iSample)
{
  return {
    iSample.fChannels,
    iSample.fSampleRate,
    std::move(iSample.fData)
  };
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::from
//------------------------------------------------------------------------
MockAudioDevice::Sample MockAudioDevice::Sample::from(StereoBuffer const &iStereoBuffer, TJBox_UInt32 iSampleRate)
{
  MockAudioDevice::Sample res{2, iSampleRate};
  res.fData.reserve(constants::kBatchSize * 2);
  for(int i = 0; i < constants::kBatchSize; i++)
  {
    res.fData.emplace_back(iStereoBuffer.fLeft[i]);
    res.fData.emplace_back(iStereoBuffer.fRight[i]);
  }
  return res;
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::getLeftChannelSample
//------------------------------------------------------------------------
MockAudioDevice::Sample MockAudioDevice::Sample::getLeftChannelSample() const
{
  if(isMono())
    return *this;

  MockAudioDevice::Sample res{1, fSampleRate};

  res.fData.resize(getFrameCount());

  auto iPtr = fData.data();
  auto oPtr = res.fData.data();

  for(int i = 0; i < getFrameCount(); i++)
  {
    *oPtr = *iPtr;
    oPtr++;
    iPtr += fChannels;
  }

  return res;
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::getRightChannelSample
//------------------------------------------------------------------------
MockAudioDevice::Sample MockAudioDevice::Sample::getRightChannelSample() const
{
  if(isMono())
    return *this;

  MockAudioDevice::Sample res{1, fSampleRate};

  res.fData.resize(getFrameCount());

  auto iPtr = fData.data();
  auto oPtr = res.fData.data();

  for(int i = 0; i < getFrameCount(); i++)
  {
    *oPtr = *(iPtr + 1);
    oPtr++;
    iPtr += fChannels;
  }

  return res;
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::append
//------------------------------------------------------------------------
MockAudioDevice::Sample &MockAudioDevice::Sample::append(StereoBuffer const &iAudioBuffer, size_t iFrameCount)
{
  RE_MOCK_ASSERT(isStereo());

  auto size = std::min<size_t>(iFrameCount, constants::kBatchSize);
  fData.reserve(fData.size() + size);

  for(size_t i = 0; i < size; i++)
  {
    fData.emplace_back(iAudioBuffer.fLeft[i]);
    fData.emplace_back(iAudioBuffer.fRight[i]);
  }
  return *this;
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::append
//------------------------------------------------------------------------
MockAudioDevice::Sample &MockAudioDevice::Sample::append(Sample const &iOtherSample, size_t iFrameCount)
{
  auto sampleCount = std::min<size_t>(iFrameCount, iOtherSample.getFrameCount()) * fChannels;
  fData.reserve(fData.size() + sampleCount);
  std::copy(std::begin(iOtherSample.fData), std::begin(iOtherSample.fData) + sampleCount, std::back_inserter(fData));
  return *this;
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::mixWith
//------------------------------------------------------------------------
MockAudioDevice::Sample &MockAudioDevice::Sample::mixWith(MockAudioDevice::Sample const &iOtherSample, size_t iFrameCount)
{
  RE_MOCK_ASSERT(fChannels == iOtherSample.fChannels);
  RE_MOCK_ASSERT(fSampleRate == iOtherSample.fSampleRate);

  auto totalSampleCount = std::min<size_t>(iFrameCount, iOtherSample.getFrameCount()) * fChannels;
  if(totalSampleCount > fData.size())
    fData.resize(totalSampleCount); // increases and add 0
  auto p = fData.data();
  auto o = iOtherSample.fData.data();
  for(size_t i = 0; i < totalSampleCount; i++)
  {
    *p += *o; // mix (meaning sum) this and other sample
    ++p;
    ++o;
  }
  return *this;
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::applyGain
//------------------------------------------------------------------------
MockAudioDevice::Sample &MockAudioDevice::Sample::applyGain(TJBox_Float32 iGain)
{
  stl::for_each(fData, [iGain](auto &s) { s *= iGain; });
  return *this;
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::subSample
//------------------------------------------------------------------------
MockAudioDevice::Sample MockAudioDevice::Sample::subSample(size_t iFromFrame, size_t iFrameCount) const
{
  auto sampleCount = std::min<size_t>(iFrameCount, getFrameCount() - iFromFrame) * fChannels;
  auto startSample = static_cast<ptrdiff_t>(iFromFrame * fChannels);
  auto endSample = static_cast<ptrdiff_t>(startSample + sampleCount);
  RE_MOCK_ASSERT(startSample >= 0 && startSample < fData.size());
  RE_MOCK_ASSERT(endSample >= startSample && endSample <= fData.size());
  Sample res{fChannels, fSampleRate};
  res.fData.reserve(sampleCount);
  std::copy(std::begin(fData) + startSample, std::begin(fData) + endSample, std::back_inserter(res.fData));
  return res;
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::trimLeft
//------------------------------------------------------------------------
MockAudioDevice::Sample MockAudioDevice::Sample::trimLeft() const
{
  auto iter = std::find_if(fData.begin(), fData.end(), [](auto s) { return !isSilent(s); });
  if(iter != fData.end())
  {
    return subSample((iter - fData.begin()) / fChannels);
  }
  else
    return clone();
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::trimRight
//------------------------------------------------------------------------
MockAudioDevice::Sample MockAudioDevice::Sample::trimRight() const
{
  auto end = std::find_if(fData.rbegin(), fData.rend(), [](auto s) { return !isSilent(s); });
  if(end != fData.rend())
  {
    return subSample(0, (end.base() + (fChannels - 1) - fData.begin()) / fChannels);
  }
  else
    return clone();
}

//------------------------------------------------------------------------
// MockAudioDevice::Sample::trim
//------------------------------------------------------------------------
MockAudioDevice::Sample MockAudioDevice::Sample::trim() const
{
  auto start = std::find_if(fData.begin(), fData.end(), [](auto s) { return !isSilent(s); });
  if(start != fData.end())
  {
    auto end = std::find_if(fData.rbegin(), fData.rend(), [](auto s) { return !isSilent(s); });
    if(end != fData.rend())
    {
      auto adjustedStart = (start - fData.begin()) / fChannels;
      auto adjustedEnd = (end.base() + (fChannels - 1) - fData.begin()) / fChannels;
      return subSample(adjustedStart, adjustedEnd - adjustedStart);
    }
    else
      return subSample((start - fData.begin()) / fChannels);
  }
  else
    return clone();
}

namespace impl {

//------------------------------------------------------------------------
// impl::compareTJBox_NoteEvent
//------------------------------------------------------------------------
bool compareTJBox_NoteEvent(TJBox_NoteEvent const &l, TJBox_NoteEvent const &r)
{
  return std::tuple(l.fNoteNumber, l.fVelocity, l.fAtFrameIndex) < std::tuple(r.fNoteNumber, r.fVelocity, r.fAtFrameIndex);
}

}

//------------------------------------------------------------------------
// MockDevice::NoteEvents::operator==
//------------------------------------------------------------------------
bool operator==(MockDevice::NoteEvents const &lhs, MockDevice::NoteEvents const &rhs)
{
  // implementation note: we are comparing note events as sets because the order does not matter
  using TJBox_NoteEvent_set = std::set<TJBox_NoteEvent, decltype(&impl::compareTJBox_NoteEvent)>;
  TJBox_NoteEvent_set ls{lhs.fNoteEvents.begin(), lhs.fNoteEvents.end(), impl::compareTJBox_NoteEvent};
  TJBox_NoteEvent_set rs{rhs.fNoteEvents.begin(), rhs.fNoteEvents.end(), impl::compareTJBox_NoteEvent};
  return ls == rs;
}

//------------------------------------------------------------------------
// MockDevice::NoteEvents::operator!=
//------------------------------------------------------------------------
bool operator!=(MockDevice::NoteEvents const &lhs, MockDevice::NoteEvents const &rhs)
{
  return !(rhs == lhs);
}

//------------------------------------------------------------------------
// MockDevice::NoteEvents::operator<<
//------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, MockDevice::NoteEvents const &events)
{
  return os << "[" << stl::Join(events.fNoteEvents, ", ") << "]";
}

}