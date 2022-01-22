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
class Rack;
}

namespace re::mock::sequencer {

/**
 * Small wrapper around a PPQ value (PPQ stands for Pulse Per Quarter Note) to make the APIs more obvious */
struct PPQ {
  PPQ(TJBox_Float64 iCount = 0) : fCount{iCount} { RE_MOCK_ASSERT( iCount >= 0); }

  constexpr TJBox_Float64 count() const { return fCount; }

private:
  TJBox_Float64 fCount{};
};

class Duration;

/**
 * Defines a time signature (numerator / denominator) */
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

/**
 * Represents the concept of time on the sequencer track as defined by `<bars>.<beats>.<sixteenth>.<ticks>`
 * (+ a time signature). This is the same concept as can be seen on the transport bar in Reason.
 *
 * @note Time 0 is defined by `1.1.1.0`
 */
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

  /**
   * Normalization is the process of making sure that every entry in the time "fits" the signature. For example,
   * with a 4/4 time signature, `1.5.1.0` gets normalized to `2.1.1.0` */
  Time &normalize();

  //! Converts this time to a PPQ
  TJBox_Float64 toPPQCount() const;

  //! Converts this time to a PPQ
  PPQ toPPQ() const { return toPPQCount(); }

  Time withOtherSignature(TimeSignature iTimeSignature) { return from(toPPQ(), iTimeSignature); }

  //! Returns a string representation of this time (ex: `1.2.3.120`)
  std::string toString() const;

  //! Builds a time from PPQ / time signature
  static Time from(PPQ iPPQ, TimeSignature iTimeSignature = {});

private:
  TimeSignature fTimeSignature;
  TJBox_UInt32 fBars;
  TJBox_UInt32 fBeats;
  TJBox_UInt32 f16th;
  TJBox_UInt32 fTicks;
};

/**
 * Represents the concept of duration on the sequencer track as defined by `<bars>.<beats>.<sixteenth>.<ticks>`. This
 * is the same concept as can be seen on the transport bar in Reason, for example when selecting a Midi note.
 *
 * @note Duration 0 is defined by `0.0.0.0` */
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

  /**
   * Normalization is the process of making sure that every entry in the time "fits" the signature. For example,
   * with a 4/4 time signature, `1.5.1.0` gets normalized to `2.1.1.0` */
  Duration &normalize();

  //! Converts this time to a PPQ
  TJBox_Float64 toPPQCount() const;

  //! Converts this time to a PPQ
  PPQ toPPQ() const { return toPPQCount(); }

  //! Builds a duration from PPQ / time signature
  static Duration from(PPQ iPPQ, TimeSignature iTimeSignature = {});

  //! Returns a string representation of this duration (ex: `1.0.0.0`)
  std::string toString() const;

  //! Convenient shortcut for 1 bar (for a 4/4 time signature): `1.0.0.0`
  static const Duration k1Bar_4x4;

  //! Convenient shortcut for 1 beat (for a 4/4 time signature): `0.1.0.0`
  static const Duration k1Beat_4x4;

  //! Convenient shortcut for 1 16th (for a 4/4 time signature): `0.0.1.0`
  static const Duration k1Sixteenth_4x4;

private:
  TimeSignature fTimeSignature;
  TJBox_UInt32 fBars;
  TJBox_UInt32 fBeats;
  TJBox_UInt32 f16th;
  TJBox_UInt32 fTicks;
};

//! time + duration => time
Time operator+(Time const &iTime, Duration const &iDuration);

//! duration + duration => duration
Duration operator+(Duration const &d1, Duration const &d2);

//! duration * number => duration: ex: Duration::k1Beat_4x4 * 2 => 2 beats
template<typename Number>
Duration operator*(Duration const &iDuration, Number iFactor) { return Duration::from(iDuration.toPPQCount() * iFactor, iDuration.signature()); }

/**
 * A note on the sequencer track: a (midi) note number, a velocity, a time (when it starts) and a duration */
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

/**
 * This class represents a track in the sequencer. There is one per device/extension accessible via
 * `tester.sequencerTrack()` or `Extension::getSequencerTrack()`. It is meant to represent the same concept of the
 * sequencer track associated to a device in Reason: contains notes as well as other generic events (like property
 * value changes).
 *
 * ```cpp
 * tester.sequencerTrack()
 *   .note(sequencer::Note{}.number(Midi::C(3)).time(2,1,1,0).duration(0,0,0,120)
 *   .at(sequencer::Time(2,2,1,0)) // all events after this that don't specify a time use this time
 *   .note(Midi::C(4), sequencer::Duration(0,0,1,0))
 *   .event([&tester]) { tester.device().setNum("/custom_properties/gain", 0.8); })
 * ```
 *
 * This example sets a C3 note starting at 2.1.1.0 for a duration of 120 ticks, a C4 note that starts at 2.2.1.0 and
 * lasts 1 16th, and finally changes the custom property "gain" to 0.8 at 2.2.1.0.
 *
 * @note It is possible to populate the sequencer track with (imported) midi notes. Check `DeviceTester::importMidi()`
 *       and `Extension::importMidiNotes()`
 *
 */
class Track
{
public:
  /**
   * Contains information about the batch during the callback of an event (see `Event` below) */
  struct Batch
  {
    enum class Type { kFull, kLoopingStart, kLoopingEnd };

    TJBox_Int64 fPlayBatchStartPos{};
    TJBox_Int64 fPlayBatchEndPos{};
    Type fType{};
    TJBox_UInt16 fAtFrameIndex{};
  };

  //! A simpler definition of an event (no arguments)
  using SimpleEvent = std::function<void()>;

