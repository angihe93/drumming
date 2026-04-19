#include "DrumSynth.h"

namespace
{
    constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

    enum class DrumKind
    {
        Kick, Snare, HiHatClosed, HiHatOpen, HiHatPedal,
        TomLow, TomMid, TomHigh,
        Crash, Splash, Ride, RideBell,
        Clap, SideStick, Cowbell,
        Unknown
    };

    DrumKind classify (int n)
    {
        switch (n)
        {
            case 35: case 36: return DrumKind::Kick;
            case 37:          return DrumKind::SideStick;
            case 38: case 40: return DrumKind::Snare;
            case 39:          return DrumKind::Clap;
            case 41: case 43: case 45: return DrumKind::TomLow;
            case 47: case 48: return DrumKind::TomMid;
            case 50:          return DrumKind::TomHigh;
            case 42:          return DrumKind::HiHatClosed;
            case 44:          return DrumKind::HiHatPedal;
            case 46:          return DrumKind::HiHatOpen;
            case 49: case 52: case 57: return DrumKind::Crash;
            case 51: case 59: return DrumKind::Ride;
            case 53:          return DrumKind::RideBell;
            case 55:          return DrumKind::Splash;
            case 56:          return DrumKind::Cowbell;
            default:          return DrumKind::Unknown;
        }
    }
}

void DrumSynth::prepare (double sr)
{
    sampleRate = sr;
    reset();
}

void DrumSynth::reset()
{
    for (auto& v : voices)
        v.active = false;
}

int DrumSynth::lifespanForNote (int n) const
{
    double sec = 0.25;
    switch (classify (n))
    {
        case DrumKind::Kick:        sec = 0.30; break;
        case DrumKind::Snare:       sec = 0.25; break;
        case DrumKind::HiHatClosed: sec = 0.10; break;
        case DrumKind::HiHatOpen:   sec = 0.70; break;
        case DrumKind::HiHatPedal:  sec = 0.08; break;
        case DrumKind::TomLow:
        case DrumKind::TomMid:
        case DrumKind::TomHigh:     sec = 0.45; break;
        case DrumKind::Crash:       sec = 1.60; break;
        case DrumKind::Splash:      sec = 0.90; break;
        case DrumKind::Ride:        sec = 1.20; break;
        case DrumKind::RideBell:    sec = 0.80; break;
        case DrumKind::Clap:        sec = 0.18; break;
        case DrumKind::SideStick:   sec = 0.08; break;
        case DrumKind::Cowbell:     sec = 0.35; break;
        default:                    sec = 0.20; break;
    }
    return (int) (sec * sampleRate);
}

void DrumSynth::trigger (int note, float vel, int sampleOffset)
{
    Voice* target = nullptr;
    for (auto& v : voices)
    {
        if (! v.active) { target = &v; break; }
    }
    if (target == nullptr)
    {
        int oldest = -1;
        for (auto& v : voices)
            if (v.ageSamples > oldest) { oldest = v.ageSamples; target = &v; }
    }

    target->active             = true;
    target->noteNumber         = note;
    target->velocity           = juce::jlimit (0.0f, 1.0f, vel);
    target->ageSamples         = 0;
    target->lifespanSamples    = lifespanForNote (note);
    target->startOffsetInBlock = juce::jmax (0, sampleOffset);
    target->noiseLpState       = 0.0f;
}

float DrumSynth::renderSample (Voice& v, float noise)
{
    const float t = v.ageSamples / (float) sampleRate;

    // Per-voice highpassed noise for each cymbal/snare colour.
    v.noiseLpState += 0.5f * (noise - v.noiseLpState);
    const float hp = noise - v.noiseLpState;

    switch (classify (v.noteNumber))
    {
        case DrumKind::Kick:
        {
            const float freq  = 45.0f + 70.0f * std::exp (-t * 25.0f);
            const float env   = std::exp (-t * 6.5f);
            const float click = (v.ageSamples < 160) ? hp * 0.5f * std::exp (-t * 250.0f) : 0.0f;
            return std::sin (kTwoPi * freq * t) * env + click;
        }
        case DrumKind::Snare:
        {
            const float env  = std::exp (-t * 14.0f);
            const float tone = std::sin (kTwoPi * 185.0f * t) * std::exp (-t * 35.0f) * 0.35f;
            return hp * env * 0.7f + tone;
        }
        case DrumKind::HiHatClosed: return hp * std::exp (-t * 45.0f) * 0.45f;
        case DrumKind::HiHatOpen:   return hp * std::exp (-t *  5.0f) * 0.40f;
        case DrumKind::HiHatPedal:  return hp * std::exp (-t * 70.0f) * 0.35f;

        case DrumKind::TomLow:
        case DrumKind::TomMid:
        case DrumKind::TomHigh:
        {
            float freq = 130.0f;
            switch (v.noteNumber)
            {
                case 41: freq =  95.0f; break;
                case 43: freq = 110.0f; break;
                case 45: freq = 130.0f; break;
                case 47: freq = 160.0f; break;
                case 48: freq = 200.0f; break;
                case 50: freq = 240.0f; break;
                default: break;
            }
            const float pitchMod = 1.0f + std::exp (-t * 25.0f) * 0.25f;
            const float env      = std::exp (-t * 4.5f);
            return std::sin (kTwoPi * freq * pitchMod * t) * env * 0.8f;
        }

        case DrumKind::Crash:  return hp * std::exp (-t * 1.3f) * 0.55f;
        case DrumKind::Splash: return hp * std::exp (-t * 3.0f) * 0.55f;

        case DrumKind::Ride:
        {
            const float env  = std::exp (-t * 2.0f);
            const float bell = std::sin (kTwoPi * 520.0f * t) * 0.18f;
            return (hp * 0.3f + bell) * env;
        }
        case DrumKind::RideBell: return std::sin (kTwoPi * 800.0f * t) * std::exp (-t * 2.0f) * 0.45f;

        case DrumKind::Clap:      return hp * std::exp (-t * 22.0f) * 0.55f;
        case DrumKind::SideStick: return hp * std::exp (-t * 55.0f) * 0.40f;

        case DrumKind::Cowbell:
        {
            const float env = std::exp (-t * 10.0f);
            return (std::sin (kTwoPi * 540.0f * t) + std::sin (kTwoPi * 810.0f * t) * 0.5f) * env * 0.30f;
        }
        default: return 0.0f;
    }
}

void DrumSynth::render (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const int   numChannels = buffer.getNumChannels();
    const float gain        = masterGain.load();

    for (auto& v : voices)
    {
        if (! v.active) continue;

        for (int i = v.startOffsetInBlock; i < numSamples; ++i)
        {
            if (v.ageSamples >= v.lifespanSamples)
            {
                v.active = false;
                break;
            }

            const float noise  = rng.nextFloat() * 2.0f - 1.0f;
            const float sample = renderSample (v, noise) * v.velocity * gain;

            for (int ch = 0; ch < numChannels; ++ch)
                buffer.getWritePointer (ch)[startSample + i] += sample;

            ++v.ageSamples;
        }
        v.startOffsetInBlock = 0;
    }
}
