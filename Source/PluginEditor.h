#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"

//==============================================================================
class PiradexMVAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit PiradexMVAudioProcessorEditor (PiradexMVAudioProcessor&);
    ~PiradexMVAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawHeader (juce::Graphics&, juce::Rectangle<int>);
    void drawFaderScale (juce::Graphics&, juce::Rectangle<int>);
    void drawMeters (juce::Graphics&, juce::Rectangle<int>);
    void drawPiradexLogo (juce::Graphics&, juce::Rectangle<float>, juce::Colour);

    PiradexMVAudioProcessor& proc;
    MVLookAndFeel lnf;

    juce::Slider lowFader, highFader, outputKnob, mixKnob, trimKnob;
    juce::TextButton lowLink {"LINK"}, highLink {"LINK"};
    juce::TextButton muteBtn {"MUTE"}, dimBtn {"DIM"}, monoBtn {"MONO"}, bypassBtn {"BYPASS"};
    juce::ComboBox osBox, respBox, modeBox;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SA> lowA, highA, outA, mixA, trimA;
    std::unique_ptr<BA> lowLinkA, highLinkA, muteA, dimA, monoA, bypassA;
    std::unique_ptr<CA> osA, respA, modeA;

    // meters suavizados
    float mL = 0.0f, mR = 0.0f;
    juce::Rectangle<int> meterArea, faderArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PiradexMVAudioProcessorEditor)
};
