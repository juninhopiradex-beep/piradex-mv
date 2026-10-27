#pragma once
#include <JuceHeader.h>

//==============================================================================
//  Paleta (a condizer com o mockup: charcoal escuro + dourado + azul Piradex)
//==============================================================================
namespace MVColours
{
    const juce::Colour bg        { 0xff0e1116 };   // fundo geral
    const juce::Colour panel     { 0xff161b22 };   // painéis
    const juce::Colour panelHi   { 0xff1c232c };   // painel claro
    const juce::Colour stroke    { 0xff2a323d };   // bordas
    const juce::Colour gold      { 0xffe3a63b };   // dourado (faders/knobs)
    const juce::Colour goldHi    { 0xfff5c76a };   // dourado claro
    const juce::Colour blue      { 0xff3d7bd6 };   // azul Piradex
    const juce::Colour text      { 0xffd7dde4 };   // texto
    const juce::Colour textDim   { 0xff8a95a1 };   // texto esbatido
    const juce::Colour meterGrn  { 0xff35c08a };
    const juce::Colour meterYel  { 0xffe8c84a };
    const juce::Colour meterRed  { 0xffe2574c };
}

//==============================================================================
class MVLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MVLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, MVColours::gold);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ComboBox::backgroundColourId, MVColours::panelHi);
        setColour (juce::ComboBox::textColourId, MVColours::text);
        setColour (juce::ComboBox::outlineColourId, MVColours::stroke);
        setColour (juce::ComboBox::arrowColourId, MVColours::textDim);
        setColour (juce::PopupMenu::backgroundColourId, MVColours::panelHi);
        setColour (juce::PopupMenu::textColourId, MVColours::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, MVColours::gold.withAlpha (0.25f));
    }

    //-------------------------------------------------- fader vertical
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider&) override
    {
        auto cx = x + w * 0.5f;
        const float trackW = 6.0f;

        // calha
        juce::Rectangle<float> track (cx - trackW*0.5f, (float) y, trackW, (float) h);
        g.setColour (MVColours::bg);
        g.fillRoundedRectangle (track, 3.0f);
        g.setColour (MVColours::stroke);
        g.drawRoundedRectangle (track, 3.0f, 1.0f);

        // preenchimento dourado (do centro 0 dB até à posição)
        float zeroY = y + h * 0.5f;   // 0 dB aprox. ao centro da escala visual
        float top = juce::jmin (pos, zeroY);
        float bot = juce::jmax (pos, zeroY);
        g.setColour (MVColours::gold.withAlpha (0.9f));
        g.fillRoundedRectangle (cx - trackW*0.5f, top, trackW, bot - top, 3.0f);

        // thumb metálico
        const float tw = 26.0f, th = 13.0f;
        juce::Rectangle<float> thumb (cx - tw*0.5f, pos - th*0.5f, tw, th);
        juce::ColourGradient grad (juce::Colour(0xff4a4f57), thumb.getX(), thumb.getY(),
                                   juce::Colour(0xff23272d), thumb.getX(), thumb.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (thumb, 3.0f);
        g.setColour (MVColours::goldHi);
        g.fillRect (thumb.getX()+3.0f, thumb.getCentreY()-0.5f, tw-6.0f, 1.5f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (thumb, 3.0f, 1.0f);
    }

    //-------------------------------------------------- knob rotativo metálico
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float posProp, float startA, float endA,
                           juce::Slider& s) override
    {
        auto b = juce::Rectangle<float> (x, y, w, h).reduced (6.0f);
        auto r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        auto c = b.getCentre();
        auto ang = startA + posProp * (endA - startA);

        // arco de fundo
        juce::Path bgArc;
        bgArc.addCentredArc (c.x, c.y, r, r, 0.0f, startA, endA, true);
        g.setColour (MVColours::stroke);
        g.strokePath (bgArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        // arco dourado até ao valor
        juce::Path valArc;
        valArc.addCentredArc (c.x, c.y, r, r, 0.0f, startA, ang, true);
        g.setColour (MVColours::gold);
        g.strokePath (valArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        // corpo metálico
        auto body = b.reduced (r * 0.28f);
        juce::ColourGradient grad (juce::Colour(0xff5a616b), body.getX(), body.getY(),
                                   juce::Colour(0xff20262e), body.getX(), body.getBottom(), false);
        g.setGradientFill (grad);
        g.fillEllipse (body);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.drawEllipse (body, 1.2f);

        // ponteiro dourado
        juce::Path ptr;
        float pr = body.getWidth() * 0.5f;
        ptr.startNewSubPath (c.x, c.y);
        ptr.lineTo (c.x + pr * 0.82f * std::cos (ang - juce::MathConstants<float>::halfPi),
                    c.y + pr * 0.82f * std::sin (ang - juce::MathConstants<float>::halfPi));
        g.setColour (MVColours::goldHi);
        g.strokePath (ptr, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        juce::ignoreUnused (s);
    }

    //-------------------------------------------------- botões toggle
    void drawButtonBackground (juce::Graphics& g, juce::Button& b,
                               const juce::Colour&, bool, bool isDown) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        bool on = b.getToggleState();
        auto fill = on ? MVColours::gold.withAlpha (0.90f)
                       : (isDown ? MVColours::panelHi.brighter (0.1f) : MVColours::panelHi);
        g.setColour (fill);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (on ? MVColours::goldHi : MVColours::stroke);
        g.drawRoundedRectangle (r, 5.0f, 1.0f);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setColour (b.getToggleState() ? juce::Colour (0xff20160a) : MVColours::textDim);
        g.setFont (juce::Font (12.0f, juce::Font::bold));
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, false);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (13.0f); }
    juce::Font getPopupMenuFont() override { return juce::Font (13.0f); }
};
