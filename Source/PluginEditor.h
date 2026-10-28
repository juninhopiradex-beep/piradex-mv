#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include <map>

//==============================================================================
// Base para componentes que seguem o tema
struct Themed { const Theme* th = nullptr; };

//==============================================================================
// Barra segmentada (Response / Oversampling / Stereo Mode)
class SegBar : public juce::Component, public Themed
{
public:
    std::function<void(int)> onChange;
    void setItems (juce::StringArray i) { labels = std::move (i); repaint(); }
    void setIndex (int i, bool notify = false)
    {
        index = juce::jlimit (0, juce::jmax (0, labels.size()-1), i);
        if (notify && onChange) onChange (index);
        repaint();
    }
    int getIndex() const { return index; }

    void paint (juce::Graphics& g) override
    {
        if (th == nullptr) return;
        auto b = getLocalBounds().toFloat();
        g.setColour (th->slot);          g.fillRoundedRectangle (b, 5.0f);
        g.setColour (th->stroke);        g.drawRoundedRectangle (b, 5.0f, 1.0f);
        if (labels.isEmpty()) return;
        const float segW = b.getWidth() / (float) labels.size();
        for (int i = 0; i < labels.size(); ++i)
        {
            juce::Rectangle<float> seg (b.getX()+segW*i, b.getY(), segW, b.getHeight());
            if (i == index)
            {
                auto s = seg.reduced (2.0f);
                juce::ColourGradient gr (th->blue.brighter (0.25f), s.getX(), s.getY(),
                                         th->blue.darker (0.45f), s.getX(), s.getBottom(), false);
                g.setGradientFill (gr);
                g.fillRoundedRectangle (s, 4.0f);
            }
            g.setColour (i == index ? juce::Colours::white : th->textDim);
            g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
            g.drawText (labels[i], seg, juce::Justification::centred);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (labels.isEmpty()) return;
        setIndex ((int) (e.position.x / ((float) getWidth()/labels.size())), true);
    }
private:
    juce::StringArray labels;
    int index = 0;
};

//==============================================================================
// Botão utilitário com ícone (Bypass / Mute / Dim / Mono)
class IconToggle : public juce::Button, public Themed
{
public:
    enum Kind { Power, Speaker, Half, Rings };
    IconToggle (const juce::String& t, Kind k) : juce::Button (t), kind (k)
    { setClickingTogglesState (true); }

    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        if (th == nullptr) return;
        bool on = getToggleState();
        auto accent = (getButtonText()=="BYPASS") ? th->amber : th->blue;
        auto area = getLocalBounds().toFloat();
        auto icon = area.removeFromTop (area.getHeight()*0.62f).reduced (6.0f);
        auto r = juce::jmin (icon.getWidth(), icon.getHeight()) * 0.42f;
        auto c = icon.getCentre();
        g.setColour (on ? accent : th->textDim.brighter (over ? 0.25f : 0.0f));
        const float lw = 1.8f;

        if (kind == Power)
        {
            juce::Path p; p.addCentredArc (c.x, c.y, r, r, 0.0f, 0.5f, juce::MathConstants<float>::twoPi - 0.5f, true);
            g.strokePath (p, juce::PathStrokeType (lw));
            g.drawLine (c.x, c.y - r - 2.0f, c.x, c.y + 1.0f, lw);
        }
        else if (kind == Speaker)
        {
            juce::Path p;
            p.startNewSubPath (c.x-r, c.y-r*0.4f); p.lineTo (c.x-r*0.3f, c.y-r*0.4f);
            p.lineTo (c.x+r*0.4f, c.y-r); p.lineTo (c.x+r*0.4f, c.y+r);
            p.lineTo (c.x-r*0.3f, c.y+r*0.4f); p.lineTo (c.x-r, c.y+r*0.4f); p.closeSubPath();
            g.fillPath (p);
            if (!on) { g.drawLine (c.x+r*0.7f, c.y-r*0.6f, c.x+r*1.1f, c.y+r*0.6f, lw);
                       g.drawLine (c.x+r*1.1f, c.y-r*0.6f, c.x+r*0.7f, c.y+r*0.6f, lw); }
            else     { juce::Path w; w.addCentredArc (c.x+r*0.5f, c.y, r*0.6f, r*0.6f, 0.0f, 0.5f, 2.2f, true);
                       g.strokePath (w, juce::PathStrokeType (lw)); }
        }
        else if (kind == Half)
        {
            g.drawEllipse (c.x-r, c.y-r, r*2, r*2, lw);
            juce::Path p; p.addPieSegment (c.x-r, c.y-r, r*2, r*2, 0.0f, juce::MathConstants<float>::pi, 0.0f);
            g.fillPath (p);
        }
        else // Rings
        {
            g.drawEllipse (c.x-r, c.y-r*0.55f, r*1.3f, r*1.3f, lw);
            g.drawEllipse (c.x-r*0.3f, c.y-r*0.55f, r*1.3f, r*1.3f, lw);
        }

