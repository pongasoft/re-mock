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
Time &Time::normalize(TimeSignature iTimeSignature)
{
  while(fTicks >= kNumTicksPer16th)
  {
    f16th++;
    fTicks -= kNumTicksPer16th;
  }

  auto max16th = 16 / iTimeSignature.denominator();

  while(f16th > max16th)
  {
    fBeats++;
    f16th -= max16th;
  }

  while(fBeats > iTimeSignature.numerator())
  {
    fBars++;
    fBeats -= iTimeSignature.numerator();
  }

  return *this;
}

//------------------------------------------------------------------------
// Time::toPPQ
//------------------------------------------------------------------------
PPQ Time::toPPQ(TimeSignature iTimeSignature) const
{
  auto ppBarResolution =
    static_cast<TJBox_Float64>(4.0 * kPPQResolution * iTimeSignature.numerator() / iTimeSignature.denominator());

  return (fBars - 1) * ppBarResolution
         + (fBeats - 1) * ppBarResolution / iTimeSignature.numerator()
         + (f16th - 1) * kPPT16thResolution
         + (fTicks) * kPPTickResolution
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
// Sequencer::Time::operator+
//------------------------------------------------------------------------
Time operator+(Time const &iTime, Duration const &iDuration)
{
  return Time(iTime.bars() + iDuration.bars(),
              iTime.beats() + iDuration.beats(),
              iTime.sixteenth() + iDuration.sixteenth(),
              iTime.ticks() + iDuration.ticks());
}

//------------------------------------------------------------------------
// Duration::toPPQResolution
//------------------------------------------------------------------------
PPQ Duration::toPPQ(TimeSignature iTimeSignature) const
{
  auto ppBarResolution =
    static_cast<TJBox_Float64>(4.0 * kPPQResolution * iTimeSignature.numerator() / iTimeSignature.denominator());

  return fBars * ppBarResolution
         + fBeats * ppBarResolution / iTimeSignature.numerator()
         + f16th * kPPT16thResolution
         + fTicks * kPPTickResolution
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
// Track::note
//------------------------------------------------------------------------
Track &Track::note(Note const &iNote)
{
  // note on
  event(iNote.fTime.toPPQ(fTimeSignature),
        [iNote](Motherboard &iMotherboard, TJBox_Int64 iPlayBatchStartPos, TJBox_Int64 iPlayBatchEndPos, TJBox_UInt16 iAtFrameIndex) {
          iMotherboard.setNoteInEvent(iNote.fNumber, iNote.fVelocity, iAtFrameIndex);
        });

  // note off
  event((iNote.fTime + iNote.fDuration).toPPQ(fTimeSignature),
        [iNote](Motherboard &iMotherboard, TJBox_Int64 iPlayBatchStartPos, TJBox_Int64 iPlayBatchEndPos, TJBox_UInt16 iAtFrameIndex) {
          iMotherboard.setNoteInEvent(iNote.fNumber, 0, iAtFrameIndex);
        });

  return *this;
}

const Duration Duration::k1Bar(1,0,0,0);
const Duration Duration::k1Beat(0,1,0,0);

//------------------------------------------------------------------------
// Track::executeEvents
//------------------------------------------------------------------------
void Track::executeEvents(Motherboard &iMotherboard,
                          TJBox_Int64 iPlayBatchStartPos,
                          TJBox_Int64 iPlayBatchEndPos) const
{
  for(auto &event: fOnEveryBatchEvents)
    event(iMotherboard, iPlayBatchStartPos, iPlayBatchEndPos, 0);

  auto const &events = getEvents(); // events are sorted by fAtPPQ!

  // binary search for the first event where fAtPPQ >= iPlayStartPos
  auto event = std::lower_bound(events.begin(), events.end(), static_cast<TJBox_Float64>(iPlayBatchStartPos),
                                [](EventImpl const &iEvent, TJBox_Float64 iValue) {
                                  return iEvent.fAtPPQ < iValue;
                                });

  while(event != events.end() && event->fAtPPQ < iPlayBatchEndPos)
  {
    auto frameIndex = kBatchSize * (event->fAtPPQ - iPlayBatchStartPos) / (iPlayBatchEndPos - iPlayBatchStartPos);
    event->fEvent(iMotherboard, iPlayBatchStartPos, iPlayBatchEndPos, static_cast<TJBox_UInt16>(frameIndex));
    event++;
  }
}

//------------------------------------------------------------------------
// Track::kNoOp
//------------------------------------------------------------------------
const Track::Event Track::kNoOp = [](Motherboard &, TJBox_Int64, TJBox_Int64, TJBox_UInt16) { };

//------------------------------------------------------------------------
// Track::wrap
//------------------------------------------------------------------------
Track::Event Track::wrap(Track::SimpleEvent iEvent)
{
  if(iEvent)
    return [event = std::move(iEvent)](Motherboard &, TJBox_Int64, TJBox_Int64, TJBox_UInt16) {
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


}