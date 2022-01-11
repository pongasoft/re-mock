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

#ifndef RE_MOCK_SEQUENCER_H
#define RE_MOCK_SEQUENCER_H

#include <JukeboxTypes.h>
#include <string>
#include <vector>
#include "Errors.h"

namespace re::mock {
class Motherboard;
}

namespace re::mock::sequencer {

struct PPQ {
  PPQ(TJBox_Float64 iCount = 0) : fCount{iCount} { RE_MOCK_ASSERT( iCount >= 0); }

  constexpr TJBox_Float64 count() const { return fCount; }

private:
  TJBox_Float64 fCount{};
};

class Duration;

struct TimeSignature
{
  constexpr TimeSignature(int iNumerator = 4, int iDenominator = 4) : fNumerator{iNumerator}, fDenominator{iDenominator}
  {
    RE_MOCK_ASSERT(fNumerator >= 1 && fNumerator <= 16);
    RE_MOCK_ASSERT(fDenominator == 2 || fDenominator == 4 || fDenominator == 8 || fDenominator == 16);
  }

  constexpr int numerator() const { return fNumerator; }
  constexpr int denominator() const { return fDenominator; }

  Duration oneBar() const;
  Duration oneBeat() const;
  Duration one16th() const;

private:
  int fNumerator;
  int fDenominator;
};

class Time
{
public:
  explicit Time(TJBox_UInt32 iBars = 1, TJBox_UInt32 iBeats = 1, TJBox_UInt32 i16th = 1, TJBox_UInt32 iTicks = 0, TimeSignature iTimeSignature = {}) :
    fTimeSignature{iTimeSignature}, fBars{iBars}, fBeats{iBeats}, f16th{i16th}, fTicks{iTicks}
  {
  }

  TJBox_UInt32 bars() const { return fBars; }
  Time &bars(TJBox_UInt32 iBars) { fBars = iBars; return *this; }

  TJBox_UInt32 beats() const { return fBeats; }
  Time &beats(TJBox_UInt32 iBeats) { fBeats = iBeats; return *this; }

  TJBox_UInt32 sixteenth() const { return f16th; }
  Time &sixteenth(TJBox_UInt32 i16th) { f16th = i16th; return *this; }

  TJBox_UInt32 ticks() const { return fTicks; }
  Time &ticks(TJBox_UInt32 iTicks) { fTicks = iTicks; return *this; }

  TimeSignature signature() const { return fTimeSignature; }
  Time &signature(TimeSignature iTimeSignature) { fTimeSignature = iTimeSignature; return *this; }

  Time &normalize();

  TJBox_Float64 toPPQCount() const;
  PPQ toPPQ() const { return toPPQCount(); }

  Time withOtherSignature(TimeSignature iTimeSignature) { return from(toPPQ(), iTimeSignature); }

  std::string toString() const;

  static Time from(PPQ iPPQ, TimeSignature iTimeSignature = {});

private:
  TimeSignature fTimeSignature;
  TJBox_UInt32 fBars;
  TJBox_UInt32 fBeats;
  TJBox_UInt32 f16th;
  TJBox_UInt32 fTicks;
};

class Duration
{
public:
  constexpr explicit Duration(TJBox_UInt32 iBars = 0, TJBox_UInt32 iBeats = 0, TJBox_UInt32 i16th = 0, TJBox_UInt32 iTicks = 0, TimeSignature iTimeSignature = {}) :
    fTimeSignature{iTimeSignature}, fBars{iBars}, fBeats{iBeats}, f16th{i16th}, fTicks{iTicks}
  {
  }

  TJBox_UInt32 bars() const { return fBars; }
  Duration &bars(TJBox_UInt32 iBars) { fBars = iBars; return *this; }

  TJBox_UInt32 beats() const { return fBeats; }
  Duration &beats(TJBox_UInt32 iBeats) { fBeats = iBeats; return *this; }

  TJBox_UInt32 sixteenth() const { return f16th; }
  Duration &sixteenth(TJBox_UInt32 i16th) { f16th = i16th; return *this; }

  TJBox_UInt32 ticks() const { return fTicks; }
  Duration &ticks(TJBox_UInt32 iTicks) { fTicks = iTicks; return *this; }

  TimeSignature signature() const { return fTimeSignature; }
  Duration &signature(TimeSignature iTimeSignature) { fTimeSignature = iTimeSignature; return *this; }

  Duration &normalize();

  TJBox_Float64 toPPQCount() const;
  PPQ toPPQ() const { return toPPQCount(); }

  static Duration from(PPQ iPPQ, TimeSignature iTimeSignature = {});

  std::string toString() const;

  static const Duration k1Bar_4x4;
  static const Duration k1Beat_4x4;
  static const Duration k1Sixteenth_4x4;

private:
  TimeSignature fTimeSignature;
  TJBox_UInt32 fBars;
  TJBox_UInt32 fBeats;
  TJBox_UInt32 f16th;
  TJBox_UInt32 fTicks;
};

Time operator+(Time const &iTime, Duration const &iDuration);
Duration operator+(Duration const &d1, Duration const &d2);

template<typename Number>
Duration operator*(Duration const &iDuration, Number iFactor) { return Duration::from(iDuration.toPPQCount() * iFactor, iDuration.signature()); }

struct Note
{
  TJBox_UInt8 fNumber{69};
  TJBox_UInt8 fVelocity{100};
  Time fTime{};
  Duration fDuration{0,0,0,120};

