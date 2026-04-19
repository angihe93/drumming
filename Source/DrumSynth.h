#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class DrumSynth
{
public:
    static constexpr int kMaxVoices = 48;

    void prepare (double sampleRate);
    void reset();

    // Called from audio thread (or message thread when stopped).
    void trigger (int noteNumber, float velocity, int sampleOffsetInBlock);
    void render  (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    void setMasterGain (float g) { masterGain.store (juce::jlimit (0.0f, 2.0f, g)); }

private:
    struct Voice
    {
        bool  active             = false;
        int   noteNumber         = 0;
        float velocity           = 1.0f;
        int   ageSamples         = 0;
        int   lifespanSamples    = 0;
        int   startOffsetInBlock = 0;
        float noiseLpState       = 0.0f;
    };

    int   lifespanForNote (int note) const;
    float renderSample    (Voice& v, float noise);

    std::array<Voice, kMaxVoices> voices {};
    double             sampleRate { 44100.0 };
    juce::Random       rng        { 20260418 };
    std::atomic<float> masterGain { 0.8f };
};
