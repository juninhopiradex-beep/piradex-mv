#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"

//==============================================================================
//  Controlo segmentado (OVERSAMPLING / RESPONSE / OUTPUT MODE) ligado a
//  um AudioParameterChoice via o seu índice.
//==============================================================================
class SegmentedControl : public juce::Component
{
public:
    std::function<void(int)> onChange;

    void setItems (juce::StringArray items) { labels = std::move (items); repaint(); }
    void setIndex (int i, bool notify = false)
    {
        index = juce::jlimit (0, juce::jmax (0, labels.size()-1), i);
        if (notify && onChange) onChange (index);
        repaint();
    }
    int  getIndex() const { return index; }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (MVColours::slot);
        g.fillRoundedRectangle (b, 5.0f);
        g.setColour (MVColours::stroke);
        g.drawRoundedRectangle (b, 5.0f, 1.0f);

        if (labels.isEmpty()) return;
        const float segW = b.getWidth() / (float) labels.size();
        for (int i = 0; i < labels.size(); ++i)
        {
            juce::Rectangle<float> seg (b.getX() + segW*i, b.getY(), segW, b.getHeight());
            if (i == index)
            {
                auto s = seg.reduced (2.0f);
                juce::ColourGradient grad (MVColours::accentHi, s.getX(), s.getY(),
                                           MVColours::accentDeep, s.getX(), s.getBottom(), false);
                g.setGradientFill (grad);
                g.fillRoundedRectangle (s, 4.0f);
                g.setColour (juce::Colours::white.withAlpha (0.18f));
                g.fillRoundedRectangle (s.getX(), s.getY(), s.getWidth(), s.getHeight()*0.45f, 4.0f);
            }
            g.setColour (i == index ? juce::Colours::white : MVColours::textDim);
            g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
            g.drawText (labels[i], seg, juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (labels.isEmpty()) return;
        const float segW = (float) getWidth() / (float) labels.size();
        setIndex ((int) (e.position.x / segW), true);
    }

private:
    juce::StringArray labels;
    int index = 0;
};

//==============================================================================
//  Botão 3D azul retroiluminado (toggle)
//==============================================================================
class GlowButton : public juce::Button
{
public:
    explicit GlowButton (const juce::String& t) : juce::Button (t) { setClickingTogglesState (true); }

    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        auto b = getLocalBounds().toFloat().reduced (1.5f);
        bool on = getToggleState();

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (b.translated (0, 2.0f), 6.0f);

        juce::ColourGradient grad (on ? MVColours::accentHi : MVColours::panelHi,
                                   b.getX(), b.getY(),
                                   on ? MVColours::accentDeep : MVColours::slot,
                                   b.getX(), b.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (b, 6.0f);

        g.setColour (juce::Colours::white.withAlpha (on ? 0.35f : 0.12f));
        g.fillRoundedRectangle (b.getX(), b.getY(), b.getWidth(), b.getHeight()*0.42f, 6.0f);

        g.setColour (on ? MVColours::accentHi.withAlpha (0.9f) : MVColours::stroke);
        g.drawRoundedRectangle (b, 6.0f, on ? 1.4f : 1.0f);

        g.setColour (on ? juce::Colours::white : MVColours::textDim.brighter (over ? 0.3f : 0.0f));
        g.setFont (juce::Font (juce::FontOptions (12.5f, juce::Font::bold)));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred);
    }
};

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
    void drawFaderColumn (juce::Graphics&, juce::Rectangle<int>, const juce::String& title,
                          float grDb);
    void drawMeters (juce::Graphics&, juce::Rectangle<int>);
    void drawDiamond (juce::Graphics&, juce::Rectangle<float>);
    void bindChoice (SegmentedControl&, const juce::String& paramID);

    PiradexMVAudioProcessor& proc;
    MVLookAndFeel lnf;

    juce::Slider lowFader, highFader, outputKnob, mixKnob, trimKnob;
    GlowButton muteBtn {"MUTE"}, dimBtn {"DIM"}, monoBtn {"MONO"}, bypassBtn {"BYPASS"};
    SegmentedControl osSeg, respSeg, modeSeg;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> lowA, highA, outA, mixA, trimA;
    std::unique_ptr<BA> muteA, dimA, monoA, bypassA;

    float mL = 0.0f, mR = 0.0f;          // meters suavizados (0..1)
    float grLowS = 0.0f, grHighS = 0.0f; // gain reduction suavizado (dB)

    juce::Rectangle<int> lowCol, highCol, meterArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PiradexMVAudioProcessorEditor)
};
