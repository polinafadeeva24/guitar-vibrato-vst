#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

VibratoProcessor::VibratoProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VibratoProcessor::createLayout()
{
    using namespace juce;
    using P = ParameterID;
    AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<AudioParameterFloat> (P { "threshold", 1 }, ru ("Порог"),
           NormalisableRange<float> (0.25f, 4.0f, 0.25f), 0.5f));
    l.add (std::make_unique<AudioParameterFloat> (P { "delay", 1 }, ru ("Задержка"),
           NormalisableRange<float> (0.0f, 2.0f, 0.05f), 0.0f));
    l.add (std::make_unique<AudioParameterFloat> (P { "ramp", 1 }, ru ("Нарастание"),
           NormalisableRange<float> (0.0f, 2.0f, 0.05f), 0.5f));
    l.add (std::make_unique<AudioParameterFloat> (P { "depth", 1 }, ru ("Глубина"),
           NormalisableRange<float> (0.0f, 100.0f, 1.0f), 30.0f));
    l.add (std::make_unique<AudioParameterFloat> (P { "rate", 1 }, ru ("Скорость"),
           NormalisableRange<float> (3.0f, 9.0f, 0.1f), 5.5f));
    l.add (std::make_unique<AudioParameterBool> (P { "sync", 1 }, ru ("Синхро"), false));
    l.add (std::make_unique<AudioParameterChoice> (P { "division", 1 }, ru ("Деление"),
           StringArray { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3));
    l.add (std::make_unique<AudioParameterFloat> (P { "random", 1 }, ru ("Рандом"),
           NormalisableRange<float> (0.0f, 100.0f, 1.0f), 15.0f));
    l.add (std::make_unique<AudioParameterChoice> (P { "mode", 1 }, ru ("Режим вывода"),
           StringArray { "Pitch Bend", "Mod Wheel" }, 0));
    l.add (std::make_unique<AudioParameterChoice> (P { "bendRange", 1 }, ru ("Диапазон Bend"),
           StringArray { ru ("±2"), ru ("±12"), ru ("±24") }, 0));
    l.add (std::make_unique<AudioParameterInt> (P { "channel", 1 }, ru ("Канал"), 0, 16, 0));
    return l;
}

void VibratoProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    held.clear();
    lastValue = -1;
    virtualPpq = 0.0;
}

void VibratoProcessor::readSettings()
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    cfg.threshold = g ("threshold");
    cfg.delay     = g ("delay");
    cfg.ramp      = g ("ramp");
    cfg.depth     = g ("depth");
    cfg.rate      = g ("rate");
    cfg.random    = g ("random") / 100.0f;
    cfg.channel   = (int) g ("channel");
    cfg.sync      = g ("sync") > 0.5f;
    cfg.division  = juce::jlimit (0, 5, (int) g ("division"));
    cfg.mode      = (int) g ("mode");
    cfg.bendRange = juce::jlimit (0, 2, (int) g ("bendRange"));
}

void VibratoProcessor::neutralise (juce::MidiBuffer& out, int pos, int ch)
{
    const int neutral = cfg.mode == 0 ? 8192 : 0;
    if (lastValue == neutral && lastChannel == ch)
        return;
    out.addEvent (cfg.mode == 0 ? juce::MidiMessage::pitchWheel (ch, neutral)
                                : juce::MidiMessage::controllerEvent (ch, 1, neutral), pos);
    lastValue = neutral;
    lastChannel = ch;
}

