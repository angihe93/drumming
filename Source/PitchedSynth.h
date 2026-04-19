#pragma once

#include <JuceHeader.h>
#include <atomic>

// Simple polyphonic synth for non-drum MIDI channels. A "guide track" voice:
// sine + a couple of harmonics, ADSR envelope. Good enough to follow along;
// not meant to sound like real instruments.
class PitchedSynth
{
public:
    PitchedSynth();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Audio thread. Additively mixes pitched output into `output`, scaled by gain.
    void processBlock (juce::AudioBuffer<float>& output,
                       juce::MidiBuffer&         midi,
                       int                       startSample,
                       int                       numSamples);

    void setMasterGain (float g) { masterGain.store (juce::jlimit (0.0f, 2.0f, g)); }

private:
    juce::Synthesiser        synth;
    juce::AudioBuffer<float> tempBuffer;
    std::atomic<float>       masterGain { 0.5f };
};
