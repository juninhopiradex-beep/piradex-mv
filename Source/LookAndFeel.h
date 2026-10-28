#pragma once
#include <JuceHeader.h>

//==============================================================================
//  LEVEL-X — tema (escuro / claro) + LookAndFeel detalhado
//==============================================================================
struct Theme
{
    juce::Colour bg, header, panel, panelHi, slot, stroke, text, textDim, sub;
    juce::Colour blue, amber, meterLo, meterMid, meterHi;
    juce::Colour knobHi, knobMid, knobLo, arcBg;
    bool dark = true;

    static Theme darkTheme()
    {
        Theme t;
        t.dark    = true;
        t.bg      = juce::Colour (0xff0b0e12);
        t.header  = juce::Colour (0xff12171e);
        t.panel   = juce::Colour (0xff161b22);
        t.panelHi = juce::Colour (0xff1d242d);
        t.slot    = juce::Colour (0xff05070a);
        t.stroke  = juce::Colour (0xff2b343f);
        t.text    = juce::Colour (0xffe6edf4);
        t.textDim = juce::Colour (0xff8793a1);
        t.sub     = juce::Colour (0xff5f6b78);
        t.blue    = juce::Colour (0xff3aa6ee);
        t.amber   = juce::Colour (0xffe8a33d);
        t.meterLo = juce::Colour (0xff2ecc71);
        t.meterMid= juce::Colour (0xfff1c40f);
        t.meterHi = juce::Colour (0xffe74c3c);
        t.knobHi  = juce::Colour (0xff3b444f);
        t.knobMid = juce::Colour (0xff232a33);
        t.knobLo  = juce::Colour (0xff10151b);
        t.arcBg   = juce::Colour (0xff273039);
        return t;
    }

    static Theme lightTheme()
    {
        Theme t;
        t.dark    = false;
        t.bg      = juce::Colour (0xffd9dde2);
        t.header  = juce::Colour (0xffe9edf1);
        t.panel   = juce::Colour (0xffe3e7ec);
        t.panelHi = juce::Colour (0xffeef1f5);
        t.slot    = juce::Colour (0xffb9c0c8);
        t.stroke  = juce::Colour (0xffb0b8c1);
        t.text    = juce::Colour (0xff1c2530);
        t.textDim = juce::Colour (0xff5b6672);
        t.sub     = juce::Colour (0xff78828e);
        t.blue    = juce::Colour (0xff2f8fd6);
        t.amber   = juce::Colour (0xffd98a2b);
        t.meterLo = juce::Colour (0xff27ae60);
        t.meterMid= juce::Colour (0xffe0b015);
        t.meterHi = juce::Colour (0xffd84a3a);
        t.knobHi  = juce::Colour (0xfff4f6f8);
        t.knobMid = juce::Colour (0xffccd3da);
        t.knobLo  = juce::Colour (0xff9aa3ad);
        t.arcBg   = juce::Colour (0xffc2cad2);
        return t;
    }
};

