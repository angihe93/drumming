#pragma once

#include <JuceHeader.h>

#include "DrumNotesView.h"
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
    void updateTransportUI();
    void timerCallback() override;

    juce::TextButton loadButton       { "Load MIDI file..." };
    juce::TextButton playButton       { "Play" };
    juce::TextButton stopButton       { "Stop" };
    juce::Slider     tempoSlider;
    juce::Label      tempoLabel;
    juce::Slider     drumsVolumeSlider;
    juce::Label      drumsVolumeLabel;
    juce::Slider     melodyVolumeSlider;
    juce::Label      melodyVolumeLabel;
    juce::Slider     lookAheadSlider;
    juce::Label      lookAheadLabel;
    juce::Label      statusLabel;
    juce::Label      positionLabel;

    std::unique_ptr<juce::FileChooser> chooser;

    DrumSynth     drumSynth;
    PitchedSynth  pitchedSynth;
    MidiPlayer    player;
    DrumNotesView notesView { player };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
