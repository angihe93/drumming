#include "DrumNotesView.h"

DrumNotesView::DrumNotesView (MidiPlayer& p)
    : player (p)
{
    setOpaque (true);
    frameEvents.reserve (64);
    startTimerHz (60);
}

DrumNotesView::~DrumNotesView()
{
    stopTimer();
}

void DrumNotesView::setLookAheadSeconds (double s)
{
    lookAheadSeconds = juce::jlimit (0.5, 8.0, s);
}

void DrumNotesView::resized() {}

void DrumNotesView::timerCallback()
{
    repaint();
}

// Ordered to match standard drum-tab layout: kick sits between the mounted toms.
int DrumNotesView::noteToLane (int n)
{
    switch (n)
    {
        case 42: case 44: case 46:          return 0; // Hi-Hat (closed / pedal / open)
        case 49: case 52: case 55: case 57: return 1; // Crash / China / Splash
        case 37: case 38: case 39: case 40: return 2; // Snare family + side-stick / clap
        case 50:                            return 3; // High Tom
        case 35: case 36:                   return 4; // Kick
        case 47: case 48:                   return 5; // Mid Tom
        case 41: case 43: case 45:          return 6; // Floor / Low Tom
        case 51: case 53: case 59: case 56: return 7; // Ride / Ride Bell / Cowbell
        default:                            return -1;
    }
}

const char* DrumNotesView::laneName (int lane)
{
    switch (lane)
    {
        case 0: return "Hi-Hat";
        case 1: return "Crash";
        case 2: return "Snare";
        case 3: return "Tom 1";
        case 4: return "Kick";
        case 5: return "Tom 2";
        case 6: return "Floor";
        case 7: return "Ride";
        default: return "";
    }
}

juce::Colour DrumNotesView::laneColour (int lane)
{
    switch (lane)
    {
        case 0: return juce::Colour (0xffffd84d); // Hi-Hat   — yellow
        case 1: return juce::Colour (0xffff6b6b); // Crash    — coral
        case 2: return juce::Colour (0xfff06292); // Snare    — pink
        case 3: return juce::Colour (0xff66bb6a); // Tom 1    — green
        case 4: return juce::Colour (0xffffa726); // Kick     — orange
        case 5: return juce::Colour (0xff26c6da); // Tom 2    — cyan
        case 6: return juce::Colour (0xff42a5f5); // Floor    — blue
        case 7: return juce::Colour (0xffba68c8); // Ride     — purple
        default: return juce::Colours::white;
    }
}

void DrumNotesView::paint (juce::Graphics& g)
{
    const float width  = (float) getWidth();
    const float height = (float) getHeight();

    g.fillAll (juce::Colour (0xff14141a));

    const float labelH    = 26.0f;
    const float hitH      = 4.0f;
    const float laneTop   = labelH;
    const float hitLineY  = height - labelH - hitH;
    const float laneH     = hitLineY - laneTop;
    const float laneWidth = width / (float) kNumLanes;

    // Lane backgrounds + separators
    for (int i = 0; i < kNumLanes; ++i)
    {
        const float x = i * laneWidth;
        g.setColour ((i % 2 == 0) ? juce::Colour (0xff1c1c26)
                                  : juce::Colour (0xff181820));
        g.fillRect (x, laneTop, laneWidth, laneH);

        if (i > 0)
        {
            g.setColour (juce::Colour (0xff2a2a38));
            g.drawLine (x, laneTop, x, hitLineY, 1.0f);
        }
    }

    // Top strip — direction hint
    g.setColour (juce::Colours::darkgrey);
    g.setFont (juce::Font (12.0f));
    g.drawText ("notes fall   v",
                juce::Rectangle<float> (0, 0, width, labelH),
                juce::Justification::centred);

    // Falling notes
    const double now    = player.getPositionSeconds();
    const double startT = now - 0.15;              // brief tail so the hit registers visually
    const double endT   = now + lookAheadSeconds;

    frameEvents.clear();
    player.getDrumEventsInRange (startT, endT, frameEvents);

    for (const auto& e : frameEvents)
    {
        const int lane = noteToLane (e.note);
        if (lane < 0) continue;

        const double dt = e.timeSec - now;                            // future > 0
        const float  y  = (float) (hitLineY - (dt / lookAheadSeconds) * laneH);

        const float noteH = 16.0f;
        const float noteW = juce::jmax (24.0f, laneWidth - 16.0f);
        const float x     = lane * laneWidth + (laneWidth - noteW) * 0.5f;

        const juce::Rectangle<float> rect (x, y - noteH * 0.5f, noteW, noteH);

        const float velScale = 0.5f + 0.5f * juce::jlimit (0.0f, 1.0f, e.velocity);
        float       alpha    = 1.0f;
        if (dt < 0.0)
            alpha = juce::jmax (0.0f, 1.0f + (float) dt * 6.0f);      // fade out past-hit

        auto col = laneColour (lane).withMultipliedAlpha (velScale * alpha);
        g.setColour (col);
        g.fillRoundedRectangle (rect, 5.0f);

        g.setColour (juce::Colours::white.withAlpha (0.25f * alpha));
        g.drawRoundedRectangle (rect, 5.0f, 1.0f);
    }

    // Hit line
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.fillRect (0.0f, hitLineY, width, hitH);

    // Lane labels (bottom)
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    for (int i = 0; i < kNumLanes; ++i)
    {
        const float x = i * laneWidth;
        g.setColour (laneColour (i));
        g.drawText (laneName (i),
                    juce::Rectangle<float> (x, hitLineY + hitH, laneWidth, labelH),
                    juce::Justification::centred);
    }
}
