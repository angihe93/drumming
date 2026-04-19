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

    addAndMakeVisible (seekBackButton);
    seekBackButton.onClick = [this] { seekTo (player.getPositionSeconds() - 10.0); };

    addAndMakeVisible (seekForwardButton);
    seekForwardButton.onClick = [this] { seekTo (player.getPositionSeconds() + 10.0); };

    addAndMakeVisible (seekSlider);
    seekSlider.setRange (0.0, 1.0, 0.0);
    seekSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    seekSlider.onDragStart = [this] { userDraggingSeek = true; };
    seekSlider.onDragEnd   = [this]
    {
        userDraggingSeek = false;
        seekTo (seekSlider.getValue());
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

    inputVolumeLabel.setText ("Input", juce::dontSendNotification);
    addAndMakeVisible (inputVolumeLabel);

    addAndMakeVisible (inputVolumeSlider);
    inputVolumeSlider.setRange (0.0, 1.5, 0.01);
    inputVolumeSlider.setValue (0.9, juce::dontSendNotification);
    inputVolumeSlider.setNumDecimalPlacesToDisplay (2);
    inputVolumeSlider.onValueChange = [this]
    {
        drumInput.setMasterGain ((float) inputVolumeSlider.getValue());
    };
    drumInput.setMasterGain (0.9f);

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

    midiInputLabel.setText (juce::String::fromUTF8 ("MIDI In  \xe2\x93\x98"),
                            juce::dontSendNotification);
    addAndMakeVisible (midiInputLabel);

    const juce::String keyboardHelp =
        "No drum pad? Click the notes area below to focus, then play with the keyboard:\n"
        "  Space = Kick\n"
        "  A = Closed hi-hat,   ; = Open hi-hat\n"
        "  S = Snare\n"
        "  D = Hi tom,   F = Mid tom,   J = Floor tom\n"
        "  K = Ride,   L = Crash\n"
        "Pick a MIDI device here to also play along with a drum pad.";
    midiInputLabel.setTooltip (keyboardHelp);
    midiInputCombo.setTooltip (keyboardHelp);

    addAndMakeVisible (midiInputCombo);
    midiInputCombo.onChange = [this]
    {
        const int id = midiInputCombo.getSelectedId();
        if (id <= 1)
        {
            setActiveMidiDevice ({});
        }
        else
        {
            const int index = id - 2;
            if (index >= 0 && index < midiDevices.size())
                setActiveMidiDevice (midiDevices.getReference (index).identifier);
        }
    };
    refreshMidiDeviceList();

    addAndMakeVisible (statusLabel);
    statusLabel.setText ("No file loaded. Keys: A S D F J K L ; Space", juce::dontSendNotification);

    addAndMakeVisible (positionLabel);
    positionLabel.setText ("0.00 / 0.00 s", juce::dontSendNotification);
    positionLabel.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (notesView);

    setWantsKeyboardFocus (true);

    setSize (960, 720);
    setAudioChannels (0, 2);
    startTimerHz (30);
}

MainComponent::~MainComponent()
{
    setActiveMidiDevice ({});

    if (keyListenerInstalled)
        removeKeyListener (this);

    shutdownAudio();
}

void MainComponent::parentHierarchyChanged()
{
    if (! keyListenerInstalled)
    {
        addKeyListener (this);
        keyListenerInstalled = true;
    }
    grabKeyboardFocus();
}

void MainComponent::mouseDown (const juce::MouseEvent&)
{
    grabKeyboardFocus();
}

void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sr)
{
    drumSynth.prepare (sr);
    pitchedSynth.prepare (sr, samplesPerBlockExpected);
    player.prepare (sr);
    drumInput.prepare (sr);
}