        g.setColour (on ? accent : th->textDim);
        g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
        g.drawText (getButtonText(), area, juce::Justification::centredTop);
    }
    Kind kind;
};

//==============================================================================
// Botão do cabeçalho (setas, undo/redo, A, B, engrenagem)
class IconButton : public juce::Button, public Themed
{
public:
    enum Kind { Prev, Next, Undo, Redo, A, B, Gear };
    IconButton (Kind k) : juce::Button ("hdr"), kind (k) {}
    bool active = false;

    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        if (th == nullptr) return;
        auto b = getLocalBounds().toFloat().reduced (2.0f);
        bool box = (kind==A || kind==B || kind==Gear);
        if (box)
        {
            g.setColour (active ? th->blue : th->slot);
            g.fillRoundedRectangle (b, 4.0f);
            g.setColour (active ? th->blue.brighter() : th->stroke);
            g.drawRoundedRectangle (b, 4.0f, 1.0f);
        }
        auto c = b.getCentre();
        float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.3f;
        g.setColour (active ? juce::Colours::white : (over ? th->text : th->textDim));
        const float lw = 1.7f;

        auto arrow = [&] (bool right)
        {
            float d = right ? 1.0f : -1.0f;
            juce::Path p; p.startNewSubPath (c.x - d*r*0.5f, c.y - r*0.7f);
            p.lineTo (c.x + d*r*0.6f, c.y); p.lineTo (c.x - d*r*0.5f, c.y + r*0.7f);
            g.strokePath (p, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };
        if      (kind==Prev) arrow (false);
        else if (kind==Next) arrow (true);
        else if (kind==A) { g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold))); g.drawText ("A", b, juce::Justification::centred); }
        else if (kind==B) { g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold))); g.drawText ("B", b, juce::Justification::centred); }
        else if (kind==Undo || kind==Redo)
        {
            bool rd = (kind==Redo); float d = rd?1.0f:-1.0f;
            juce::Path p; p.addCentredArc (c.x, c.y, r, r, 0.0f,
                 rd? -0.4f : 0.4f, rd? 3.4f : -3.4f, true);
            g.strokePath (p, juce::PathStrokeType (lw));
            auto tip = c.getPointOnCircumference (r, rd? -0.4f : 0.4f);
            g.fillEllipse (tip.x-1.6f + d*0.0f, tip.y-1.6f, 3.2f, 3.2f);
        }
        else if (kind==Gear)
        {
            g.drawEllipse (c.x-r*0.5f, c.y-r*0.5f, r, r, lw);
            for (int i=0;i<8;++i){ float a=i*juce::MathConstants<float>::pi/4.0f;
                auto p1=c.getPointOnCircumference (r*0.7f,a); auto p2=c.getPointOnCircumference (r*1.05f,a);
                g.drawLine ({p1,p2}, lw); }
        }
    }
    Kind kind;
};

//==============================================================================
// Meter vertical L/R com escala em dB
class Meter : public juce::Component, public Themed
{
public:
    void setLevels (float l, float r) { lvlL = l; lvlR = r; repaint(); }

