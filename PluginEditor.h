#pragma once
#include "PluginProcessor.h"
#include <array>

class VibratoEditor : public juce::AudioProcessorEditor
{
public:
    explicit VibratoEditor (VibratoProcessor& p) : juce::AudioProcessorEditor (p), proc (p)
    {
        setSize (640, 420);

        setupKnob (0, "threshold", "Порог",         " кв.");
        setupKnob (1, "delay",     "Задержка",      " кв.");
        setupKnob (2, "ramp",      "Нарастание",    " кв.");
        setupKnob (3, "depth",     "Глубина",       " ц");
        setupKnob (4, "rate",      "Скорость, Гц",  "");
        setupKnob (5, "random",    "Рандом",        " %");
        setupKnob (6, "channel",   "Канал",         "");

        knobs[6].slider.textFromValueFunction = [] (double v)
        {
            return v < 0.5 ? ru ("Авто") : juce::String ((int) v);
        };
        knobs[6].slider.updateText();

        syncBtn.setButtonText (ru ("Синхро с темпом"));
        addAndMakeVisible (syncBtn);
        syncAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "sync", syncBtn);

        setupCombo (divisionBox, divLabel,  divAtt,  "division",  "Деление");
        setupCombo (modeBox,     modeLabel, modeAtt, "mode",      "Режим вывода");
        setupCombo (rangeBox,    rangeLabel, rangeAtt, "bendRange", "Диапазон Bend");
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (20.0f));
        g.drawText ("Guitar Vibrato", 20, 8, 300, 26, juce::Justification::left);
    }

    void resized() override
    {
        for (size_t i = 0; i < knobs.size(); ++i)
        {
            const int row = (int) i / 4, col = (int) i % 4;
            knobs[i].slider.setBounds (20 + col * 150, 60 + row * 140, 130, 110);
        }
        syncBtn.setBounds    (20,  365, 150, 28);
        divisionBox.setBounds (190, 365, 90, 28);
        modeBox.setBounds    (330, 365, 120, 28);
        rangeBox.setBounds   (490, 365, 100, 28);
    }

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };

    void setupKnob (int i, const char* id, const char* name, const char* suffix)
    {
        auto& k = knobs[(size_t) i];
        k.slider.setTextValueSuffix (ru (suffix));
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
        addAndMakeVisible (k.slider);
        k.label.setText (ru (name), juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.attachToComponent (&k.slider, false);
        k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
    }

    void setupCombo (juce::ComboBox& box, juce::Label& label,
                     std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att,
                     const char* id, const char* name)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
            box.addItemList (p->choices, 1);
        addAndMakeVisible (box);
        label.setText (ru (name), juce::dontSendNotification);
        label.attachToComponent (&box, false);
        att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, box);
    }

    VibratoProcessor& proc;
    std::array<Knob, 7> knobs;
    juce::ToggleButton syncBtn;
    juce::ComboBox divisionBox, modeBox, rangeBox;
    juce::Label divLabel, modeLabel, rangeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divAtt, modeAtt, rangeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibratoEditor)
};