void MainComponent::releaseResources()
{
    drumSynth.reset();
    pitchedSynth.reset();
    drumInput.reset();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    info.clearActiveBufferRegion();

    juce::MidiBuffer pitchedEvents;
    player.processBlock (info.numSamples, drumSynth, pitchedEvents);

    drumSynth.render (*info.buffer, info.startSample, info.numSamples);
    pitchedSynth.processBlock (*info.buffer, pitchedEvents, info.startSample, info.numSamples);
    drumInput.processBlock (*info.buffer, info.startSample, info.numSamples);
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
    topRow.removeFromLeft (8);
    seekBackButton.setBounds (topRow.removeFromLeft (60));
    topRow.removeFromLeft (4);
    seekForwardButton.setBounds (topRow.removeFromLeft (60));
    topRow.removeFromLeft (12);
    positionLabel.setBounds (topRow.removeFromRight (160));
    statusLabel.setBounds (topRow);

    area.removeFromTop (6);
    seekSlider.setBounds (area.removeFromTop (22));
    area.removeFromTop (6);

    auto makeSliderRow = [&] (juce::Label& label, juce::Slider& slider)
    {
        auto row = area.removeFromTop (26);
        label.setBounds (row.removeFromLeft (80));
        slider.setBounds (row);
        area.removeFromTop (4);
    };

    makeSliderRow (tempoLabel,        tempoSlider);
    makeSliderRow (drumsVolumeLabel,  drumsVolumeSlider);
    makeSliderRow (melodyVolumeLabel, melodyVolumeSlider);
    makeSliderRow (inputVolumeLabel,  inputVolumeSlider);
    makeSliderRow (lookAheadLabel,    lookAheadSlider);

    {
        auto row = area.removeFromTop (26);
        midiInputLabel.setBounds (row.removeFromLeft (80));
        midiInputCombo.setBounds (row);
        area.removeFromTop (4);
    }

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
    seekSlider.setRange (0.0, juce::jmax (0.001, player.getLengthSeconds()), 0.0);
    seekSlider.setValue (0.0, juce::dontSendNotification);
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
    if (! userDraggingSeek)
        seekSlider.setValue (pos, juce::dontSendNotification);
    updateTransportUI();
}

void MainComponent::seekTo (double timeSec)
{
    player.seek (timeSec);
    drumSynth.reset();
    pitchedSynth.reset();
    seekSlider.setValue (player.getPositionSeconds(), juce::dontSendNotification);
}

void MainComponent::refreshMidiDeviceList()
{
    midiDevices = juce::MidiInput::getAvailableDevices();

    midiInputCombo.clear (juce::dontSendNotification);
    midiInputCombo.addItem ("(none)", 1);

    for (int i = 0; i < midiDevices.size(); ++i)
        midiInputCombo.addItem (midiDevices.getReference (i).name, i + 2);

    if (activeMidiDeviceId.isEmpty())
    {
        midiInputCombo.setSelectedId (1, juce::dontSendNotification);
    }
    else
    {
        int found = 1;
        for (int i = 0; i < midiDevices.size(); ++i)
            if (midiDevices.getReference (i).identifier == activeMidiDeviceId)
                found = i + 2;
        midiInputCombo.setSelectedId (found, juce::dontSendNotification);
    }
}

void MainComponent::setActiveMidiDevice (const juce::String& deviceId)
{
    if (activeMidiDeviceId == deviceId) return;

    if (activeMidiDeviceId.isNotEmpty())
    {
        deviceManager.removeMidiInputDeviceCallback (activeMidiDeviceId, &drumInput);
        deviceManager.setMidiInputDeviceEnabled (activeMidiDeviceId, false);
    }

    activeMidiDeviceId = deviceId;

    if (activeMidiDeviceId.isNotEmpty())
    {
        deviceManager.setMidiInputDeviceEnabled (activeMidiDeviceId, true);
        deviceManager.addMidiInputDeviceCallback (activeMidiDeviceId, &drumInput);
    }
}

int MainComponent::keyCodeToDrumNote (int keyCode)
{
    if (keyCode == juce::KeyPress::spaceKey) return 36;  // Kick
    switch (keyCode)
    {
        case 'A': return 42;  // Closed hi-hat
        case 'S': return 38;  // Snare
        case 'D': return 48;  // Hi tom
        case 'F': return 45;  // Mid/low tom
        case 'J': return 43;  // Floor tom
        case 'K': return 51;  // Ride
        case 'L': return 49;  // Crash
        case ';': return 46;  // Open hi-hat
        default: return -1;
    }
}

bool MainComponent::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    const int code = key.getKeyCode();
    const int note = keyCodeToDrumNote (code);
    if (note < 0) return false;

    if (keysDown.insert (code).second)
        drumInput.postNoteOn (note, 0.9f);

    return true;
}

bool MainComponent::keyStateChanged (bool, juce::Component*)
{
    for (auto it = keysDown.begin(); it != keysDown.end(); )
    {
        if (! juce::KeyPress::isKeyCurrentlyDown (*it))
            it = keysDown.erase (it);
        else
            ++it;
    }
    return false;
}
