#include "MainComponent.h"

MainComponent::MainComponent()
{
    addAndMakeVisible (loadButton);
    loadButton.onClick = [this] { chooseAndLoadMidi(); };

    addAndMakeVisible (statusLabel);
    statusLabel.setText ("No file loaded.", juce::dontSendNotification);
    statusLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (eventLog);
    eventLog.setMultiLine (true);
    eventLog.setReadOnly (true);
    eventLog.setScrollbarsShown (true);
    eventLog.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain));

    setSize (900, 600);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (12);
    auto top  = area.removeFromTop (32);
    loadButton.setBounds (top.removeFromLeft (180));
    top.removeFromLeft (12);
    statusLabel.setBounds (top);
    area.removeFromTop (8);
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

    // Convert tick-based timestamps to seconds using the file's tempo map.
    midi.convertTimestampTicksToSeconds();

    juce::StringArray lines;
    int drumEventCount = 0;

    for (int t = 0; t < midi.getNumTracks(); ++t)
    {
        const auto* track = midi.getTrack (t);
        if (track == nullptr) continue;

        for (int i = 0; i < track->getNumEvents(); ++i)
        {
            const auto& msg = track->getEventPointer (i)->message;
            if (! msg.isNoteOn()) continue;

            // General MIDI puts drums on channel 10 (1-based) = channel 10 in JUCE's API.
            if (msg.getChannel() != 10) continue;

            ++drumEventCount;
            if (lines.size() < 200)
            {
                lines.add (juce::String::formatted (
                    "t=%7.3fs  track=%d  note=%3d (%s)  vel=%3d",
                    msg.getTimeStamp(),
                    t,
                    msg.getNoteNumber(),
                    describeDrumNote (msg.getNoteNumber()).toRawUTF8(),
                    msg.getVelocity()));
            }
        }
    }

    statusLabel.setText (
        juce::String::formatted ("Loaded: %s  —  %d drum note-on events across %d tracks",
                                 file.getFileName().toRawUTF8(),
                                 drumEventCount,
                                 midi.getNumTracks()),
        juce::dontSendNotification);

    if (lines.isEmpty())
        eventLog.setText ("No drum (channel 10) note-on events found.\n"
                          "(File may use a different channel — we'll handle that in the next phase.)");
    else
        eventLog.setText (lines.joinIntoString ("\n"));
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
