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

    addAndMakeVisible (statusLabel);
    statusLabel.setText ("No file loaded.", juce::dontSendNotification);

    addAndMakeVisible (positionLabel);
    positionLabel.setText ("0.00 / 0.00 s", juce::dontSendNotification);
    positionLabel.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (eventLog);
    eventLog.setMultiLine (true);
    eventLog.setReadOnly (true);
    eventLog.setScrollbarsShown (true);
    eventLog.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));

    setSize (960, 620);
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

    drumSynth.render    (*info.buffer, info.startSample, info.numSamples);
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

    area.removeFromTop (4);
    eventLog.setBounds (area);
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

    populateEventLog (midi);
    updateTransportUI();

    statusLabel.setText (
        juce::String::formatted ("Loaded: %s  —  %d drum note-ons, %d pitched note-ons",
                                 file.getFileName().toRawUTF8(),
                                 player.getNumDrumEvents(),
                                 player.getNumPitchedEvents()),
        juce::dontSendNotification);
}

void MainComponent::populateEventLog (const juce::MidiFile& sourceFile)
{
    juce::MidiFile midi (sourceFile);
    midi.convertTimestampTicksToSeconds();

    juce::StringArray lines;

    for (int t = 0; t < midi.getNumTracks(); ++t)
    {
        const auto* track = midi.getTrack (t);
        if (track == nullptr) continue;

        for (int i = 0; i < track->getNumEvents() && lines.size() < 200; ++i)
        {
            const auto& msg = track->getEventPointer (i)->message;
            if (! msg.isNoteOn()) continue;
            if (msg.getChannel() != 10) continue;

            lines.add (juce::String::formatted (
                "t=%7.3fs  track=%d  note=%3d (%s)  vel=%3d",
                msg.getTimeStamp(),
                t,
                msg.getNoteNumber(),
                describeDrumNote (msg.getNoteNumber()).toRawUTF8(),
                msg.getVelocity()));
        }
    }

    if (lines.isEmpty())
        eventLog.setText ("No drum (channel 10) note-on events found.");
    else
        eventLog.setText (lines.joinIntoString ("\n"));
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

juce::String MainComponent::describeDrumNote (int n)
{
    switch (n)
    {
        case 35: return "Acoustic Bass Drum";
        case 36: return "Kick";
        case 37: return "Side Stick";
        case 38: return "Snare";
        case 39: return "Hand Clap";
        case 40: return "Electric Snare";
        case 41: return "Low Floor Tom";
        case 42: return "Hi-Hat Closed";
        case 43: return "High Floor Tom";
        case 44: return "Hi-Hat Pedal";
        case 45: return "Low Tom";
        case 46: return "Hi-Hat Open";
        case 47: return "Low-Mid Tom";
        case 48: return "Hi-Mid Tom";
        case 49: return "Crash 1";
        case 50: return "High Tom";
        case 51: return "Ride 1";
        case 52: return "China";
        case 53: return "Ride Bell";
        case 55: return "Splash";
        case 57: return "Crash 2";
        case 59: return "Ride 2";
        default: return "?";
    }
}
