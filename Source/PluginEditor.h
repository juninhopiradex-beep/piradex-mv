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
    enum Kind { Power, Speaker, Half, Rings, Mid, Side };
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
        else if (kind == Rings)
        {
            g.drawEllipse (c.x-r, c.y-r*0.55f, r*1.3f, r*1.3f, lw);
            g.drawEllipse (c.x-r*0.3f, c.y-r*0.55f, r*1.3f, r*1.3f, lw);
        }
        else if (kind == Mid)
        {
            // centro: círculo com ponto cheio
            g.drawEllipse (c.x-r, c.y-r, r*2, r*2, lw);
            g.fillEllipse (c.x-r*0.32f, c.y-r*0.32f, r*0.64f, r*0.64f);
        }
        else // Side
        {
            // lados: dois arcos a abrir para fora
            juce::Path l; l.addCentredArc (c.x-r*0.2f, c.y, r*0.8f, r, 0.0f,
                 juce::MathConstants<float>::pi*1.15f, juce::MathConstants<float>::pi*1.85f, true);
            juce::Path rr; rr.addCentredArc (c.x+r*0.2f, c.y, r*0.8f, r, 0.0f,
                 juce::MathConstants<float>::pi*0.15f, juce::MathConstants<float>::pi*0.85f, true);
            g.strokePath (l, juce::PathStrokeType (lw));
            g.strokePath (rr, juce::PathStrokeType (lw));
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

        // LEDs segmentados (verde -> amarelo -> laranja -> vermelho)
        const int segs = 24;
        const float gap = 1.4f;
        auto inner = r.reduced (1.6f);
        float segH = (inner.getHeight() - (segs-1)*gap) / segs;
        int lit = juce::roundToInt (juce::jlimit (0.0f, 1.0f, level) * segs);
        for (int i = 0; i < segs; ++i)
        {
            float t = (float) i / (float) (segs - 1);          // 0 base .. 1 topo
            juce::Colour col = t < 0.6f ? th->meterLo.interpolatedWith (th->meterMid, t/0.6f)
                             : t < 0.85f ? th->meterMid.interpolatedWith (th->meterHi, (t-0.6f)/0.25f)
                                         : th->meterHi;
            float y = inner.getBottom() - (i+1)*segH - i*gap;
            juce::Rectangle<float> seg (inner.getX(), y, inner.getWidth(), segH);
            if (i < lit) { g.setColour (col); g.fillRect (seg);
                           g.setColour (juce::Colours::white.withAlpha (0.12f)); g.fillRect (seg.removeFromTop (segH*0.4f)); }
            else         { g.setColour (th->sub.withAlpha (th->dark ? 0.22f : 0.32f)); g.fillRect (seg); }
        }
    }
    float lvlL = 0.0f, lvlR = 0.0f;
};

//==============================================================================
// Visualizador: SPECTRUM / CURVE / METERS (com tabs clicáveis)
class Analyser : public juce::Component, public Themed
{
public:
    explicit Analyser (PiradexMVAudioProcessor& p) : proc (p) {}
    int tab = 0;
    const juce::StringArray tabs { "SPECTRUM", "CURVE", "METERS" };

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.position.y > 34.0f) return;
        float x = 18.0f;
        for (int i = 0; i < tabs.size(); ++i)
        {
            float w = tabWidth (tabs[i]);
            if (e.position.x >= x-6 && e.position.x <= x+w+6) { tab = i; repaint(); return; }
            x += w + 26.0f;
        }
    }

    void paint (juce::Graphics& g) override
    {
        if (th == nullptr) return;
        auto b = getLocalBounds().toFloat();
        // fundo do painel
        juce::ColourGradient pg (th->panelHi, b.getCentreX(), b.getY(),
                                 th->panel, b.getCentreX(), b.getBottom(), false);
        g.setGradientFill (pg); g.fillRoundedRectangle (b, 12.0f);
        g.setColour (th->stroke); g.drawRoundedRectangle (b, 12.0f, 1.0f);

        // tabs
        float x = 18.0f;
        for (int i = 0; i < tabs.size(); ++i)
        {
            float w = tabWidth (tabs[i]);
            g.setColour (i == tab ? th->blue : th->textDim);
            g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
            g.drawText (tabs[i], (int) x, 12, (int) w + 4, 16, juce::Justification::left);
            if (i == tab) { g.setColour (th->blue); g.fillRect (x, 30.0f, w, 2.0f); }
            x += w + 26.0f;
        }

        auto area = b.reduced (16.0f).withTrimmedTop (28.0f);
        if      (tab == 0) drawSpectrum (g, area);
        else if (tab == 1) drawCurve (g, area);
        else               drawMeters (g, area);
    }

