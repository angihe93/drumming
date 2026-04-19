#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>

#include "DrumSynth.h"

class MidiPlayer
{
public:
    void prepare (double sampleRate);
    void clear();

    // Message thread. Replaces the event list. Caller should stop first; we also
    // guard the swap against the audio thread with a spinlock.
    void loadMidi (const juce::MidiFile& file);

    void play();
    void pause();
    void stop();

    bool   getIsPlaying()        const { return playing.load(); }
    double getPositionSeconds()  const { return displayPosition.load(); }
    double getLengthSeconds()    const { return lengthSeconds.load(); }
    double getTempoMultiplier()  const { return tempoMultiplier.load(); }
    int    getNumDrumEvents()    const { return numDrumEvents.load(); }
    int    getNumPitchedEvents() const { return numPitchedEvents.load(); }

    void setTempoMultiplier (double m);

    // Jumps the playhead to `timeSec` (clamped to [0, length]). Does not change
    // play state. Callers should also reset the synths to silence any held notes.
    void seek (double timeSec);

    // Audio thread. Drum note-ons fire directly on `drums`; everything else for
    // channels 1-9, 11-16 is added to `pitchedOut` at its sample-accurate offset.
    void processBlock (int numSamples, DrumSynth& drums, juce::MidiBuffer& pitchedOut);

    // UI thread. Fills `out` with drum events whose timeSec falls in [t0, t1).
    // `out` is cleared first, then appended to.
    struct DrumEvent { double timeSec; int note; float velocity; };
    void getDrumEventsInRange (double t0, double t1, std::vector<DrumEvent>& out) const;

private:
    struct Event
    {
        double            timeSec;
        juce::MidiMessage message;
    };

    double sampleRate { 44100.0 };

    std::atomic<bool>   playing          { false };
    std::atomic<double> tempoMultiplier  { 1.0 };
    std::atomic<double> displayPosition  { 0.0 };
    std::atomic<double> lengthSeconds    { 0.0 };
    std::atomic<int>    numDrumEvents    { 0 };
    std::atomic<int>    numPitchedEvents { 0 };

    // Audio-thread only.
    double      playheadSeconds { 0.0 };
    std::size_t nextEventIndex  { 0 };

    mutable juce::SpinLock loadLock;
    std::vector<Event>     events;
    std::vector<DrumEvent> drumEvents; // sorted by timeSec, drum channel note-ons only
};
