#include "DrumInput.h"

void DrumInput::prepare (double sampleRate)
{
    collector.reset (sampleRate);
    synth.prepare (sampleRate);
}

void DrumInput::reset()
{
    synth.reset();
}

void DrumInput::processBlock (juce::AudioBuffer<float>& output,
                              int startSample, int numSamples)
{
    juce::MidiBuffer buf;
    collector.removeNextBlockOfMessages (buf, numSamples);

    for (const auto meta : buf)
    {
        const auto& msg = meta.getMessage();
        if (msg.isNoteOn())
            synth.trigger (msg.getNoteNumber(),
                           msg.getFloatVelocity(),
                           meta.samplePosition);
    }

    synth.render (output, startSample, numSamples);
}

void DrumInput::postNoteOn (int midiNote, float velocity)
{
    auto msg = juce::MidiMessage::noteOn (10, midiNote, velocity);
    msg.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    collector.addMessageToQueue (msg);
}

void DrumInput::handleIncomingMidiMessage (juce::MidiInput*,
                                           const juce::MidiMessage& msg)
{
    if (! msg.isNoteOn()) return;

    auto copy = msg;
    copy.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    collector.addMessageToQueue (copy);
}
