#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

inline juce::String ru (const char* s) { return juce::String::fromUTF8 (s); }

class VibratoProcessor : public juce::AudioProcessor
{
public:
    VibratoProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Guitar Vibrato"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    struct Settings
    {
        float threshold = 0.5f, delay = 0.f, ramp = 0.5f, depth = 30.f, rate = 5.5f, random = 0.15f;
        int channel = 0, division = 3, mode = 0, bendRange = 0;
        bool sync = false;
    };

    struct Held
    {
        int note, channel;
        double startPpq, rateFactor, depthFactor;
    };

    // 1 "квадратик" пианоролла = 1 доля (четвертная нота) = 1.0 ppq.
    // Если у тебя квадратик другой, поменяй это число.
    static constexpr double cellBeats = 1.0;
    static constexpr double divisionBeats[6] = { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
    static constexpr double bendRanges[3] = { 2.0, 12.0, 24.0 };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void readSettings();
    void handleEvent (juce::MidiMessage m, int pos, double ppq, juce::MidiBuffer& out);
    void emitVibrato (juce::MidiBuffer& out, int pos, double ppq, double bpm);
    void neutralise (juce::MidiBuffer& out, int pos, int ch);
    float rnd() { return rng.nextFloat() * 2.0f - 1.0f; }

    Settings cfg;
    std::vector<Held> held;
    double virtualPpq = 0.0, sr = 44100.0;
    int lastValue = -1, lastChannel = 1;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibratoProcessor)
};
