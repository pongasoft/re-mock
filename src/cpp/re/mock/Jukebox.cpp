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

#include <algorithm>
#include "Jukebox.h"
#include "Rack.h"

#ifdef __cplusplus
extern "C" {
#endif

TJBox_ValueType JBox_GetType(TJBox_Value iValue)
{
  return static_cast<TJBox_ValueType>(iValue.fSecret[0]);
}

TJBox_Value JBox_MakeNil()
{
  TJBox_Value res;
  res.fSecret[0] = TJBox_ValueType::kJBox_Nil;
  return res;
}

TJBox_Bool JBox_GetBoolean(TJBox_Value iValue)
{
  return re::mock::impl::jbox_get_value<TJBox_Bool>(TJBox_ValueType::kJBox_Boolean, iValue);
}

TJBox_Value JBox_MakeBoolean(TJBox_Bool iBoolean)
{
  return re::mock::impl::jbox_make_value<TJBox_Bool>(TJBox_ValueType::kJBox_Boolean, iBoolean);
}

TJBox_Float64 JBox_GetNumber(TJBox_Value iValue)
{
  return re::mock::impl::jbox_get_value<TJBox_Float64>(TJBox_ValueType::kJBox_Number, iValue);
}

TJBox_Value JBox_MakeNumber(TJBox_Float64 iNumber)
{
  return re::mock::impl::jbox_make_value<TJBox_Float64>(TJBox_ValueType::kJBox_Number, iNumber);
}


TJBox_PropertyRef JBox_MakePropertyRef(TJBox_ObjectRef iObject, const TJBox_PropertyKey iKey)
{
  TJBox_PropertyRef res{};
  res.fObject = iObject;
  // implementation note: for some reason I don't understand, std::begin(iKey) and std::end(iKey) do not work
  std::copy(iKey, iKey + kJBox_MaxPropertyNameLen + 1, std::begin(res.fKey));
  res.fKey[kJBox_MaxPropertyNameLen] = '\0'; // ensures that the string is properly terminated!
  return res;
}

TJBox_ObjectRef JBox_GetMotherboardObjectRef(const TJBox_ObjectName iMOMPath)
{
  return re::mock::Rack::currentMotherboard().getObjectRef(iMOMPath);
}

TJBox_Tag JBox_GetPropertyTag(TJBox_PropertyRef iProperty)
{
  return re::mock::Rack::currentMotherboard().getPropertyTag(iProperty);
}

TJBox_PropertyRef JBox_FindPropertyByTag(TJBox_ObjectRef iObject, TJBox_Tag iTag)
{
  return re::mock::Rack::currentMotherboard().getPropertyRef(iObject, iTag);
}

TJBox_Bool JBox_IsReferencingSameProperty(TJBox_PropertyRef iProperty1, TJBox_PropertyRef iProperty2)
{
  return iProperty1.fObject == iProperty2.fObject &&
         strncmp(iProperty1.fKey, iProperty2.fKey, kJBox_MaxPropertyNameLen + 1) == 0;
}

TJBox_Value JBox_LoadMOMProperty(TJBox_PropertyRef iProperty)
{
  return re::mock::Rack::currentMotherboard().loadProperty(iProperty);
}

TJBox_Value JBox_LoadMOMPropertyByTag(TJBox_ObjectRef iObject, TJBox_Tag iTag)
{
  return re::mock::Rack::currentMotherboard().loadProperty(iObject, iTag);
}

TJBox_Float64 JBox_LoadMOMPropertyAsNumber(TJBox_ObjectRef iObject, TJBox_Tag iTag)
{
  return JBox_GetNumber(JBox_LoadMOMPropertyByTag(iObject, iTag));
}

void JBox_StoreMOMProperty(TJBox_PropertyRef iProperty, TJBox_Value iValue)
{
  return re::mock::Rack::currentMotherboard().storeProperty(iProperty, iValue);
}

void JBox_StoreMOMPropertyByTag(TJBox_ObjectRef iObject, TJBox_Tag iTag, TJBox_Value iValue)
{
  return re::mock::Rack::currentMotherboard().storeProperty(iObject, iTag, iValue);
}

void JBox_StoreMOMPropertyAsNumber(TJBox_ObjectRef iObject, TJBox_Tag iTag,TJBox_Float64 iValue)
{
  JBox_StoreMOMPropertyByTag(iObject, iTag, JBox_MakeNumber(iValue));
}

void JBox_GetDSPBufferData(TJBox_Value iValue, TJBox_AudioFramePos iStartFrame, TJBox_AudioFramePos iEndFrame,
                           TJBox_AudioSample oAudio[])
{
  return re::mock::Rack::currentMotherboard().getDSPBufferData(iValue, iStartFrame, iEndFrame, oAudio);
}

TJBox_DSPBufferInfo JBox_GetDSPBufferInfo(TJBox_Value iValue)
{
  return re::mock::Rack::currentMotherboard().getDSPBufferInfo(iValue);
}

void JBox_SetDSPBufferData(TJBox_Value iValue,
                           TJBox_AudioFramePos iStartFrame,
                           TJBox_AudioFramePos iEndFrame,
                           const TJBox_AudioSample iAudio[])
{
  return re::mock::Rack::currentMotherboard().setDSPBufferData(iValue, iStartFrame, iEndFrame, iAudio);
}

void JBox_Assert(const char iFile[], TJBox_Int32 iLine, const char iFailedExpression[], const char iMessage[])
{
  ABORT_F("%s at %s:%d | %s", iFailedExpression, iFile, iLine, iMessage);
}

void JBox_Trace(
  const char iFile[],
  TJBox_Int32 iLine,
  const char iMessage[])
{
  LOG_F(INFO, "%s:%d | %s", iFile, iLine, iMessage);
}

void JBox_TraceValues(
  const char iFile[],
  TJBox_Int32 iLine,
  const char iTemplate[],
  const TJBox_Value iValues[],
  TJBox_Int32 iValueCount)
{
  throw re::mock::Error("JBox_TraceValues: Not implemented yet");
}

TJBox_UInt32 JBox_GetStringLength(TJBox_Value iValue)
{
  throw re::mock::Error("JBox_GetStringLength: Not implemented yet");
}

void JBox_GetSubstring(
  TJBox_Value iValue,
  TJBox_SizeT iStart,
  TJBox_SizeT iEnd,
  char oString[])
{
  throw re::mock::Error("JBox_GetSubstring: Not implemented yet");
}


const void *JBox_GetNativeObjectRO(TJBox_Value iValue)
{
  return re::mock::Rack::currentMotherboard().getNativeObjectRO(iValue);
}

void *JBox_GetNativeObjectRW(TJBox_Value iValue)
{
  return re::mock::Rack::currentMotherboard().getNativeObjectRW(iValue);
}

TJBox_SampleInfo JBox_GetSampleInfo(TJBox_Value iValue)
{
  throw re::mock::Error("JBox_GetSampleInfo: Not implemented yet");
}

TJBox_SampleMetaData JBox_GetSampleMetaData(TJBox_Value iValue)
{
  throw re::mock::Error("JBox_GetSampleMetaData: Not implemented yet");
}

void JBox_GetSampleData(
  TJBox_Value iValue,
  TJBox_AudioFramePos iStartFrame,
  TJBox_AudioFramePos iEndFrame,
  TJBox_AudioSample oAudio[])
{
  throw re::mock::Error("JBox_GetSampleData: Not implemented yet");
}

TJBox_BLOBInfo JBox_GetBLOBInfo(TJBox_Value iValue)
{
  throw re::mock::Error("JBox_GetBLOBInfo: Not implemented yet");
}

void JBox_GetBLOBData(
  TJBox_Value iValue,
  TJBox_SizeT iStart,
  TJBox_SizeT iEnd,
  TJBox_UInt8 oData[])
{
  throw re::mock::Error("JBox_GetBLOBData: Not implemented yet");
}

void JBox_SetRTStringData(TJBox_PropertyRef iProperty, TJBox_SizeT iSize, const TJBox_UInt8 iData[])
{
  throw re::mock::Error("JBox_SetRTStringData: Not implemented yet");
}

void JBox_OutputNoteEvent(TJBox_NoteEvent iNoteEvent)
{
  throw re::mock::Error("JBox_OutputNoteEvent: Not implemented yet");
}

TJBox_NoteEvent JBox_AsNoteEvent(const TJBox_PropertyDiff &iPropertyDiff)
{
  throw re::mock::Error("JBox_AsNoteEvent: Not implemented yet");
}

TJBox_Int32 JBox_GetOptimalFFTAlignment()
{
  throw re::mock::Error("JBox_GetOptimalFFTAlignment: Not implemented yet");
}

void JBox_FFTRealForward(TJBox_Int32 iFFTSize, TJBox_Float32 ioData[])
{
  throw re::mock::Error("JBox_FFTRealForward: Not implemented yet");
}

void JBox_FFTRealInverse(TJBox_Int32 iFFTSize, TJBox_Float32 ioData[])
{
  throw re::mock::Error("JBox_FFTRealInverse: Not implemented yet");
}


#ifdef __cplusplus
}
#endif