  Note &number(TJBox_UInt8 iNumber) { fNumber = iNumber; return *this; }
  Note &velocity(TJBox_UInt8 iVelocity) { fVelocity = iVelocity; return *this; }
  Note &time(Time iTime) { fTime = iTime; return *this; }
  Note &time(TJBox_UInt32 iBars = 1, TJBox_UInt32 iBeats = 1, TJBox_UInt32 i16th = 1, TJBox_UInt32 iTicks = 0) { fTime = Time{iBars, iBeats, i16th, iTicks}; return *this; }
  Note &duration(Duration iDuration) { fDuration = iDuration; return *this; }
  Note &duration(TJBox_UInt32 iBars = 0, TJBox_UInt32 iBeats = 0, TJBox_UInt32 i16th = 0, TJBox_UInt32 iTicks = 0) { fDuration = Duration{iBars, iBeats, i16th, iTicks}; return *this; }
};

class Track
{
public:
  struct Batch
  {
    enum class Type { kFull, kLoopingStart, kLoopingEnd };

    TJBox_Int64 fPlayBatchStartPos{};
    TJBox_Int64 fPlayBatchEndPos{};
    Type fType{};
    TJBox_UInt16 fAtFrameIndex{};
  };

  using SimpleEvent = std::function<void()>;
  using Event = std::function<void(Motherboard &, Batch const &)>;

  static const Event kNoOp;

  static Event wrap(SimpleEvent iEvent);

public:
  explicit Track(TimeSignature iTimeSignature = {}) : fTimeSignature{iTimeSignature} {}

  Track &at(Time iTime) { fCurrentTime = iTime; return *this; }
  Track &after(Duration iDuration) { fCurrentTime = fCurrentTime + iDuration; return *this; }

  Track &noteOn(TJBox_UInt8 iNoteNumber, PPQ iTime, TJBox_UInt8 iNoteVelocity = 100);
  inline Track &noteOn(TJBox_UInt8 iNoteNumber, Time iTime, TJBox_UInt8 iNoteVelocity = 100) { return noteOn(iNoteNumber, iTime.toPPQ(), iNoteVelocity); }
  Track &noteOn(TJBox_UInt8 iNoteNumber, TJBox_UInt8 iNoteVelocity = 100) { return noteOn(iNoteNumber, fCurrentTime, iNoteVelocity); }

  Track &noteOff(TJBox_UInt8 iNoteNumber, PPQ iTime);
  inline Track &noteOff(TJBox_UInt8 iNoteNumber, Time iTime) { return noteOff(iNoteNumber, iTime.toPPQ()); }
  Track &noteOff(TJBox_UInt8 iNoteNumber) { return noteOff(iNoteNumber, fCurrentTime); }

  Track &note(Note const &iNote);

  Track &note(TJBox_UInt8 iNoteNumber, Time iTime, Duration iDuration, TJBox_UInt8 iNoteVelocity = 100) {
    return note(Note{iNoteNumber, iNoteVelocity, iTime, iDuration});
  }
  Track &note(TJBox_UInt8 iNoteNumber, Duration iDuration, TJBox_UInt8 iNoteVelocity = 100) {
    return note(iNoteNumber, fCurrentTime, iDuration, iNoteVelocity);
  }

  Track &event(Time iAt, Event iEvent) { return event(iAt.toPPQ() , std::move(iEvent)); };
  Track &event(Time iAt, SimpleEvent iEvent) { return event(iAt, wrap(std::move(iEvent))); }

  Track &event(Event iEvent) { return event(fCurrentTime, std::move(iEvent)); }
  Track &event(SimpleEvent iEvent) { return event(wrap(std::move(iEvent))); }

  inline Track &event(PPQ iAt, Event iEvent) { return event(iAt.count(), std::move(iEvent)); }
  inline Track &event(PPQ iAt, SimpleEvent iEvent) { return event(iAt.count(), wrap(std::move(iEvent))); }

  Track &onEveryBatch(Event iEvent);
  Track &onEveryBatch(SimpleEvent iEvent) { return onEveryBatch(wrap(iEvent)); }

  Track &reset();

  void setTimeSignature(TimeSignature iTimeSignature) { fTimeSignature = iTimeSignature; }

  void executeEvents(Motherboard &iMotherboard,
                     TJBox_Int64 iPlayBatchStartPos,
                     TJBox_Int64 iPlayBatchEndPos,
                     Batch::Type iBatchType,
                     int iAtFrameIndex,
                     int iBatchSize) const;

  Time getFirstEventTime() const;
  Time getLastEventTime() const;

private:
  struct EventImpl
  {
    int fId;
    TJBox_Float64 fAtPPQ{};
    Event fEvent{};
  };

  void ensureSorted() const;

  Track &event(TJBox_Float64 iAtPPQ, Event iEvent);

  std::vector<EventImpl> const &getEvents() const { ensureSorted(); return fEvents; }

private:
  TimeSignature fTimeSignature;
  Time fCurrentTime{};
  std::vector<EventImpl> fEvents{};
  mutable bool fSorted{true};
  int fLastEventId{};
  std::vector<Event> fOnEveryBatchEvents{};
};

}

#endif //RE_MOCK_SEQUENCER_H