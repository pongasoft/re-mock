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

#include "Transport.h"
#include "Motherboard.h"

namespace re::mock {

//------------------------------------------------------------------------
// Transport::initMotherboard
//------------------------------------------------------------------------
void Transport::initMotherboard(Motherboard &iMotherboard) const
{
  auto ref = iMotherboard.getObjectRef("/transport");

  iMotherboard.storeProperty(ref, kJBox_TransportPlaying, JBox_MakeBoolean(fPlaying));
  iMotherboard.storeProperty(ref, kJBox_TransportPlayPos, JBox_MakeNumber(fPlayPos));
  iMotherboard.storeProperty(ref, kJBox_TransportTempo, JBox_MakeNumber(fTempo));
  iMotherboard.storeProperty(ref, kJBox_TransportFilteredTempo, JBox_MakeNumber(fFilteredTempo));
  iMotherboard.storeProperty(ref, kJBox_TransportTempoAutomation, JBox_MakeBoolean(fTempoAutomation));
  iMotherboard.storeProperty(ref, kJBox_TransportTimeSignatureNumerator, JBox_MakeNumber(fTimeSignatureNumerator));
  iMotherboard.storeProperty(ref, kJBox_TransportTimeSignatureDenominator, JBox_MakeNumber(fTimeSignatureDenominator));
  iMotherboard.storeProperty(ref, kJBox_TransportLoopEnabled, JBox_MakeBoolean(fLoopEnabled));
  iMotherboard.storeProperty(ref, kJBox_TransportLoopStartPos, JBox_MakeNumber(fLoopStartPos));
  iMotherboard.storeProperty(ref, kJBox_TransportLoopEndPos, JBox_MakeNumber(fLoopEndPos));
  iMotherboard.storeProperty(ref, kJBox_TransportBarStartPos, JBox_MakeNumber(fBarStartPos));

}

//------------------------------------------------------------------------
// Transport::updateMotherboard
//------------------------------------------------------------------------
void Transport::updateMotherboard(Motherboard &iMotherboard) const
{
  if(!fUpdates.empty())
  {
    auto ref = iMotherboard.getObjectRef("/transport");
    for(auto &[tag, value]: fUpdates)
      iMotherboard.storeProperty(ref, tag, value);
  }
}

//------------------------------------------------------------------------
// Transport::nextFrame
//------------------------------------------------------------------------
void Transport::nextFrame()
{
  fUpdates.clear();

  if(fPlaying)
  {
    auto newPlayPos = static_cast<TJBox_Int64>(std::round(fPlayPos + getBatchLengthPPQ()));
    if(fLoopEnabled && fPlayPos <= fLoopEndPos && newPlayPos > fLoopEndPos)
      newPlayPos = fLoopStartPos;
    setPlayPos(newPlayPos);
  }
}

//------------------------------------------------------------------------
// Transport::getBatchLengthPPQ
//------------------------------------------------------------------------
TJBox_Float64 Transport::getBatchLengthPPQ() const
{
  if(!fBatchLengthPPQ)
  {
    fBatchLengthPPQ = (kBatchSize / static_cast<TJBox_Float64>(fSampleRate)) * ((fTempo / 60.0) * kPPQResolution);
  }
  return *fBatchLengthPPQ;
}

//------------------------------------------------------------------------
// Transport::getBarLengthPPQ
//------------------------------------------------------------------------
TJBox_Float64 Transport::getBarLengthPPQ() const
{
  if(!fBarLengthPPQ)
  {
    fBarLengthPPQ = 4.0 * kPPQResolution * fTimeSignatureNumerator / fTimeSignatureDenominator;
  }
  return *fBarLengthPPQ;
}

//------------------------------------------------------------------------
// Transport::setNumberValue
//------------------------------------------------------------------------
template<typename Number>
bool Transport::setNumberValue(Number &oCurrentValue, Number iNewValue, TJBox_TransportTag iTag)
{
  if(iNewValue != oCurrentValue)
  {
    oCurrentValue = iNewValue;
    fUpdates[iTag] = JBox_MakeNumber(iNewValue);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// Transport::setBooleanValue
//------------------------------------------------------------------------
bool Transport::setBooleanValue(bool &oCurrentValue, bool iNewValue, TJBox_TransportTag iTag)
{
  if(iNewValue != oCurrentValue)
  {
    oCurrentValue = iNewValue;
    fUpdates[iTag] = JBox_MakeBoolean(iNewValue);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
// Transport::setPlaying
//------------------------------------------------------------------------
void Transport::setPlaying(bool iPlaying)
{
  setBooleanValue(fPlaying, iPlaying, kJBox_TransportPlaying);
}

//------------------------------------------------------------------------
// Transport::setPlayPos
//------------------------------------------------------------------------
void Transport::setPlayPos(TJBox_Int64 iPos)
{
  if(setNumberValue(fPlayPos, iPos, kJBox_TransportPlayPos))
  {
    // recomputes the bar start pos
    recomputeBarStartPos();
  }
}

//------------------------------------------------------------------------
// Transport::recomputeBarStartPos
//------------------------------------------------------------------------
void Transport::recomputeBarStartPos()
{
  // recomputes the bar start pos
  auto barLengthPPQ = static_cast<TJBox_Int64>(getBarLengthPPQ());
  TJBox_UInt64 newBarStartPos = fPlayPos <= 0 ? 0 : (fPlayPos / barLengthPPQ) * barLengthPPQ;
  setNumberValue(fBarStartPos, newBarStartPos, kJBox_TransportBarStartPos);
}

//------------------------------------------------------------------------
// Transport::setTempo
//------------------------------------------------------------------------
void Transport::setTempo(TJBox_Float64 iTempo)
{
  if(setNumberValue(fTempo, iTempo, kJBox_TransportTempo))
    fBatchLengthPPQ = std::nullopt;
}

//------------------------------------------------------------------------
// Transport::setFilteredTempo
//------------------------------------------------------------------------
void Transport::setFilteredTempo(TJBox_Float64 iFilteredTempo)
{
  setNumberValue(fFilteredTempo, iFilteredTempo, kJBox_TransportFilteredTempo);
}

//------------------------------------------------------------------------
// Transport::setTempoAutomation
//------------------------------------------------------------------------
void Transport::setTempoAutomation(bool iTempoAutomation)
{
  setBooleanValue(fTempoAutomation, iTempoAutomation, kJBox_TransportTempoAutomation);
}

//------------------------------------------------------------------------
// Transport::setTimeSignatureNumerator
//------------------------------------------------------------------------
void Transport::setTimeSignatureNumerator(int iNumerator)
{
  if(setNumberValue(fTimeSignatureNumerator, iNumerator, kJBox_TransportTimeSignatureNumerator))
    fBarLengthPPQ = std::nullopt;
}

//------------------------------------------------------------------------
// Transport::setTimeSignatureNumerator
//------------------------------------------------------------------------
void Transport::setTimeSignatureDenominator(int iDenominator)
{
  if(setNumberValue(fTimeSignatureDenominator, iDenominator, kJBox_TransportTimeSignatureDenominator))
    fBarLengthPPQ = std::nullopt;
}

//------------------------------------------------------------------------
// Transport::setLoopEnabled
//------------------------------------------------------------------------
void Transport::setLoopEnabled(bool iLoopEnabled)
{
  setBooleanValue(fLoopEnabled, iLoopEnabled, kJBox_TransportLoopEnabled);
}

//------------------------------------------------------------------------
// Transport::setLoopStartPos
//------------------------------------------------------------------------
void Transport::setLoopStartPos(TJBox_UInt64 iLoopStartPos)
{
  setNumberValue(fLoopStartPos, iLoopStartPos, kJBox_TransportLoopStartPos);
}

//------------------------------------------------------------------------
// Transport::setLoopEndPos
//------------------------------------------------------------------------
void Transport::setLoopEndPos(TJBox_UInt64 iLoopEndPos)
{
  setNumberValue(fLoopEndPos, iLoopEndPos, kJBox_TransportLoopEndPos);
}


}