  /**
   * Definition of an event which is a function that takes the motherboard and a description of the batch in which
   * the event is happening. */
  using Event = std::function<void(Motherboard &, Batch const &)>;

  //! An event that does nothing
  static const Event kNoOp;

  //! Wraps a simple event into an event (ignores parameters)
  static Event wrap(SimpleEvent iEvent);

public:
  //! Constructor
  explicit Track(TimeSignature iTimeSignature = {}) : fTimeSignature{iTimeSignature} {}

  /**
   * Moves the "current" time to the time provided. All apis which do not take a time use this "current" time.
   *
   * ```cpp
   * tester.sequencerTrack()
   *   .at(sequencer::Time(2,2,1,0)) // all events after this that don't specify a time use this time
   *   .note(Midi::C(4), sequencer::Duration(0,0,1,0))
   *   .event([&tester]) { tester.device().setNum("/custom_properties/gain", 0.8); })
   *
   * // 100% equivalent to
   * tester.sequencerTrack()
   *   .note(sequencer::Time(2,2,1,0), Midi::C(4), sequencer::Duration(0,0,1,0))
   *   .event(sequencer::Time(2,2,1,0), [&tester]) { tester.device().setNum("/custom_properties/gain", 0.8); })
   * ```
   */
  Track &at(Time iTime) { fCurrentTime = iTime; return *this; }

  /**
   * Moves the current time by `iDuration`
   *
   * @see `at(Time)` */
  Track &after(Duration iDuration) { fCurrentTime = fCurrentTime + iDuration; return *this; }

  //! Adds a "note on" event to the sequencer track at the time (provided in PPQ)
  Track &noteOn(PPQ iTime, TJBox_UInt8 iNoteNumber, TJBox_UInt8 iNoteVelocity = 100);

  //! Adds a "note on" event to the sequencer track at the time provided
  inline Track &noteOn(Time iTime, TJBox_UInt8 iNoteNumber, TJBox_UInt8 iNoteVelocity = 100) { return noteOn(iTime.toPPQ(), iNoteNumber, iNoteVelocity); }

  //! Adds a "note on" event to the sequencer track at the "current" time (defined by `at` or `after`)
  Track &noteOn(TJBox_UInt8 iNoteNumber, TJBox_UInt8 iNoteVelocity = 100) { return noteOn(fCurrentTime, iNoteNumber, iNoteVelocity); }

  //! Adds a "note off" event to the sequencer track at the time (provided in PPQ)
  Track &noteOff(PPQ iTime, TJBox_UInt8 iNoteNumber);

  //! Adds a "note off" event to the sequencer track at the time provided
  inline Track &noteOff(Time iTime, TJBox_UInt8 iNoteNumber) { return noteOff(iTime.toPPQ(), iNoteNumber); }

  //! Adds a "note off" event to the sequencer track at the "current" time (defined by `at` or `after`)
  Track &noteOff(TJBox_UInt8 iNoteNumber) { return noteOff(fCurrentTime, iNoteNumber); }

  //! Adds a note to the sequencer track (2 events: a "note on" and "note off" events)
  Track &note(Note const &iNote);

  //! Adds a note to the sequencer track (2 events: a "note on" event at the provided time and "note off" event after the duration)
  Track &note(Time iTime, TJBox_UInt8 iNoteNumber, Duration iDuration, TJBox_UInt8 iNoteVelocity = 100) {
    return note(Note{iNoteNumber, iNoteVelocity, iTime, iDuration});
  }

  //! Adds a note to the sequencer track (2 events: a "note on" event at the "current" time and "note off" event after the duration)
  Track &note(TJBox_UInt8 iNoteNumber, Duration iDuration, TJBox_UInt8 iNoteVelocity = 100) {
    return note(fCurrentTime, iNoteNumber, iDuration, iNoteVelocity);
  }

  //! Add an event that happens at the provided time
  Track &event(Time iAt, Event iEvent) { return event(iAt.toPPQ() , std::move(iEvent)); };

  //! Add a (simple) event that happens at the provided time
  Track &event(Time iAt, SimpleEvent iEvent) { return event(iAt, wrap(std::move(iEvent))); }

  //! Add an event that happens at the "current" time
  Track &event(Event iEvent) { return event(fCurrentTime, std::move(iEvent)); }

  //! Add a (simple) event that happens at the "current" time
  Track &event(SimpleEvent iEvent) { return event(wrap(std::move(iEvent))); }

  //! Add an event that happens at the provided time (in PPQ)
  inline Track &event(PPQ iAt, Event iEvent) { return event(iAt.count(), std::move(iEvent)); }

  //! Add a (simple) event that happens at the provided time (in PPQ)
  inline Track &event(PPQ iAt, SimpleEvent iEvent) { return event(iAt.count(), wrap(std::move(iEvent))); }

   //! Add an event that happens on every batch
  Track &onEveryBatch(Event iEvent);

  //! Add a (simple) event that happens on every batch
  Track &onEveryBatch(SimpleEvent iEvent) { return onEveryBatch(wrap(iEvent)); }

  //! Clear all events and reset the "current" time to `1.1.1.0`
  Track &reset();

  //! Change the time signature associated to this track
  void setTimeSignature(TimeSignature iTimeSignature) { fTimeSignature = iTimeSignature; }

  //! Returns the time at which the first event happens
  Time getFirstEventTime() const;

  //! Returns the time at which the last event happens
  Time getLastEventTime() const;

  friend class re::mock::Rack;

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

  //! Execute
  void executeEvents(Motherboard &iMotherboard,
                     TJBox_Int64 iPlayBatchStartPos,
                     TJBox_Int64 iPlayBatchEndPos,
                     Batch::Type iBatchType,
                     int iAtFrameIndex,
                     int iBatchSize) const;

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