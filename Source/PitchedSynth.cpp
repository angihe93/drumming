#include "PitchedSynth.h"

namespace
{
    class GuideSound : public juce::SynthesiserSound
    {
    public:
        bool appliesToNote    (int) override { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    class GuideVoice : public juce::SynthesiserVoice
    {
    public:
        GuideVoice()
        {
            adsr.setParameters ({ 0.01f, 0.15f, 0.7f, 0.25f });
        }

        bool canPlaySound (juce::SynthesiserSound* s) override
        {
            return dynamic_cast<GuideSound*> (s) != nullptr;
        }

        void setCurrentPlaybackSampleRate (double sr) override
        {
            SynthesiserVoice::setCurrentPlaybackSampleRate (sr);
            adsr.setSampleRate (sr);
        }

        void startNote (int midiNote, float vel, juce::SynthesiserSound*, int) override
        {
            freq     = juce::MidiMessage::getMidiNoteInHertz (midiNote);
            velocity = vel;
            phase    = 0.0;
            adsr.noteOn();
        }

        void stopNote (float, bool allowTailOff) override
        {
            if (allowTailOff)
            {
                adsr.noteOff();
            }
            else
            {
                adsr.reset();
                clearCurrentNote();
            }
        }

        void pitchWheelMoved (int) override {}
        void controllerMoved (int, int) override {}

        void renderNextBlock (juce::AudioBuffer<float>& buffer,
                              int startSample, int numSamples) override
        {
            if (! adsr.isActive()) return;

            const double sr = getSampleRate();
            if (sr <= 0.0) return;

            const double inc   = freq / sr;
            const double twoPi = juce::MathConstants<double>::twoPi;

            for (int i = 0; i < numSamples; ++i)
            {
                const float env = adsr.getNextSample();

                float s = (float) (std::sin (phase * twoPi)
                                 + 0.25 * std::sin (phase * 2.0 * twoPi)
                                 + 0.10 * std::sin (phase * 3.0 * twoPi));
                s *= 0.25f * velocity * env;

                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    buffer.getWritePointer (ch)[startSample + i] += s;

                phase += inc;
                if (phase >= 1.0) phase -= 1.0;

                if (! adsr.isActive())
                {
                    clearCurrentNote();
                    break;
                }
            }
        }

    private:
        juce::ADSR adsr;
        double     freq     { 0.0 };
        double     phase    { 0.0 };
        float      velocity { 1.0f };
    };
}

PitchedSynth::PitchedSynth()
{
    for (int i = 0; i < 16; ++i)
        synth.addVoice (new GuideVoice());
    synth.addSound (new GuideSound());
}

void PitchedSynth::prepare (double sr, int maxBlockSize)
{
    synth.setCurrentPlaybackSampleRate (sr);
    tempBuffer.setSize (2, juce::jmax (maxBlockSize, 256), false, true, false);
}

void PitchedSynth::reset()
{
    synth.allNotesOff (0, false);
}

void PitchedSynth::processBlock (juce::AudioBuffer<float>& output,
                                 juce::MidiBuffer&         midi,
                                 int                       startSample,
                                 int                       numSamples)
{
    if (tempBuffer.getNumSamples() < numSamples)
        tempBuffer.setSize (2, numSamples, false, true, false);

    tempBuffer.clear (0, numSamples);
    synth.renderNextBlock (tempBuffer, midi, 0, numSamples);

    const float gain       = masterGain.load();
    const int   numChannels = juce::jmin (output.getNumChannels(), tempBuffer.getNumChannels());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* src = tempBuffer.getReadPointer (ch);
        float*       dst = output.getWritePointer (ch) + startSample;
        for (int i = 0; i < numSamples; ++i)
            dst[i] += src[i] * gain;
    }
}
