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
#include <map>

namespace re::mock {

class Motherboard;

class Transport
{
public:
  constexpr static TJBox_UInt64 kPPQResolution = 15360;
  constexpr static auto kBatchSize = 64;

  explicit Transport(int iSampleRate) : fSampleRate{iSampleRate} {}

  void initMotherboard(Motherboard &iMotherboard) const;
  void updateMotherboard(Motherboard &iMotherboard) const;

  void nextFrame();

  void start() { setPlaying(true);}
  void stop() { setPlaying(false);}

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

protected:
  TJBox_Float64 getBatchLengthPPQ() const;
  TJBox_Float64 getBarLengthPPQ() const;

  void recomputeBarStartPos();

  template<typename Number>
  bool setNumberValue(Number &oCurrentValue, Number iNewValue, TJBox_TransportTag iTag);

  bool setBooleanValue(bool &oCurrentValue, bool iNewValue, TJBox_TransportTag iTag);

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

  mutable std::optional<TJBox_Float64> fBatchLengthPPQ{};
  mutable std::optional<TJBox_Float64> fBarLengthPPQ{};

  std::map<TJBox_TransportTag, TJBox_Value> fUpdates{};
};

}

#endif //RE_MOCK_TRANSPORT_H