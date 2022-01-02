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
// Transport::nextBatch
//------------------------------------------------------------------------
void Transport::nextBatch()
{
  fUpdates.clear();

  if(fPlaying)
  {
    fBatchPlayPos.nextBatch();

    if(fBatchPlayPos.getCurrentPlayPos() >= fSongEndPos)
    {
      // we are done (reached the end of the song)
      setPlaying(false);
    }
    else
    {
      setPlayPos(fBatchPlayPos.getCurrentPlayPos());
    }
  }
}

//------------------------------------------------------------------------
// Transport::getBatchLengthPPQ
//------------------------------------------------------------------------
TJBox_Float64 Transport::getBatchLengthPPQ() const
{
  if(!fBatchLengthPPQ)
  {
    fBatchLengthPPQ = (constants::kBatchSize / static_cast<TJBox_Float64>(fSampleRate)) * ((fTempo / 60.0) * constants::kPPQResolution);
  }
  return *fBatchLengthPPQ;
}

//------------------------------------------------------------------------
// Transport::computeNumBatches
//------------------------------------------------------------------------
TJBox_UInt64 Transport::computeNumBatches(TJBox_Float64 iDurationPPQ) const
{
  BatchPlayPos acc{};
  acc.reset(getBatchLengthPPQ());

  auto numBatches = static_cast<size_t>(iDurationPPQ / getBatchLengthPPQ());

  return acc.peekNext(numBatches) < iDurationPPQ ? numBatches + 1 : numBatches;
}

//------------------------------------------------------------------------
// Transport::getBarLengthPPQ
//------------------------------------------------------------------------
TJBox_Float64 Transport::getBarLengthPPQ() const
{
  if(!fBarLengthPPQ)
  {
    fBarLengthPPQ = 4.0 * constants::kPPQResolution * fTimeSignatureNumerator / fTimeSignatureDenominator;
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
    if(fBatchPlayPos.getCurrentPlayPos() != iPos)
      fBatchPlayPos.reset(getBatchLengthPPQ(), iPos);

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
  {
    fBatchLengthPPQ = std::nullopt;
    fBatchPlayPos.reset(getBatchLengthPPQ(), fPlayPos);
  }
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
  fBatchPlayPos.setLooping(fLoopEnabled, fLoopStartPos, fLoopEndPos);
}

//------------------------------------------------------------------------
// Transport::setLoopStartPos
//------------------------------------------------------------------------
void Transport::setLoopStartPos(TJBox_UInt64 iLoopStartPos)
{
  setNumberValue(fLoopStartPos, iLoopStartPos, kJBox_TransportLoopStartPos);
  fBatchPlayPos.setLooping(fLoopEnabled, fLoopStartPos, fLoopEndPos);
}

//------------------------------------------------------------------------
// Transport::setLoopEndPos
//------------------------------------------------------------------------
void Transport::setLoopEndPos(TJBox_UInt64 iLoopEndPos)
{
  setNumberValue(fLoopEndPos, iLoopEndPos, kJBox_TransportLoopEndPos);
  fBatchPlayPos.setLooping(fLoopEnabled, fLoopStartPos, fLoopEndPos);
}

//------------------------------------------------------------------------
// PlayPos::nextBatch
//------------------------------------------------------------------------
void BatchPlayPos::nextBatch()
{
  fCurrentPlayPos = fNextBatch.fCurrentPlayPos;
  fInitialPlayPos = fNextBatch.fInitialPlayPos;
  fCurrentBatch = fNextBatch.fCurrentBatch;
  fNextBatch = computeNextBatch();
}

//------------------------------------------------------------------------
// BatchPlayPos::computeNextBatch
//------------------------------------------------------------------------
BatchPlayPos::NextBatch BatchPlayPos::computeNextBatch() const
{
  NextBatch batch{fInitialPlayPos,
                  peekNext(fInitialPlayPos, 1),
                  fCurrentBatch + 1};
  if(fLoopingEnabled && fLoopStartPos != fLoopEndPos && fCurrentPlayPos <= fLoopEndPos && batch.fCurrentPlayPos > fLoopEndPos)
  {
    batch.fLoopPlayPos = fLoopEndPos;

    auto oneSamplePPQ = fBatchLengthPPQ / constants::kBatchSize;
    auto ppqBeforeLooping = fLoopEndPos - fInitialPlayPos - (fCurrentBatch * fBatchLengthPPQ);
    if(ppqBeforeLooping < oneSamplePPQ)
      ppqBeforeLooping = oneSamplePPQ; // ensures there is at least 1 sample

    batch.fLoopPlaySampleCount = static_cast<int>((constants::kBatchSize * ppqBeforeLooping) / fBatchLengthPPQ);
    batch.fInitialPlayPos = fLoopStartPos - ppqBeforeLooping;
    batch.fCurrentPlayPos = static_cast<TJBox_Int64>(batch.fInitialPlayPos + fBatchLengthPPQ + fFactor);
    batch.fCurrentBatch = 1;
  }
  return batch;
}

//------------------------------------------------------------------------
// BatchPlayPos::setLooping
//------------------------------------------------------------------------
void BatchPlayPos::setLooping(bool iLoopingEnabled, TJBox_Float64 iLoopStartPos, TJBox_Float64 iLoopEndPos)
{
  fLoopingEnabled = iLoopingEnabled;
  fLoopStartPos = iLoopStartPos;
  fLoopEndPos = iLoopEndPos;
  fNextBatch = computeNextBatch();
}

//------------------------------------------------------------------------
// BatchPlayPos::reset
//------------------------------------------------------------------------
void BatchPlayPos::reset(TJBox_Float64 iBatchLengthPPQ, TJBox_Int64 iCurrentPlayPos)
{
  RE_MOCK_INTERNAL_ASSERT(iBatchLengthPPQ > 0);

  fBatchLengthPPQ = iBatchLengthPPQ;
  fFactor = 1.0 - fBatchLengthPPQ / constants::kBatchSize;
  fInitialPlayPos = iCurrentPlayPos;
  fCurrentPlayPos = iCurrentPlayPos;
  fCurrentBatch = 0;
  fNextBatch = computeNextBatch();
}

}