void VibratoProcessor::handleEvent (juce::MidiMessage m, int pos, double ppq, juce::MidiBuffer& out)
{
    if (cfg.channel > 0 && m.getChannel() > 0)
        m.setChannel (cfg.channel);

    if (m.isNoteOn())
    {
        neutralise (out, pos, m.getChannel()); // каждая новая нота начинается без сдвига
        held.push_back ({ m.getNoteNumber(), m.getChannel(), ppq,
                          1.0 + cfg.random * 0.2 * rnd(),    // чуть разная скорость
                          1.0 + cfg.random * 0.3 * rnd() }); // чуть разная глубина
    }
    else if (m.isNoteOff())
    {
        const int note = m.getNoteNumber(), ch = m.getChannel();
        held.erase (std::remove_if (held.begin(), held.end(),
                    [&] (const Held& h) { return h.note == note && h.channel == ch; }), held.end());
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        held.clear();
    }
    else if (m.isPitchWheel() && cfg.mode == 0)
    {
        return; // свой pitch bend не пропускаем, чтобы не конфликтовал с вибрато
    }

    out.addEvent (m, pos);
}

void VibratoProcessor::emitVibrato (juce::MidiBuffer& out, int pos, double ppq, double bpm)
{
    const auto& h = held.back(); // вибрато у последней сыгранной ноты
    const double cells = (ppq - h.startPpq) / cellBeats;
    const double onset = (double) cfg.threshold + (double) cfg.delay;

    double env = 0.0, lfo = 0.0;
    if (cells >= onset)
    {
        env = cfg.ramp > 0.001f ? juce::jlimit (0.0, 1.0, (cells - onset) / (double) cfg.ramp) : 1.0;
        const double beatsSince = (cells - onset) * cellBeats;
        const double cycles = cfg.sync ? beatsSince / divisionBeats[cfg.division]
                                       : beatsSince * 60.0 / bpm * (double) cfg.rate;
        lfo = std::sin (juce::MathConstants<double>::twoPi * cycles * h.rateFactor);
    }

    int value;
    if (cfg.mode == 0)
    {
        const double cents = (double) cfg.depth * env * h.depthFactor * lfo;
        value = juce::jlimit (0, 16383,
                              8192 + juce::roundToInt (cents / 100.0 / bendRanges[cfg.bendRange] * 8192.0));
    }
    else
    {
        value = juce::jlimit (0, 127,
                juce::roundToInt (127.0 * ((double) cfg.depth / 100.0) * env * h.depthFactor * (0.5 + 0.5 * lfo)));
    }

    if (value == lastValue && h.channel == lastChannel)
        return;

    out.addEvent (cfg.mode == 0 ? juce::MidiMessage::pitchWheel (h.channel, value)
                                : juce::MidiMessage::controllerEvent (h.channel, 1, value), pos);
    lastValue = value;
    lastChannel = h.channel;
}

void VibratoProcessor::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    audio.clear();
    readSettings();

    const int n = audio.getNumSamples();

    double bpm = 120.0, hostPpq = 0.0;
    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto info = ph->getPosition())
        {
            if (auto b = info->getBpm())
                bpm = *b;
            if (info->getIsPlaying())
                if (auto p = info->getPpqPosition())
                {
                    hostPpq = *p;
                    hostPlaying = true;
                }
        }

    const double ppqPerSample = bpm / 60.0 / sr;
    const double ppq0 = hostPlaying ? hostPpq : virtualPpq;
    virtualPpq = ppq0 + n * ppqPerSample;

    juce::MidiBuffer out;
    auto it = midi.begin();
    const auto end = midi.end();
    constexpr int chunk = 64; // как часто обновляем вибрато

    for (int pos = 0; pos < n; pos += chunk)
    {
        const int chunkEnd = juce::jmin (n, pos + chunk);

        if (! held.empty())
            emitVibrato (out, pos, ppq0 + pos * ppqPerSample, bpm);

        for (; it != end && (*it).samplePosition < chunkEnd; ++it)
        {
            const auto meta = *it;
            handleEvent (meta.getMessage(), meta.samplePosition,
                         ppq0 + meta.samplePosition * ppqPerSample, out);
        }
    }

    midi.swapWith (out);
}

juce::AudioProcessorEditor* VibratoProcessor::createEditor() { return new VibratoEditor (*this); }

void VibratoProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void VibratoProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VibratoProcessor(); }
