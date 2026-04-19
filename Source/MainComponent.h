#pragma once

#include <JuceHeader.h>

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void chooseAndLoadMidi();
    void loadMidiFile (const juce::File& file);
    static juce::String describeDrumNote (int noteNumber);

    juce::TextButton loadButton { "Load MIDI file..." };
    juce::Label statusLabel;
    juce::TextEditor eventLog;

    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
