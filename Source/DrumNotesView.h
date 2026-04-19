#pragma once

#include <JuceHeader.h>

#include "MidiPlayer.h"

// Vertical Guitar-Hero-style lanes. Notes stream down from the top and line up
// with the hit line at the bottom exactly when the MidiPlayer fires them.
class DrumNotesView : public juce::Component,
                      private juce::Timer
{
public:
    explicit DrumNotesView (MidiPlayer& player);
    ~DrumNotesView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setLookAheadSeconds (double seconds);

    static constexpr int kNumLanes = 8;

    // Public so MainComponent (or tests) could reuse the mapping. Returns -1 for
    // notes we don't visualise (e.g. cowbell, hand clap).
    static int          noteToLane  (int noteNumber);
    static const char*  laneName    (int lane);
    static juce::Colour laneColour  (int lane);

private:
    void timerCallback() override;

    MidiPlayer& player;
    double      lookAheadSeconds { 2.5 };

    // Reused per frame to avoid reallocation on the paint path.
    std::vector<MidiPlayer::DrumEvent> frameEvents;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumNotesView)
};
