#pragma once

#include <JuceHeader.h>

#include <unordered_set>

#include "DrumInput.h"
#include "DrumNotesView.h"
#include "DrumSynth.h"
#include "MidiPlayer.h"
#include "PitchedSynth.h"

class MainComponent : public juce::AudioAppComponent,
                      private juce::Timer,
                      private juce::KeyListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;
    void mouseDown (const juce::MouseEvent&) override;

    void prepareToPlay   (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

private:
    void chooseAndLoadMidi();
    void loadMidiFile (const juce::File& file);
    void updateTransportUI();
    void timerCallback() override;
    void seekTo (double timeSec);

    void refreshMidiDeviceList();
    void setActiveMidiDevice (const juce::String& deviceId);
    static int keyCodeToDrumNote (int keyCode);

    using juce::Component::keyPressed;
    using juce::Component::keyStateChanged;
    bool keyPressed       (const juce::KeyPress&, juce::Component*) override;
    bool keyStateChanged  (bool isKeyDown, juce::Component*) override;

    bool userDraggingSeek { false };
    bool keyListenerInstalled { false };

    juce::TextButton loadButton       { "Load MIDI file..." };
    juce::TextButton playButton       { "Play" };
    juce::TextButton stopButton       { "Stop" };
    juce::TextButton seekBackButton   { "-10s" };
    juce::TextButton seekForwardButton{ "+10s" };
    juce::Slider     seekSlider;
    juce::Slider     tempoSlider;
    juce::Label      tempoLabel;
    juce::Slider     drumsVolumeSlider;
    juce::Label      drumsVolumeLabel;
    juce::Slider     melodyVolumeSlider;
    juce::Label      melodyVolumeLabel;
    juce::Slider     inputVolumeSlider;
    juce::Label      inputVolumeLabel;
    juce::Slider     lookAheadSlider;
    juce::Label      lookAheadLabel;
    juce::ComboBox   midiInputCombo;
    juce::Label      midiInputLabel;
    juce::Label      statusLabel;
    juce::Label      positionLabel;

    std::unique_ptr<juce::FileChooser> chooser;

    DrumSynth     drumSynth;
    PitchedSynth  pitchedSynth;
    MidiPlayer    player;
    DrumInput     drumInput;
    DrumNotesView notesView { player };

    juce::Array<juce::MidiDeviceInfo> midiDevices;
    juce::String                      activeMidiDeviceId;
    std::unordered_set<int>           keysDown;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