private:
    float tabWidth (const juce::String& s) const
    {
        return juce::Font (juce::FontOptions (11.0f, juce::Font::bold)).getStringWidthFloat (s);
    }

    void drawSpectrum (juce::Graphics& g, juce::Rectangle<float> a)
    {
        const int n = PiradexMVAudioProcessor::numBins;
        // grelha vertical (freq)
        const char* labels[] = {"20","50","100","200","500","1k","2k","5k","10k","20k"};
        g.setFont (juce::Font (juce::FontOptions (8.0f)));
        for (int j = 0; j < 10; ++j)
        {
            float gx = a.getX() + (j/9.0f)*a.getWidth();
            g.setColour (th->stroke.withAlpha (0.5f));
            g.drawVerticalLine ((int) gx, a.getY(), a.getBottom());
            g.setColour (th->sub);
            g.drawText (labels[j], (int) gx-12, (int) a.getBottom()+1, 24, 10, juce::Justification::centred);
        }
        for (int k = 1; k < 4; ++k)
        { float gy = a.getY() + k*a.getHeight()/4; g.setColour (th->stroke.withAlpha (0.35f));
          g.drawHorizontalLine ((int) gy, a.getX(), a.getRight()); }

        juce::Path line, fill;
        for (int i = 0; i < n; ++i)
        {
            float v = proc.spectrum[i].load();
            float px = a.getX() + (float) i / (n-1) * a.getWidth();
            float py = a.getBottom() - v * a.getHeight();
            if (i == 0) { line.startNewSubPath (px, py); fill.startNewSubPath (px, a.getBottom()); fill.lineTo (px, py); }
            else { line.lineTo (px, py); fill.lineTo (px, py); }
        }
        fill.lineTo (a.getRight(), a.getBottom()); fill.closeSubPath();
        g.setGradientFill (juce::ColourGradient (th->blue.withAlpha (0.45f), a.getX(), a.getY(),
                                                 th->blue.withAlpha (0.0f), a.getX(), a.getBottom(), false));
        g.fillPath (fill);
        g.setColour (th->blue);
        g.strokePath (line, juce::PathStrokeType (2.0f));
    }

    void drawCurve (juce::Graphics& g, juce::Rectangle<float> a)
    {
        auto pv = [this] (const char* id){ return proc.apvts.getRawParameterValue (id)->load(); };
        float lThr = pv("lowThresh"), lMax = pv("lowMaxGain"), lAmt = pv("lowAmt")/100.0f;
        float hThr = pv("highThresh"), hRat = pv("highRatio"), hAmt = pv("highAmt")/100.0f;
        // quadrado centrado
        float side = juce::jmin (a.getWidth(), a.getHeight());
        juce::Rectangle<float> sq (a.getCentreX()-side/2, a.getY(), side, side);

        g.setColour (th->stroke.withAlpha (0.5f));
        for (int k=1;k<4;++k){ float t=k/4.0f;
            g.drawVerticalLine ((int)(sq.getX()+t*sq.getWidth()), sq.getY(), sq.getBottom());
            g.drawHorizontalLine ((int)(sq.getY()+t*sq.getHeight()), sq.getX(), sq.getRight()); }
        // unidade
        g.setColour (th->sub);
        g.drawLine (sq.getX(), sq.getBottom(), sq.getRight(), sq.getY(), 1.2f);

        auto mapx = [&](float db){ return sq.getX() + (db+60)/60.0f*sq.getWidth(); };
        auto mapy = [&](float db){ return sq.getBottom() - (db+60)/66.0f*sq.getHeight(); };
        juce::Path p; bool first=true;
        for (int i=0;i<=120;++i)
        {
            float indb = -60 + i*0.5f;
            float out = indb;
            if (indb < lThr) { float lift=(lThr-indb)*(1.0f-1.0f/2.5f); out = indb + juce::jmin(lift,lMax)*lAmt; }
            else if (indb > hThr) { float over=indb-hThr; out = indb - (over-over/hRat)*hAmt; }
            float mk = juce::jmin ((-hThr)*(1.0f-1.0f/hRat)*hAmt*0.6f, 12.0f);
            out += mk;
            out = juce::jlimit (-60.0f, 6.0f, out);
            float x=mapx(indb), y=mapy(out);
            if (first){ p.startNewSubPath(x,y); first=false; } else p.lineTo(x,y);
        }
        g.setColour (th->blue);
        g.strokePath (p, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved));
        g.setColour (th->sub); g.setFont (juce::Font (juce::FontOptions (8.5f)));
        g.drawText ("IN", sq.getRight()-20, (int)sq.getBottom()-14, 20, 12, juce::Justification::right);
        g.drawText ("OUT", sq.getX()+2, (int)sq.getY(), 30, 12, juce::Justification::left);
    }

    void drawMeters (juce::Graphics& g, juce::Rectangle<float> a)
    {
        bool ms = (int) proc.apvts.getRawParameterValue ("mode")->load() == 2;
        auto norm=[](float pk){ float d=juce::Decibels::gainToDecibels(pk+1e-6f); return juce::jlimit(0.0f,1.0f,(d+60)/60); };
        struct B { juce::String name; float lv; };
        std::vector<B> bars = {
            { ms?"IN M":"IN L",  norm (ms?proc.inPeakM.load():proc.inPeakL.load()) },
            { ms?"IN S":"IN R",  norm (ms?proc.inPeakS.load():proc.inPeakR.load()) },
            { ms?"OUT M":"OUT L",norm (ms?proc.outPeakM.load():proc.outPeakL.load()) },
            { ms?"OUT S":"OUT R",norm (ms?proc.outPeakS.load():proc.outPeakR.load()) },
        };
        float gap=18.0f, bw=(a.getWidth()-3*gap)/4.0f;
        float x=a.getX();
        for (auto& bar : bars)
        {
            juce::Rectangle<float> r (x, a.getY(), bw, a.getHeight()-16);
            g.setColour (th->slot); g.fillRoundedRectangle (r, 3.0f);
            g.setColour (th->stroke); g.drawRoundedRectangle (r, 3.0f, 1.0f);
            const int segs=28; float sg=2.0f; auto inr=r.reduced(3.0f);
            float sh=(inr.getHeight()-(segs-1)*sg)/segs; int lit=juce::roundToInt(bar.lv*segs);
            for (int i=0;i<segs;++i){ float t=(float)i/(segs-1);
                juce::Colour col = t<0.6f?th->meterLo.interpolatedWith(th->meterMid,t/0.6f)
                                 : t<0.85f?th->meterMid.interpolatedWith(th->meterHi,(t-0.6f)/0.25f):th->meterHi;
                float yy=inr.getBottom()-(i+1)*sh-i*sg;
                g.setColour (i<lit?col:th->sub.withAlpha(0.2f));
                g.fillRect (inr.getX(), yy, inr.getWidth(), sh); }
            g.setColour (th->textDim); g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
            g.drawText (bar.name, (int)x, (int)a.getBottom()-14, (int)bw, 12, juce::Justification::centred);
            x += bw + gap;
        }
    }

    PiradexMVAudioProcessor& proc;
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
    void mouseDown (const juce::MouseEvent&) override;

