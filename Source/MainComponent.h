#pragma once

#include <JuceHeader.h>

#include "DrumSynth.h"
#include "MidiPlayer.h"
#include "PitchedSynth.h"

class MainComponent : public juce::AudioAppComponent,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void prepareToPlay   (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

private:
    void chooseAndLoadMidi();
    void loadMidiFile (const juce::File& file);
    void populateEventLog (const juce::MidiFile& file);
    void updateTransportUI();
    void timerCallback() override;

    static juce::String describeDrumNote (int noteNumber);

    juce::TextButton loadButton       { "Load MIDI file..." };
    juce::TextButton playButton       { "Play" };
    juce::TextButton stopButton       { "Stop" };
    juce::Slider     tempoSlider;
    juce::Label      tempoLabel;
    juce::Slider     drumsVolumeSlider;
    juce::Label      drumsVolumeLabel;
    juce::Slider     melodyVolumeSlider;
    juce::Label      melodyVolumeLabel;
    juce::Label      statusLabel;
    juce::Label      positionLabel;
    juce::TextEditor eventLog;

    std::unique_ptr<juce::FileChooser> chooser;

    DrumSynth    drumSynth;
    PitchedSynth pitchedSynth;
    MidiPlayer   player;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
