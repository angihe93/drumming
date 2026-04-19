#include "MainComponent.h"

MainComponent::MainComponent()
{
    addAndMakeVisible (loadButton);
    loadButton.onClick = [this] { chooseAndLoadMidi(); };

    addAndMakeVisible (playButton);
    playButton.onClick = [this]
    {
        if (player.getIsPlaying())
        {
            player.pause();
            pitchedSynth.reset();
        }
        else
        {
            player.play();
        }
        updateTransportUI();
    };

    addAndMakeVisible (stopButton);
    stopButton.onClick = [this]
    {
        player.stop();
        drumSynth.reset();
        pitchedSynth.reset();
        updateTransportUI();
    };

    tempoLabel.setText ("Tempo", juce::dontSendNotification);
    addAndMakeVisible (tempoLabel);

    addAndMakeVisible (tempoSlider);
    tempoSlider.setRange (0.25, 2.0, 0.01);
    tempoSlider.setValue (1.0, juce::dontSendNotification);
    tempoSlider.setTextValueSuffix ("x");
    tempoSlider.setNumDecimalPlacesToDisplay (2);
    tempoSlider.onValueChange = [this] { player.setTempoMultiplier (tempoSlider.getValue()); };

    drumsVolumeLabel.setText ("Drums", juce::dontSendNotification);
    addAndMakeVisible (drumsVolumeLabel);

    addAndMakeVisible (drumsVolumeSlider);
    drumsVolumeSlider.setRange (0.0, 1.5, 0.01);
    drumsVolumeSlider.setValue (0.8, juce::dontSendNotification);
    drumsVolumeSlider.setNumDecimalPlacesToDisplay (2);
    drumsVolumeSlider.onValueChange = [this]
    {
        drumSynth.setMasterGain ((float) drumsVolumeSlider.getValue());
    };

    melodyVolumeLabel.setText ("Melody", juce::dontSendNotification);
    addAndMakeVisible (melodyVolumeLabel);

    addAndMakeVisible (melodyVolumeSlider);
    melodyVolumeSlider.setRange (0.0, 1.5, 0.01);
    melodyVolumeSlider.setValue (0.5, juce::dontSendNotification);
    melodyVolumeSlider.setNumDecimalPlacesToDisplay (2);
    melodyVolumeSlider.onValueChange = [this]
    {
        pitchedSynth.setMasterGain ((float) melodyVolumeSlider.getValue());
    };

    lookAheadLabel.setText ("Lookahead", juce::dontSendNotification);
    addAndMakeVisible (lookAheadLabel);

    addAndMakeVisible (lookAheadSlider);
    lookAheadSlider.setRange (0.75, 5.0, 0.05);
    lookAheadSlider.setValue (2.5, juce::dontSendNotification);
    lookAheadSlider.setTextValueSuffix (" s");
    lookAheadSlider.setNumDecimalPlacesToDisplay (2);
    lookAheadSlider.onValueChange = [this]
    {
        notesView.setLookAheadSeconds (lookAheadSlider.getValue());
    };

    addAndMakeVisible (statusLabel);
    statusLabel.setText ("No file loaded.", juce::dontSendNotification);

    addAndMakeVisible (positionLabel);
    positionLabel.setText ("0.00 / 0.00 s", juce::dontSendNotification);
    positionLabel.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (notesView);

    setSize (960, 640);
    setAudioChannels (0, 2);
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sr)
{
    drumSynth.prepare (sr);
    pitchedSynth.prepare (sr, samplesPerBlockExpected);
    player.prepare (sr);
}

void MainComponent::releaseResources()
{
    drumSynth.reset();
    pitchedSynth.reset();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();

    juce::MidiBuffer pitchedEvents;
    player.processBlock (info.numSamples, drumSynth, pitchedEvents);

    drumSynth.render (*info.buffer, info.startSample, info.numSamples);
    pitchedSynth.processBlock (*info.buffer, pitchedEvents, info.startSample, info.numSamples);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (12);

    auto topRow = area.removeFromTop (32);
    loadButton.setBounds (topRow.removeFromLeft (160));
    topRow.removeFromLeft (8);
    playButton.setBounds (topRow.removeFromLeft (80));
    topRow.removeFromLeft (4);
    stopButton.setBounds (topRow.removeFromLeft (80));
    topRow.removeFromLeft (12);
    positionLabel.setBounds (topRow.removeFromRight (160));
    statusLabel.setBounds (topRow);

    area.removeFromTop (8);

    auto makeSliderRow = [&] (juce::Label& label, juce::Slider& slider)
    {
        auto row = area.removeFromTop (26);
        label.setBounds (row.removeFromLeft (70));
        slider.setBounds (row);
        area.removeFromTop (4);
    };

    makeSliderRow (tempoLabel,        tempoSlider);
    makeSliderRow (drumsVolumeLabel,  drumsVolumeSlider);
    makeSliderRow (melodyVolumeLabel, melodyVolumeSlider);
    makeSliderRow (lookAheadLabel,    lookAheadSlider);

    area.removeFromTop (6);
    notesView.setBounds (area);
}

void MainComponent::chooseAndLoadMidi()
{
    chooser = std::make_unique<juce::FileChooser> (
        "Select a MIDI file",
        juce::File::getSpecialLocation (juce::File::userHomeDirectory),
        "*.mid;*.midi");

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file.existsAsFile())
            loadMidiFile (file);
    });
}

void MainComponent::loadMidiFile (const juce::File& file)
{
    juce::MidiFile midi;
    {
        juce::FileInputStream stream (file);
        if (! stream.openedOk() || ! midi.readFrom (stream))
        {
            statusLabel.setText ("Failed to read: " + file.getFileName(),
                                 juce::dontSendNotification);
            return;
        }
    }

    player.stop();
    drumSynth.reset();
    pitchedSynth.reset();
    player.loadMidi (midi);
    updateTransportUI();

    statusLabel.setText (
        juce::String::formatted ("Loaded: %s  —  %d drum note-ons, %d pitched note-ons",
                                 file.getFileName().toRawUTF8(),
                                 player.getNumDrumEvents(),
                                 player.getNumPitchedEvents()),
        juce::dontSendNotification);
}

void MainComponent::updateTransportUI()
{
    playButton.setButtonText (player.getIsPlaying() ? "Pause" : "Play");
}

void MainComponent::timerCallback()
{
    const double pos = player.getPositionSeconds();
    const double len = player.getLengthSeconds();
    positionLabel.setText (juce::String::formatted ("%.2f / %.2f s", pos, len),
                           juce::dontSendNotification);
    updateTransportUI();
}