private:
    struct KnobInfo { juce::Slider* s; juce::String name; std::function<juce::String(double)> fmt;
                      juce::String lo, hi; };

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
               dimBtn {"DIM", IconToggle::Half}, monoBtn {"MONO", IconToggle::Rings},
               midBtn {"MID", IconToggle::Mid}, sideBtn {"SIDE", IconToggle::Side};
    IconButton prevBtn {IconButton::Prev}, nextBtn {IconButton::Next},
               undoBtn {IconButton::Undo}, redoBtn {IconButton::Redo},
               aBtn {IconButton::A}, bBtn {IconButton::B}, gearBtn {IconButton::Gear};
    Meter inMeter, outMeter;
    Analyser analyser { proc };

    std::vector<KnobInfo> knobs;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SA>> sAtt;
    std::vector<std::unique_ptr<BA>> bAtt;

    // layout rects (preenchidos em resized, usados em paint)
    juce::Rectangle<int> inCol, lowPanel, centerCol, highPanel, outCol,
                         respGroup, osGroup, modeGroup, utilGroup, headerRect, presetNameArea, vizRect;

    // estado UI
    int currentPreset = 0;
    bool onB = false;
    std::map<juce::String,float> slotA, slotB;
    juce::StringArray presetNames;

    // meters suavizados (L/R e M/S)
    float mInL=0, mInR=0, mOutL=0, mOutR=0;
    float mInM=0, mInS=0, mOutM=0, mOutS=0;
    juce::String meterLblL = "L", meterLblR = "R";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PiradexMVAudioProcessorEditor)
};
