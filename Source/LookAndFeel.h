#pragma once
#include <JuceHeader.h>

//==============================================================================
//  LEVEL-X — paleta azul crómio
//==============================================================================
namespace MVColours
{
    const juce::Colour panel      { 0xff0e1116 }; // fundo quase preto azulado
    const juce::Colour panelHi    { 0xff1a2029 };
    const juce::Colour slot       { 0xff05070a }; // calhas / recessos
    const juce::Colour stroke     { 0xff2a333f };
    const juce::Colour text       { 0xffd7e3ef };
    const juce::Colour textDim    { 0xff7d8a99 };

    const juce::Colour accent     { 0xff2f9fe6 }; // azul LEVEL-X
    const juce::Colour accentHi   { 0xff67c7ff }; // azul claro (glow)
    const juce::Colour accentDeep { 0xff1663a8 };

    const juce::Colour chromeHi   { 0xffe9eef4 };
    const juce::Colour chromeMid  { 0xff8d99a7 };
    const juce::Colour chromeLo   { 0xff2b333d };
}

//==============================================================================
class MVLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MVLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId,       MVColours::text);
        setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId,               MVColours::text);
        setColour (juce::ComboBox::backgroundColourId,      MVColours::slot);
        setColour (juce::ComboBox::textColourId,            MVColours::text);
        setColour (juce::ComboBox::outlineColourId,         MVColours::stroke);
        setColour (juce::PopupMenu::backgroundColourId,     MVColours::panelHi);
        setColour (juce::PopupMenu::textColourId,           MVColours::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, MVColours::accentDeep);
    }

    //========================================================================
    // FADER vertical — calha recessada + knob azul brilhante
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider& s) override
    {
        auto cx = (float) x + (float) w * 0.5f;
        const float trackW = 8.0f;
        juce::Rectangle<float> track (cx - trackW*0.5f, (float) y, trackW, (float) h);

        // calha
        g.setColour (MVColours::slot);
        g.fillRoundedRectangle (track, trackW*0.5f);
        g.setColour (MVColours::stroke);
        g.drawRoundedRectangle (track, trackW*0.5f, 1.0f);

        // preenchimento azul de baixo até ao cursor (glow)
        juce::Rectangle<float> fill (track.getX(), pos, trackW, track.getBottom() - pos);
        if (fill.getHeight() > 0)
        {
            juce::ColourGradient grad (MVColours::accentHi, fill.getCentreX(), fill.getY(),
                                       MVColours::accentDeep, fill.getCentreX(), fill.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (fill, trackW*0.5f);
        }

        // thumb
        const float thumbW = (float) w * 0.72f;
        const float thumbH = 22.0f;
        juce::Rectangle<float> thumb (cx - thumbW*0.5f, pos - thumbH*0.5f, thumbW, thumbH);

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (thumb.translated (0, 2.0f), 5.0f);

        juce::ColourGradient tg (MVColours::chromeHi, thumb.getX(), thumb.getY(),
                                 MVColours::chromeLo, thumb.getX(), thumb.getBottom(), false);
        tg.addColour (0.5, MVColours::chromeMid);
        g.setGradientFill (tg);
        g.fillRoundedRectangle (thumb, 5.0f);

        // linha central azul brilhante no thumb
        g.setColour (MVColours::accentHi);
        g.fillRoundedRectangle (thumb.getX()+3.0f, thumb.getCentreY()-1.5f,
                                thumb.getWidth()-6.0f, 3.0f, 1.5f);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.fillRoundedRectangle (thumb.getX()+2.0f, thumb.getY()+2.0f, thumb.getWidth()-4.0f, 2.0f, 1.0f);

        juce::ignoreUnused (s);
    }

    //========================================================================
    // KNOB rotativo crómio com anel pontilhado azul
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float posProp, float startAng, float endAng,
                           juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
        auto r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto c = bounds.getCentre();
        auto ang = startAng + posProp * (endAng - startAng);

        // anel pontilhado (track + valor)
        const int dots = 32;
        for (int i = 0; i <= dots; ++i)
        {
            float t  = (float) i / (float) dots;
            float a  = startAng + t * (endAng - startAng);
            float rr = r + 6.0f;
            juce::Point<float> p (c.x + std::cos (a - juce::MathConstants<float>::halfPi) * rr,
                                  c.y + std::sin (a - juce::MathConstants<float>::halfPi) * rr);
            bool on = a <= ang + 0.0001f;
            g.setColour (on ? MVColours::accentHi : MVColours::stroke);
            float d = on ? 3.2f : 2.2f;
            g.fillEllipse (p.x - d*0.5f, p.y - d*0.5f, d, d);
        }

        // corpo crómio
        juce::Rectangle<float> body (c.x - r, c.y - r, r*2.0f, r*2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (body.translated (0, 2.5f));

        juce::ColourGradient cg (MVColours::chromeHi, body.getX(), body.getY(),
                                 MVColours::chromeLo, body.getX(), body.getBottom(), false);
        cg.addColour (0.45, MVColours::chromeMid);
        cg.addColour (0.55, juce::Colour (0xff5a6673));
        g.setGradientFill (cg);
        g.fillEllipse (body);

        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawEllipse (body, 1.0f);

        // cavidade central escura
        auto inner = body.reduced (r * 0.34f);
        juce::ColourGradient ig (MVColours::panelHi, inner.getX(), inner.getY(),
                                 juce::Colours::black, inner.getX(), inner.getBottom(), false);
        g.setGradientFill (ig);
        g.fillEllipse (inner);

        // ponteiro azul
        juce::Point<float> tip (c.x + std::cos (ang - juce::MathConstants<float>::halfPi) * (r - 4.0f),
                                c.y + std::sin (ang - juce::MathConstants<float>::halfPi) * (r - 4.0f));
        juce::Point<float> base (c.x + std::cos (ang - juce::MathConstants<float>::halfPi) * (r * 0.4f),
                                 c.y + std::sin (ang - juce::MathConstants<float>::halfPi) * (r * 0.4f));
        g.setColour (MVColours::accentHi);
        g.drawLine ({ base, tip }, 3.0f);
        g.fillEllipse (tip.x - 2.5f, tip.y - 2.5f, 5.0f, 5.0f);
    }

    //========================================================================
    juce::Font getLabelFont (juce::Label& l) override
    {
        return l.getFont();
    }
};
