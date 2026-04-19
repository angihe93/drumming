#include "MidiPlayer.h"

#include <algorithm>

void MidiPlayer::prepare (double sr)
{
    sampleRate = sr;
}

void MidiPlayer::clear()
{
    playing.store (false);
    const juce::SpinLock::ScopedLockType sl (loadLock);
    events.clear();
    drumEvents.clear();
    playheadSeconds = 0.0;
    nextEventIndex  = 0;
    lengthSeconds.store (0.0);
    displayPosition.store (0.0);
    numDrumEvents.store (0);
    numPitchedEvents.store (0);
}

void MidiPlayer::loadMidi (const juce::MidiFile& sourceFile)
{
    juce::MidiFile m (sourceFile);
    m.convertTimestampTicksToSeconds();

    std::vector<Event>     newEvents;
    std::vector<DrumEvent> newDrumEvents;
    double maxT         = 0.0;
    int    pitchedCount = 0;

    for (int t = 0; t < m.getNumTracks(); ++t)
    {
        const auto* track = m.getTrack (t);
        if (track == nullptr) continue;

        for (int i = 0; i < track->getNumEvents(); ++i)
        {
            const auto& msg = track->getEventPointer (i)->message;
            if (msg.getChannel() <= 0) continue;  // skip meta/sysex

            const double ts = msg.getTimeStamp();
            newEvents.push_back ({ ts, msg });
            maxT = std::max (maxT, ts);

            if (msg.isNoteOn())
            {
                if (msg.getChannel() == 10)
                    newDrumEvents.push_back ({ ts, msg.getNoteNumber(), msg.getFloatVelocity() });
                else
                    ++pitchedCount;
            }
        }
    }

    std::sort (newEvents.begin(), newEvents.end(),
               [] (const Event& a, const Event& b) { return a.timeSec < b.timeSec; });
    std::sort (newDrumEvents.begin(), newDrumEvents.end(),
               [] (const DrumEvent& a, const DrumEvent& b) { return a.timeSec < b.timeSec; });

    playing.store (false);
    {
        const juce::SpinLock::ScopedLockType sl (loadLock);
        events          = std::move (newEvents);
        drumEvents      = std::move (newDrumEvents);
        playheadSeconds = 0.0;
        nextEventIndex  = 0;
    }

    lengthSeconds.store (maxT + 2.0);
    displayPosition.store (0.0);
    numDrumEvents.store ((int) drumEvents.size());
    numPitchedEvents.store (pitchedCount);
}

void MidiPlayer::getDrumEventsInRange (double t0, double t1,
                                       std::vector<DrumEvent>& out) const
{
    out.clear();
    if (t1 <= t0) return;

    const juce::SpinLock::ScopedLockType sl (loadLock);
    if (drumEvents.empty()) return;

    const auto lo = std::lower_bound (drumEvents.begin(), drumEvents.end(), t0,
                                      [] (const DrumEvent& e, double t) { return e.timeSec < t; });
    const auto hi = std::lower_bound (lo, drumEvents.end(), t1,
                                      [] (const DrumEvent& e, double t) { return e.timeSec < t; });

    out.insert (out.end(), lo, hi);
}

void MidiPlayer::play()
{
    if (events.empty()) return;

    const juce::SpinLock::ScopedLockType sl (loadLock);
    if (playheadSeconds >= lengthSeconds.load())
    {
        playheadSeconds = 0.0;
        nextEventIndex  = 0;
    }
    playing.store (true);
}

void MidiPlayer::pause()
{
    playing.store (false);
}

void MidiPlayer::stop()
{
    playing.store (false);
    const juce::SpinLock::ScopedLockType sl (loadLock);
    playheadSeconds = 0.0;
    nextEventIndex  = 0;
    displayPosition.store (0.0);
}

void MidiPlayer::setTempoMultiplier (double m)
{
    tempoMultiplier.store (juce::jlimit (0.1, 4.0, m));
}

void MidiPlayer::processBlock (int numSamples, DrumSynth& drums, juce::MidiBuffer& pitchedOut)
{
    if (! playing.load()) return;

    const juce::SpinLock::ScopedTryLockType tl (loadLock);
    if (! tl.isLocked()) return;

    const double tempo    = tempoMultiplier.load();
    const double blockSec = numSamples / sampleRate * tempo;
    const double startT   = playheadSeconds;
    const double endT     = startT + blockSec;

    while (nextEventIndex < events.size() && events[nextEventIndex].timeSec < endT)
    {
        const auto& e = events[nextEventIndex];
        if (e.timeSec >= startT)
        {
            const double deltaSec = e.timeSec - startT;
            int sampleOffset = (int) (deltaSec / tempo * sampleRate);
            sampleOffset = juce::jlimit (0, numSamples - 1, sampleOffset);

            if (e.message.getChannel() == 10)
            {
                if (e.message.isNoteOn())
                    drums.trigger (e.message.getNoteNumber(),
                                   e.message.getFloatVelocity(),
                                   sampleOffset);
            }
            else
            {
                pitchedOut.addEvent (e.message, sampleOffset);
            }
        }
        ++nextEventIndex;
    }

    playheadSeconds = endT;
    displayPosition.store (playheadSeconds);

    if (playheadSeconds >= lengthSeconds.load())
    {
        playing.store (false);
        playheadSeconds = 0.0;
        nextEventIndex  = 0;
    }
}
