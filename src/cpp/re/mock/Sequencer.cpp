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

#include "Sequencer.h"
#include "fmt.h"
#include "Motherboard.h"

namespace re::mock::sequencer {

//------------------------------------------------------------------------
// Time::normalize
//------------------------------------------------------------------------
Time &Time::normalize()
{
  while(fTicks >= constants::kNumTicksPer16th)
  {
    f16th++;
    fTicks -= constants::kNumTicksPer16th;
  }

  auto max16th = 16 / fTimeSignature.denominator();

  while(f16th > max16th)
  {
    fBeats++;
    f16th -= max16th;
  }

  while(fBeats > fTimeSignature.numerator())
  {
    fBars++;
    fBeats -= fTimeSignature.numerator();
  }

  return *this;
}

//------------------------------------------------------------------------
// Time::toPPQCount
//------------------------------------------------------------------------
TJBox_Float64 Time::toPPQCount() const
{
  auto ppBarResolution =
    static_cast<TJBox_Float64>(4.0 * constants::kPPQResolution * fTimeSignature.numerator() / fTimeSignature.denominator());

  return (fBars - 1) * ppBarResolution
         + (fBeats - 1) * ppBarResolution / fTimeSignature.numerator()
         + (f16th - 1) * constants::kPPT16thResolution
         + (fTicks) * constants::kPPTickResolution
    ;
}

//------------------------------------------------------------------------
// Time::toString
//------------------------------------------------------------------------
std::string Time::toString() const
{
  return fmt::printf("%d.%d.%d.%d", fBars, fBeats, f16th, fTicks);
}

//------------------------------------------------------------------------
// Time::from
//------------------------------------------------------------------------
Time Time::from(PPQ iPPQ, TimeSignature iTimeSignature)
{
  return Time{1,1,1,0,iTimeSignature}.ticks(iPPQ.count() / constants::kPPTickResolution).normalize();
}

//------------------------------------------------------------------------
// Sequencer::Time::operator+
//------------------------------------------------------------------------
Time operator+(Time const &iTime, Duration const &iDuration)
{
  return Time::from(iTime.toPPQCount() + iDuration.toPPQCount(), iTime.signature());
}

//------------------------------------------------------------------------
// Sequencer::Duration::operator+
//------------------------------------------------------------------------
Duration operator+(Duration const &d1, Duration const &d2)
{
  return Duration::from(d1.toPPQCount() + d2.toPPQCount(), d1.signature());
}

//------------------------------------------------------------------------
// Sequencer::Duration::operator*
//------------------------------------------------------------------------
Duration operator*(Duration const &iDuration, TJBox_UInt32 iFactor)
{
  return Duration(iDuration.bars() * iFactor,
                  iDuration.beats() * iFactor,
                  iDuration.sixteenth() * iFactor,
                  iDuration.ticks() * iFactor);
}

//------------------------------------------------------------------------
// Duration::normalize
//------------------------------------------------------------------------
Duration &Duration::normalize()
{
  while(fTicks >= constants::kNumTicksPer16th)
  {
    f16th++;
    fTicks -= constants::kNumTicksPer16th;
  }

  auto max16th = 16 / fTimeSignature.denominator();

  while(f16th > max16th)
  {
    fBeats++;
    f16th -= max16th;
  }

  while(fBeats > fTimeSignature.numerator())
  {
    fBars++;
    fBeats -= fTimeSignature.numerator();
  }

  return *this;
}

//------------------------------------------------------------------------
// Duration::from
//------------------------------------------------------------------------
Duration Duration::from(PPQ iPPQ, TimeSignature iTimeSignature)
{
  return Duration{0,0,0,0,iTimeSignature}.ticks(iPPQ.count() / constants::kPPTickResolution).normalize();
}

//------------------------------------------------------------------------
// Duration::toPPQCount
//------------------------------------------------------------------------
TJBox_Float64 Duration::toPPQCount() const
{
  auto ppBarResolution =
    static_cast<TJBox_Float64>(4.0 * constants::kPPQResolution * fTimeSignature.numerator() / fTimeSignature.denominator());

  return fBars * ppBarResolution
         + fBeats * ppBarResolution / fTimeSignature.numerator()
         + f16th * constants::kPPT16thResolution
         + fTicks * constants::kPPTickResolution
    ;
}

//------------------------------------------------------------------------
// Duration::toString
//------------------------------------------------------------------------
std::string Duration::toString() const
{
  return fmt::printf("%d.%d.%d.%d", fBars, fBeats, f16th, fTicks);
}

//------------------------------------------------------------------------
// Track::event
//------------------------------------------------------------------------
Track &Track::event(TJBox_Float64 iAtPPQ, Event iEvent)
{
  if(!iEvent)
    iEvent = kNoOp;

  auto id = fLastEventId++;

  if(fSorted && !fEvents.empty())
    fSorted = fEvents[fEvents.size() - 1].fAtPPQ <= iAtPPQ;

  fEvents.emplace_back(EventImpl{id, iAtPPQ, std::move(iEvent)});

  return *this;
}

//------------------------------------------------------------------------
// Track::noteOn
//------------------------------------------------------------------------
Track &Track::noteOn(PPQ iTime, TJBox_UInt8 iNoteNumber, TJBox_UInt8 iNoteVelocity)
{
  event(iTime.count(),
        [iNoteNumber, iNoteVelocity](Motherboard &iMotherboard, Batch const &iBatch) {
          // we don't start a note that will be turned off anyway
          if(iBatch.fType != Batch::Type::kLoopingStart)
            iMotherboard.setNoteInEvent(iNoteNumber, iNoteVelocity, iBatch.fAtFrameIndex);
        });

  return *this;
}

//------------------------------------------------------------------------
// Track::noteOff
//------------------------------------------------------------------------
Track &Track::noteOff(PPQ iTime, TJBox_UInt8 iNoteNumber)
{
  event(iTime.count(),
        [iNoteNumber](Motherboard &iMotherboard, Batch const &iBatch) {
          iMotherboard.stopNoteIfOn(iNoteNumber);
        });
  return *this;
}

//------------------------------------------------------------------------
// Track::note
//------------------------------------------------------------------------
Track &Track::note(Note const &iNote)
{
  noteOn(iNote.fTime, iNote.fNumber, iNote.fVelocity);
  noteOff(iNote.fTime + iNote.fDuration, iNote.fNumber);
  return *this;
}

const Duration Duration::k1Bar_4x4(1,0,0,0);
const Duration Duration::k1Beat_4x4(0,1,0,0);
const Duration Duration::k1Sixteenth_4x4(0,0,1,0);

//------------------------------------------------------------------------
// Track::executeEvents
//------------------------------------------------------------------------
void Track::executeEvents(Motherboard &iMotherboard,
                          TJBox_Int64 iPlayBatchStartPos,
                          TJBox_Int64 iPlayBatchEndPos,
                          Batch::Type iBatchType,
                          int iAtFrameIndex,
                          int iBatchSize) const
{
  Batch batch{iPlayBatchStartPos, iPlayBatchEndPos, iBatchType, static_cast<TJBox_UInt16>(iAtFrameIndex)};

  for(auto &event: fOnEveryBatchEvents)
    event(iMotherboard, batch);

  auto const &events = getEvents(); // events are sorted by fAtPPQ!

  // binary search for the first event where fAtPPQ >= iPlayStartPos
  auto event = std::lower_bound(events.begin(), events.end(), static_cast<TJBox_Float64>(iPlayBatchStartPos),
                                [](EventImpl const &iEvent, TJBox_Float64 iValue) {
                                  return iEvent.fAtPPQ < iValue;
                                });

  while(event != events.end() && event->fAtPPQ < iPlayBatchEndPos)
  {
    batch.fAtFrameIndex = std::round(iAtFrameIndex + iBatchSize * (event->fAtPPQ - iPlayBatchStartPos) / (iPlayBatchEndPos - iPlayBatchStartPos));
    if(batch.fAtFrameIndex == constants::kBatchSize)
      batch.fAtFrameIndex = constants::kBatchSize - 1;
    RE_MOCK_INTERNAL_ASSERT(batch.fAtFrameIndex >= 0 && batch.fAtFrameIndex < constants::kBatchSize);
    event->fEvent(iMotherboard, batch);
    event++;
  }
}

//------------------------------------------------------------------------
// Track::kNoOp
//------------------------------------------------------------------------
const Track::Event Track::kNoOp = [](Motherboard &, Batch const &) { };

//------------------------------------------------------------------------
// Track::wrap
//------------------------------------------------------------------------
Track::Event Track::wrap(Track::SimpleEvent iEvent)
{
  if(iEvent)
    return [event = std::move(iEvent)](Motherboard &, Batch const &) {
      event();
    };
  else
    return kNoOp;
}

//------------------------------------------------------------------------
// Track::onEveryBatch
//------------------------------------------------------------------------
Track &Track::onEveryBatch(Track::Event iEvent)
{
  fOnEveryBatchEvents.emplace_back(std::move(iEvent));
  return *this;
}

//------------------------------------------------------------------------
// Track::reset
//------------------------------------------------------------------------
Track &Track::reset()
{
  fEvents.clear();
  fCurrentTime = Time{};
  fOnEveryBatchEvents.clear();
  fSorted = true;
  return *this;
}

//------------------------------------------------------------------------
// Track::ensureSorted
//------------------------------------------------------------------------
void Track::ensureSorted() const
{
  if(!fSorted)
  {
    // Implementation note: technically the fEvents vector should be sorted all the time, but it is more
    // efficient to keep a flag (fSorted) which remains true for as long as events are added in order
    // and only sort once when required. This is why we have to remove const here: from the outside the
    // events are sorted
    auto nonConstThis = const_cast<Track *>(this);

    // we sort the timeline so that events are in the proper fAtBatch
    std::sort(nonConstThis->fEvents.begin(), nonConstThis->fEvents.end(),
              [](EventImpl const &l, EventImpl const &r) {
                if(l.fAtPPQ == r.fAtPPQ)
                  return l.fId < r.fId;
                else
                  return l.fAtPPQ < r.fAtPPQ;
              });
  }

  fSorted = true;

}

//------------------------------------------------------------------------
// Track::getFirstEventTime
//------------------------------------------------------------------------
Time Track::getFirstEventTime() const
{
  RE_MOCK_ASSERT(!fEvents.empty(), "no events");
  return Time::from(getEvents().at(0).fAtPPQ, fTimeSignature);
}

//------------------------------------------------------------------------
// Track::getLastEventTime
//------------------------------------------------------------------------
Time Track::getLastEventTime() const
{
  RE_MOCK_ASSERT(!fEvents.empty(), "no events");
  return Time::from(getEvents().at(fEvents.size() - 1).fAtPPQ, fTimeSignature);
}

//------------------------------------------------------------------------
// TimeSignature::oneBar
//------------------------------------------------------------------------
Duration TimeSignature::oneBar() const
{
  return Duration(1,0,0,0,*this);
}

//------------------------------------------------------------------------
// TimeSignature::oneBeat
//------------------------------------------------------------------------
Duration TimeSignature::oneBeat() const
{
  return Duration(0,1,0,0,*this);
}

//------------------------------------------------------------------------
// TimeSignature::one16th
//------------------------------------------------------------------------
Duration TimeSignature::one16th() const
{
  return Duration(0,0,1,0,*this);
}

}