    void paint (juce::Graphics& g) override
    {
        if (th == nullptr) return;
        auto b = getLocalBounds().toFloat();
        auto scaleW = 24.0f;
        auto scale = b.removeFromLeft (scaleW);

        g.setFont (juce::Font (juce::FontOptions (7.5f)));
        g.setColour (th->sub);
        const int marks[] = { 0, -6, -12, -24, -36, -48, -60 };
        for (int m : marks)
        {
            float t = (float) (m + 60) / 60.0f;         // 0..1 bottom..top
            float y = b.getBottom() - t * b.getHeight();
            g.drawText (juce::String (m), scale.getX(), (int) y - 5, (int) scaleW - 3, 10,
                        juce::Justification::centredRight);
        }

        auto bars = b.reduced (2.0f, 0.0f);
        float gap = 4.0f;
        float bw = (bars.getWidth() - gap) * 0.5f;
        drawBar (g, bars.removeFromLeft (bw), lvlL);
        bars.removeFromLeft (gap);
        drawBar (g, bars, lvlR);
    }
private:
    void drawBar (juce::Graphics& g, juce::Rectangle<float> r, float level)
    {
        g.setColour (th->slot);
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (th->stroke);
        g.drawRoundedRectangle (r, 2.0f, 0.8f);

        float h = r.getHeight() * juce::jlimit (0.0f, 1.0f, level);
        if (h > 0)
        {
            juce::ColourGradient grad (th->meterLo, r.getX(), r.getBottom(),
                                       th->meterHi, r.getX(), r.getY(), false);
            grad.addColour (0.55, th->meterMid);
            grad.addColour (0.80, th->meterMid.interpolatedWith (th->meterHi, 0.5f));
            g.setGradientFill (grad);
            g.fillRoundedRectangle (r.getX(), r.getBottom()-h, r.getWidth(), h, 2.0f);
        }
        // divisões
        g.setColour (th->bg.withAlpha (0.5f));
        for (int i=1;i<14;++i){ float y=r.getBottom()-r.getHeight()*(i/14.0f); g.drawLine (r.getX(),y,r.getRight(),y,0.6f); }
    }
    float lvlL = 0.0f, lvlR = 0.0f;
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
    struct KnobInfo { juce::Slider* s; juce::String name; std::function<juce::String(double)> fmt; };

    void timerCallback() override;
    void applyThemeToAll();
    void bindChoice (SegBar&, const juce::String& paramID);
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title,
                    const juce::String& subtitle, juce::Colour accent);
    void drawGroup (juce::Graphics&, juce::Rectangle<int>, const juce::String& title);
    void drawKnobLabels (juce::Graphics&);
    void drawHeader (juce::Graphics&, juce::Rectangle<int>);
    void drawBeatfreakLogo (juce::Graphics&, juce::Rectangle<float>);

    // presets / A-B
    std::map<juce::String,float> captureState();
    void loadState (const std::map<juce::String,float>&);
    void applyPreset (int idx);
    void toggleAB (bool toB);

    PiradexMVAudioProcessor& proc;
    MVLookAndFeel lnf;
    Theme th;

    // controlos
    juce::Slider inputTrimK, lowThreshK, lowMaxGainK, trimK, mixK, highThreshK, highRatioK, outputK;
    juce::Slider lowFader, highFader;
    SegBar respSeg, osSeg, modeSeg;
    IconToggle bypassBtn {"BYPASS", IconToggle::Power}, muteBtn {"MUTE", IconToggle::Speaker},
               dimBtn {"DIM", IconToggle::Half}, monoBtn {"MONO", IconToggle::Rings};
    IconButton prevBtn {IconButton::Prev}, nextBtn {IconButton::Next},
               undoBtn {IconButton::Undo}, redoBtn {IconButton::Redo},
               aBtn {IconButton::A}, bBtn {IconButton::B}, gearBtn {IconButton::Gear};
    Meter inMeter, outMeter;

    std::vector<KnobInfo> knobs;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SA>> sAtt;
    std::vector<std::unique_ptr<BA>> bAtt;

    // layout rects (preenchidos em resized, usados em paint)
    juce::Rectangle<int> inCol, lowPanel, centerCol, highPanel, outCol,
                         respGroup, osGroup, modeGroup, utilGroup, headerRect, presetNameArea;

    // estado UI
    int currentPreset = 0;
    bool onB = false;
    std::map<juce::String,float> slotA, slotB;
    juce::StringArray presetNames;

    // meters suavizados
    float mInL=0, mInR=0, mOutL=0, mOutR=0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PiradexMVAudioProcessorEditor)
};