//==============================================================================
class MVLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MVLookAndFeel() { setTheme (Theme::darkTheme()); }

    void setTheme (const Theme& t)
    {
        th = t;
        setColour (juce::Slider::textBoxTextColourId,    th.text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId,            th.text);
        setColour (juce::Slider::rotarySliderFillColourId, th.blue);
    }
    const Theme& theme() const { return th; }

    static juce::Colour accentOf (juce::Component& c, juce::Colour fallback)
    {
        auto v = c.getProperties().getWithDefault ("accent", (int) fallback.getARGB());
        return juce::Colour ((juce::uint32) (int) v);
    }

    //========================================================================
    // FADER vertical — calha recessada + preenchimento colorido com glow
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider& s) override
    {
        auto accent = accentOf (s, th.blue);
        auto cx = (float) x + (float) w * 0.5f;
        const float trackW = 7.0f;
        juce::Rectangle<float> track (cx - trackW*0.5f, (float) y + 4.0f, trackW, (float) h - 8.0f);

        g.setColour (th.slot);
        g.fillRoundedRectangle (track, trackW*0.5f);
        g.setColour (th.stroke);
        g.drawRoundedRectangle (track, trackW*0.5f, 1.0f);

        juce::Rectangle<float> fill (track.getX(), pos, trackW, track.getBottom() - pos);
        if (fill.getHeight() > 0)
        {
            // glow
            g.setColour (accent.withAlpha (0.25f));
            g.fillRoundedRectangle (fill.expanded (3.0f, 0.0f), trackW);
            juce::ColourGradient grad (accent.brighter (0.3f), fill.getCentreX(), fill.getY(),
                                       accent.darker (0.4f), fill.getCentreX(), fill.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (fill, trackW*0.5f);
        }

        // thumb metálico
        const float thumbW = (float) w * 0.62f;
        const float thumbH = 24.0f;
        juce::Rectangle<float> thumb (cx - thumbW*0.5f, pos - thumbH*0.5f, thumbW, thumbH);

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (thumb.translated (0, 2.0f), 5.0f);
        juce::ColourGradient tg (th.knobHi, thumb.getX(), thumb.getY(),
                                 th.knobLo, thumb.getX(), thumb.getBottom(), false);
        tg.addColour (0.5, th.knobMid);
        g.setGradientFill (tg);
        g.fillRoundedRectangle (thumb, 5.0f);
        g.setColour (th.stroke);
        g.drawRoundedRectangle (thumb, 5.0f, 1.0f);
        g.setColour (accent);
        g.fillRoundedRectangle (thumb.getX()+3.0f, thumb.getCentreY()-1.5f,
                                thumb.getWidth()-6.0f, 3.0f, 1.5f);
    }

    //========================================================================
    // KNOB — corpo metálico, arco de valor colorido, ticks e indicador
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float posProp, float startAng, float endAng,
                           juce::Slider& s) override
    {
        auto accent = accentOf (s, th.blue);
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        auto r  = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto c  = bounds.getCentre();
        auto ang = startAng + posProp * (endAng - startAng);
        const float arcR = r - 2.0f;

        // ticks à volta
        const int ticks = 11;
        for (int i = 0; i < ticks; ++i)
        {
            float t = (float) i / (float) (ticks - 1);
            float a = startAng + t * (endAng - startAng);
            auto p1 = c.getPointOnCircumference (r + 1.5f, a);
            auto p2 = c.getPointOnCircumference (r + 4.5f, a);
            g.setColour (a <= ang ? accent.withAlpha (0.9f) : th.sub.withAlpha (0.6f));
            g.drawLine ({ p1, p2 }, 1.4f);
        }

        // arco de fundo + valor
        juce::Path bg;  bg.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAng, endAng, true);
        g.setColour (th.arcBg);
        g.strokePath (bg, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path val; val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAng, ang, true);
        g.setColour (accent.withAlpha (0.35f));
        g.strokePath (val, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // corpo
        auto bodyR = r - 7.0f;
        juce::Rectangle<float> body (c.x - bodyR, c.y - bodyR, bodyR*2.0f, bodyR*2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (body.translated (0, 2.0f));
        juce::ColourGradient cg (th.knobHi, body.getX(), body.getY(),
                                 th.knobLo, body.getX(), body.getBottom(), false);
        cg.addColour (0.5, th.knobMid);
        g.setGradientFill (cg);
        g.fillEllipse (body);
        g.setColour (th.dark ? juce::Colours::black.withAlpha (0.5f) : th.stroke);
        g.drawEllipse (body, 1.2f);
        // brilho superior
        g.setColour (juce::Colours::white.withAlpha (th.dark ? 0.06f : 0.5f));
        g.fillEllipse (body.reduced (bodyR*0.18f).withY (body.getY()+bodyR*0.12f).withHeight (bodyR*0.8f));

        // indicador
        auto tip  = c.getPointOnCircumference (bodyR - 3.0f, ang);
        auto base = c.getPointOnCircumference (bodyR * 0.35f, ang);
        g.setColour (accent.brighter (0.2f));
        g.drawLine ({ base, tip }, 2.6f);
        g.fillEllipse (tip.x - 2.2f, tip.y - 2.2f, 4.4f, 4.4f);
    }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = juce::LookAndFeel_V4::createSliderTextBox (s);
        l->setJustificationType (juce::Justification::centred);
        l->setColour (juce::Label::textColourId, th.text);
        return l;
    }

private:
    Theme th;
};
