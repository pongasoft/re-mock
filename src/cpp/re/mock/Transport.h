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

#ifndef RE_MOCK_TRANSPORT_H
#define RE_MOCK_TRANSPORT_H

#include <JukeboxTypes.h>
#include "Constants.h"
#include "Errors.h"
#include <map>
#include <cmath>
#include <optional>

namespace re::mock {

class Motherboard;
class Rack;

class BatchPlayPos
{
public:
  void reset(TJBox_Float64 iBatchLengthPPQ, TJBox_Int64 iCurrentPlayPos = 0.0);

  void nextBatch();

  constexpr TJBox_Int64 peekNext(size_t iCount = 1) const { return peekNext(fInitialPlayPos, iCount); }

  constexpr TJBox_Int64 getCurrentPlayPos() const { return fCurrentPlayPos; }
  constexpr TJBox_Int64 getNextPlayPos() const { RE_MOCK_INTERNAL_ASSERT(fNextBatch.fCurrentPlayPos > -1); return fNextBatch.fCurrentPlayPos; }
  constexpr TJBox_Int64 getLoopStartPos() const { return fLoopStartPos; }
  constexpr TJBox_Int64 getLoopPlayPos() const { return fNextBatch.fLoopPlayPos; }
  int getLoopPlaySampleCount() const { return fNextBatch.fLoopPlaySampleCount; }

  bool isLooping() const { return fNextBatch.fLoopPlayPos > 0; }

  void setLooping(bool iLoopingEnabled, TJBox_Float64 iLoopStartPos, TJBox_Float64 iLoopEndPos);

private:
  struct NextBatch
  {
    TJBox_Float64 fInitialPlayPos{-1};
    TJBox_Int64 fCurrentPlayPos{-1};
    TJBox_Int64 fCurrentBatch{-1};
    TJBox_Int64 fLoopPlayPos{-1};
    int fLoopPlaySampleCount{-1};
  };

  constexpr TJBox_Int64 peekNext(TJBox_Float64 iInitialPlayPos, size_t iCount) const {
    auto exact = iInitialPlayPos + (fCurrentBatch + iCount) * fBatchLengthPPQ + fFactor;
    auto int64 = static_cast<TJBox_Int64>(exact);
//    RE_MOCK_LOG_INFO("%ld / %.7f / %ld", iCount, exact, int64);
    if(int64 + 1 - exact < 1.e-8)
      int64++;
    return int64;
  }

  NextBatch computeNextBatch() const;

private:
  TJBox_Float64 fBatchLengthPPQ{};
  TJBox_Float64 fFactor{};
  TJBox_Float64 fInitialPlayPos{};
  bool fLoopingEnabled{};
  TJBox_Int64 fLoopStartPos{};
  TJBox_Int64 fLoopEndPos{};

  TJBox_Int64 fCurrentPlayPos{};
  TJBox_Int64 fCurrentBatch{};

  NextBatch fNextBatch{};
};

class Transport
{
public:
  explicit Transport(int iSampleRate) : fSampleRate{iSampleRate} {
    fBatchPlayPos.reset(getBatchLengthPPQ(), getPlayPos());
  }

  void initMotherboard(Motherboard &iMotherboard) const;
  void updateMotherboard(Motherboard &iMotherboard) const;

  void nextBatch();

  int getSampleRate() const { return fSampleRate; }

  bool getPlaying() const { return fPlaying; }
  void setPlaying(bool iPlaying);

  TJBox_Int64 getPlayPos() const { return fPlayPos; }
  void setPlayPos(TJBox_Int64 iPos);

  TJBox_Float64 getTempo() const { return fTempo; }
  void setTempo(TJBox_Float64 iTempo);

  TJBox_Float64 getFilteredTempo() const { return fFilteredTempo; }
  void setFilteredTempo(TJBox_Float64 iFilteredTempo);

  bool getTempoAutomation() const { return fTempoAutomation; }
  void setTempoAutomation(bool iTempoAutomation);

  int getTimeSignatureNumerator() const { return fTimeSignatureNumerator; }
  void setTimeSignatureNumerator(int iNumerator);

  int getTimeSignatureDenominator() const { return fTimeSignatureDenominator; }
  void setTimeSignatureDenominator(int iDenominator);

  bool getLoopEnabled() const { return fLoopEnabled; }
  void setLoopEnabled(bool iLoopEnabled);

  TJBox_UInt64 getLoopStartPos() const { return fLoopStartPos; }
  void setLoopStartPos(TJBox_UInt64 iLoopStartPos);

  TJBox_UInt64 getLoopEndPos() const { return fLoopEndPos; }
  void setLoopEndPos(TJBox_UInt64 iLoopEndPos);

  TJBox_UInt64 getBarStartPos() const { return fBarStartPos; }

  TJBox_Int64 getPlayBatchEndPos() const { return fBatchPlayPos.getNextPlayPos(); }

  TJBox_UInt64 getSongEndPos() const { return fSongEndPos; }
  void setSongEndPos(TJBox_UInt64 iSongEndPos) { fSongEndPos = iSongEndPos; }

  TJBox_UInt64 computeNumBatches(TJBox_Float64 iDurationPPQ) const;

  friend class Rack;

protected:
  TJBox_Float64 getBatchLengthPPQ() const;
  TJBox_Float64 getBarLengthPPQ() const;

  void recomputeBarStartPos();

  template<typename Number>
  bool setNumberValue(Number &oCurrentValue, Number iNewValue, TJBox_TransportTag iTag);

  bool setBooleanValue(bool &oCurrentValue, bool iNewValue, TJBox_TransportTag iTag);

protected:

  BatchPlayPos const &getBatchPlayPos() const { return fBatchPlayPos; }

private:
  int fSampleRate;

  bool fPlaying{};
  TJBox_Int64 fPlayPos{}; // can be negative
  TJBox_Float64 fTempo{120}; // [1.0, 999.999]
  TJBox_Float64 fFilteredTempo{120};
  bool fTempoAutomation{};
  int fTimeSignatureNumerator{4}; // number of beats (defined by denominator) per bar [1,16]
  int fTimeSignatureDenominator{4}; // 2, 4, 8, 16 with 4 == quarter note
  bool fLoopEnabled{};
  TJBox_UInt64 fLoopStartPos{};
  TJBox_UInt64 fLoopEndPos{}; // **is** allowed to be less than fLoopStartPos
  TJBox_UInt64 fBarStartPos{};

  TJBox_UInt64 fSongEndPos{};

  BatchPlayPos fBatchPlayPos{};
  mutable std::optional<TJBox_Float64> fBatchLengthPPQ{};
  mutable std::optional<TJBox_Float64> fBarLengthPPQ{};

  std::map<TJBox_TransportTag, TJBox_Value> fUpdates{};
};

}

#endif //RE_MOCK_TRANSPORT_H