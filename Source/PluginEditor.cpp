#include "PluginEditor.h"

//==============================================================================
namespace fmt
{
    static juce::String dbpm (double v) { return (v>0?"+":"") + juce::String (v,1) + " dB"; }
    static juce::String db   (double v) { return juce::String (v,1) + " dB"; }
    static juce::String gain (double v) { return "+" + juce::String (v,1) + " dB"; }
    static juce::String ratio(double v) { return juce::String (v,1) + ":1"; }
    static juce::String pct  (double v) { return juce::String ((int) v) + " %"; }
}

//==============================================================================
static std::vector<std::pair<juce::String, std::map<juce::String,float>>> makePresets()
{
    using M = std::map<juce::String,float>;
    std::vector<std::pair<juce::String, M>> v;
    v.push_back ({ "Vocal Lift",      M{{"inputTrim",0},{"lowAmt",70},{"lowThresh",-28},{"lowMaxGain",12},{"trim",0},{"mix",100},{"highAmt",60},{"highThresh",-6},{"highRatio",4},{"output",-1},{"os",1},{"response",1},{"mode",0}} });
    v.push_back ({ "Master Control",  M{{"inputTrim",0},{"lowAmt",40},{"lowThresh",-34},{"lowMaxGain",8}, {"trim",0},{"mix",100},{"highAmt",75},{"highThresh",-3},{"highRatio",6},{"output",0},{"os",1},{"response",2},{"mode",0}} });
    v.push_back ({ "Vocal Smooth",    M{{"inputTrim",0},{"lowAmt",55},{"lowThresh",-30},{"lowMaxGain",10},{"trim",0},{"mix",100},{"highAmt",45},{"highThresh",-8},{"highRatio",3},{"output",-1},{"os",0},{"response",1},{"mode",0}} });
    v.push_back ({ "Drum Glue",       M{{"inputTrim",0},{"lowAmt",50},{"lowThresh",-24},{"lowMaxGain",9}, {"trim",0},{"mix",100},{"highAmt",65},{"highThresh",-5},{"highRatio",8},{"output",-1},{"os",1},{"response",0},{"mode",1}} });
    v.push_back ({ "Bus Transparent", M{{"inputTrim",0},{"lowAmt",30},{"lowThresh",-40},{"lowMaxGain",6}, {"trim",0},{"mix",100},{"highAmt",50},{"highThresh",-2},{"highRatio",4},{"output",0},{"os",2},{"response",2},{"mode",0}} });
    return v;
}
static auto presetTable = makePresets();

//==============================================================================
PiradexMVAudioProcessorEditor::PiradexMVAudioProcessorEditor (PiradexMVAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    th = proc.apvts.state.getProperty ("uiDark", true) ? Theme::darkTheme() : Theme::lightTheme();

    auto rotary = [this] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f, true);
        addAndMakeVisible (s);
    };
    for (auto* k : { &inputTrimK,&lowThreshK,&lowMaxGainK,&trimK,&mixK,&highThreshK,&highRatioK,&outputK })
        rotary (*k);

    auto fader = [this] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (s);
    };
    fader (lowFader); fader (highFader);

    for (auto* sg : { &respSeg, &osSeg, &modeSeg }) addAndMakeVisible (*sg);
    respSeg.setItems ({ "FAST","MEDIUM","SLOW" });
    osSeg.setItems   ({ "1x","2x","4x","8x" });
    modeSeg.setItems ({ "STEREO","DUAL MONO","MID-SIDE" });

    for (auto* b : { &bypassBtn,&muteBtn,&dimBtn,&monoBtn }) addAndMakeVisible (*b);
    for (auto* b : { &prevBtn,&nextBtn,&undoBtn,&redoBtn,&aBtn,&bBtn,&gearBtn }) addAndMakeVisible (*b);
    addAndMakeVisible (inMeter); addAndMakeVisible (outMeter);

    // attachments
    auto& v = proc.apvts;
    auto SAmk = [&] (const char* id, juce::Slider& s){ sAtt.push_back (std::make_unique<SA>(v,id,s)); };
    SAmk ("inputTrim", inputTrimK); SAmk ("lowThresh", lowThreshK); SAmk ("lowMaxGain", lowMaxGainK);
    SAmk ("trim", trimK); SAmk ("mix", mixK); SAmk ("highThresh", highThreshK);
    SAmk ("highRatio", highRatioK); SAmk ("output", outputK);
    SAmk ("lowAmt", lowFader); SAmk ("highAmt", highFader);
    auto BAmk = [&] (const char* id, juce::Button& b){ bAtt.push_back (std::make_unique<BA>(v,id,b)); };
    BAmk ("bypass", bypassBtn); BAmk ("mute", muteBtn); BAmk ("dim", dimBtn); BAmk ("mono", monoBtn);

    bindChoice (respSeg, "response");
    bindChoice (osSeg,   "os");
    bindChoice (modeSeg, "mode");

    // knob info (nome + formato) para desenhar labels/valores
    knobs = {
        { &inputTrimK, "INPUT TRIM", fmt::dbpm, "", ""   },
        { &lowThreshK, "THRESHOLD",  fmt::db,   "-60", "0" },
        { &lowMaxGainK,"MAX GAIN",   fmt::gain, "0", "24"  },
        { &trimK,      "TRIM",       fmt::dbpm, "-12", "+12" },
        { &mixK,       "MIX",        fmt::pct,  "0", "100" },
        { &highThreshK,"THRESHOLD",  fmt::db,   "-60", "0" },
        { &highRatioK, "RATIO",      fmt::ratio,"1", "20"  },
        { &outputK,    "OUTPUT",     fmt::dbpm, "", ""   },
    };

    // presets
    for (auto& pr : presetTable) presetNames.add (pr.first);

    // header buttons
    prevBtn.onClick = [this]{ applyPreset ((currentPreset - 1 + (int)presetTable.size()) % (int)presetTable.size()); };
    nextBtn.onClick = [this]{ applyPreset ((currentPreset + 1) % (int)presetTable.size()); };
    aBtn.onClick = [this]{ toggleAB (false); };
    bBtn.onClick = [this]{ toggleAB (true); };
    gearBtn.onClick = [this]
    {
        th = th.dark ? Theme::lightTheme() : Theme::darkTheme();
        proc.apvts.state.setProperty ("uiDark", th.dark, nullptr);
        gearBtn.active = !th.dark;
        applyThemeToAll();
    };

    aBtn.active = true;
    slotA = captureState();
    slotB = slotA;

    applyThemeToAll();
    setSize (1160, 560);
    startTimerHz (30);
}

PiradexMVAudioProcessorEditor::~PiradexMVAudioProcessorEditor() { setLookAndFeel (nullptr); }

//==============================================================================
void PiradexMVAudioProcessorEditor::applyThemeToAll()
{
    lnf.setTheme (th);
    setLookAndFeel (&lnf);

    for (auto* c : std::initializer_list<Themed*>{ &respSeg,&osSeg,&modeSeg,
            &bypassBtn,&muteBtn,&dimBtn,&monoBtn,
            &prevBtn,&nextBtn,&undoBtn,&redoBtn,&aBtn,&bBtn,&gearBtn,&inMeter,&outMeter })
        c->th = &th;

    auto setAccent = [] (juce::Slider& s, juce::Colour col){ s.getProperties().set ("accent", (int) col.getARGB()); };
    setAccent (lowFader, th.blue);   setAccent (highFader, th.amber);
    setAccent (lowThreshK, th.blue); setAccent (lowMaxGainK, th.blue);
    setAccent (highThreshK, th.amber); setAccent (highRatioK, th.amber);
    setAccent (inputTrimK, th.blue); setAccent (trimK, th.blue);
    setAccent (mixK, th.blue);       setAccent (outputK, th.blue);

    for (auto* s : { &inputTrimK,&lowThreshK,&lowMaxGainK,&trimK,&mixK,&highThreshK,&highRatioK,&outputK,&lowFader,&highFader })
        s->setLookAndFeel (&lnf);

    repaint();
}

void PiradexMVAudioProcessorEditor::bindChoice (SegBar& seg, const juce::String& id)
{
    if (auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
    {
        seg.setIndex (pc->getIndex());
        seg.onChange = [pc] (int i)
        {
            pc->beginChangeGesture();
            pc->setValueNotifyingHost (pc->convertTo0to1 ((float) i));
            pc->endChangeGesture();
        };
    }
}

//==============================================================================
std::map<juce::String,float> PiradexMVAudioProcessorEditor::captureState()
{
    std::map<juce::String,float> m;
    for (auto& id : PiradexMVAudioProcessor::paramIDs())
        if (auto* rp = proc.apvts.getRawParameterValue (id)) m[id] = rp->load();
    return m;
}

void PiradexMVAudioProcessorEditor::loadState (const std::map<juce::String,float>& m)
{
    for (auto& kv : m)
        if (auto* p = proc.apvts.getParameter (kv.first))
            p->setValueNotifyingHost (p->convertTo0to1 (kv.second));
    respSeg.setIndex ((int) proc.apvts.getRawParameterValue ("response")->load());
    osSeg.setIndex   ((int) proc.apvts.getRawParameterValue ("os")->load());
    modeSeg.setIndex ((int) proc.apvts.getRawParameterValue ("mode")->load());
}

void PiradexMVAudioProcessorEditor::applyPreset (int idx)
{
    idx = juce::jlimit (0, (int) presetTable.size()-1, idx);
    currentPreset = idx;
    loadState (presetTable[(size_t) idx].second);
    repaint();
}

void PiradexMVAudioProcessorEditor::toggleAB (bool toB)
{
    if (toB == onB) return;
    if (onB) slotB = captureState(); else slotA = captureState();   // guarda o atual
    onB = toB;
    loadState (onB ? slotB : slotA);
    aBtn.active = !onB; bBtn.active = onB;
    repaint();
}

//==============================================================================
void PiradexMVAudioProcessorEditor::timerCallback()
{
    auto norm = [] (float peak){ float db = juce::Decibels::gainToDecibels (peak + 1.0e-6f);
                                 return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f); };
    auto sm = [] (float& s, float t){ s += (t - s) * (t > s ? 0.5f : 0.12f); };
    sm (mInL, norm (proc.inPeakL.load()));  sm (mInR, norm (proc.inPeakR.load()));
    sm (mOutL,norm (proc.outPeakL.load())); sm (mOutR,norm (proc.outPeakR.load()));
    inMeter.setLevels (mInL, mInR);
    outMeter.setLevels (mOutL, mOutR);
    repaint();
}

//==============================================================================
void PiradexMVAudioProcessorEditor::drawBeatfreakLogo (juce::Graphics& g, juce::Rectangle<float> r)
{
    // "by Piradex" com um pequeno losango azul, alinhado à direita
    juce::Font fby (juce::FontOptions (12.0f));
    juce::Font fnm (juce::FontOptions (16.0f, juce::Font::bold));
    const juce::String by = "by ", nm = "Piradex";
    float wby = fby.getStringWidthFloat (by);
    float wnm = fnm.getStringWidthFloat (nm);
    const float dia = 12.0f, gap = 8.0f;
    float blockW = dia + gap + wby + wnm;
    float x = r.getRight() - blockW;
    float cy = r.getCentreY();

    juce::Rectangle<float> d (x, cy - dia*0.5f, dia, dia);
    juce::Path dp;
    dp.startNewSubPath (d.getCentreX(), d.getY());
    dp.lineTo (d.getRight(), d.getCentreY());
    dp.lineTo (d.getCentreX(), d.getBottom());
    dp.lineTo (d.getX(), d.getCentreY());
    dp.closeSubPath();
    juce::ColourGradient dg (th.blue.brighter (0.3f), d.getX(), d.getY(),
                             th.blue.darker (0.3f), d.getRight(), d.getBottom(), false);
    g.setGradientFill (dg); g.fillPath (dp);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.strokePath (dp, juce::PathStrokeType (1.0f));

    x += dia + gap;
    g.setColour (th.textDim);
    g.setFont (fby);
    g.drawText (by, x, r.getY(), wby + 2.0f, r.getHeight(), juce::Justification::centredLeft);
    g.setColour (th.text);
    g.setFont (fnm);
    g.drawText (nm, x + wby, r.getY(), wnm + 4.0f, r.getHeight(), juce::Justification::centredLeft);
}

void PiradexMVAudioProcessorEditor::drawHeader (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto a = area.toFloat();
    juce::ColourGradient hg (th.header.brighter (0.04f), a.getX(), a.getY(),
                             th.header, a.getX(), a.getBottom(), false);
    g.setGradientFill (hg); g.fillRect (a);
    g.setColour (th.blue.withAlpha (0.5f));
    g.fillRect (a.getX(), a.getBottom()-1.5f, a.getWidth(), 1.5f);

    auto hr = area.reduced (18, 0);
    // título
    auto title = hr.removeFromLeft (300);
    g.setColour (th.text);
    g.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    g.drawText ("LEVEL-X", title.removeFromTop (44).withTrimmedTop (8), juce::Justification::bottomLeft);
    g.setColour (th.blue);
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    g.drawText ("DUAL-LEVEL DYNAMIC PROCESSOR", title, juce::Justification::topLeft);

    // logo beatfreak à direita
    drawBeatfreakLogo (g, hr.removeFromRight (165).toFloat().withSizeKeepingCentre (165, 34));

    // caixa do preset
    if (! presetNameArea.isEmpty())
    {
        auto pb = presetNameArea.toFloat();
        g.setColour (th.slot);   g.fillRoundedRectangle (pb, 5.0f);
        g.setColour (th.stroke); g.drawRoundedRectangle (pb, 5.0f, 1.0f);
        g.setColour (th.text);
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        g.drawText (presetNames[currentPreset], pb.withTrimmedBottom (8), juce::Justification::centred);
        // dots
        int n = presetNames.size();
        float dotY = pb.getBottom() - 7.0f, cx = pb.getCentreX(), sp = 8.0f;
        for (int i=0;i<n;++i){ float dx = cx + (i - (n-1)/2.0f)*sp;
            g.setColour (i==currentPreset ? th.blue : th.sub);
            g.fillEllipse (dx-1.6f, dotY-1.6f, 3.2f, 3.2f); }
    }
}

//==============================================================================
void PiradexMVAudioProcessorEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> rr,
                                               const juce::String& title, const juce::String& subtitle,
                                               juce::Colour accent)
{
    auto a = rr.toFloat();
    juce::ColourGradient pg (th.panelHi, a.getCentreX(), a.getY(),
                             th.panel, a.getCentreX(), a.getBottom(), false);
    g.setGradientFill (pg); g.fillRoundedRectangle (a, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (th.dark ? 0.05f : 0.5f));
    g.drawRoundedRectangle (a.reduced (0.6f).withTrimmedBottom (a.getHeight()*0.5f), 8.0f, 1.0f);
    g.setColour (th.stroke);  g.drawRoundedRectangle (a, 8.0f, 1.0f);
    auto head = rr.reduced (10, 0).removeFromTop (34);
    g.setColour (th.text);
    g.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
    g.drawText (title, head, juce::Justification::centredTop);
    g.setColour (accent);
    g.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
    g.drawText (subtitle, head.withTrimmedTop (18), juce::Justification::centredTop);
}

void PiradexMVAudioProcessorEditor::drawGroup (juce::Graphics& g, juce::Rectangle<int> rr, const juce::String& title)
{
    auto a = rr.toFloat();
    g.setColour (th.panel);  g.fillRoundedRectangle (a, 7.0f);
    g.setColour (th.stroke); g.drawRoundedRectangle (a, 7.0f, 1.0f);
    g.setColour (th.textDim);
    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    g.drawText (title, rr.removeFromTop (17), juce::Justification::centred);
}

void PiradexMVAudioProcessorEditor::drawKnobLabels (juce::Graphics& g)
{
    for (auto& k : knobs)
    {
        auto b = k.s->getBounds();
        int cx = b.getCentreX();

        // rótulos de range nos cantos inferiores do knob
        if (k.lo.isNotEmpty())
        {
            g.setColour (th.sub);
            g.setFont (juce::Font (juce::FontOptions (8.0f)));
            g.drawText (k.lo, b.getX()-4, b.getBottom()-16, 26, 11, juce::Justification::centredLeft);
            g.drawText (k.hi, b.getRight()-22, b.getBottom()-16, 26, 11, juce::Justification::centredRight);
        }

        // nome
        g.setColour (th.textDim);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawText (k.name, cx-70, b.getBottom()+2, 140, 12, juce::Justification::centred);

        // caixa de valor recessada
        juce::Rectangle<float> vb ((float) cx-33, (float) b.getBottom()+15, 66.0f, 17.0f);
        g.setColour (th.slot);   g.fillRoundedRectangle (vb, 3.5f);
        g.setColour (th.stroke); g.drawRoundedRectangle (vb, 3.5f, 1.0f);
        g.setColour (th.text);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawText (k.fmt (k.s->getValue()), vb, juce::Justification::centred);
    }
}

//==============================================================================
void PiradexMVAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (th.bg);
    drawHeader (g, headerRect);

    // colunas IN / OUT (labels)
    auto meterCol = [&] (juce::Rectangle<int> col, const juce::String& title, float pL, float pR)
    {
        auto head = col.removeFromTop (38);
        g.setColour (th.text);
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        g.drawText (title, head.removeFromTop (18), juce::Justification::centred);
        g.setColour (th.sub);
        g.setFont (juce::Font (juce::FontOptions (7.5f, juce::Font::bold)));
        g.drawText ("PEAK", head.removeFromTop (10), juce::Justification::centred);
        auto pk = [] (float lin){ float d = juce::Decibels::gainToDecibels (lin + 1.0e-6f);
                                  return d <= -60.0f ? juce::String("-inf") : juce::String (d,1); };
        g.setColour (th.textDim);
        g.setFont (juce::Font (juce::FontOptions (8.5f)));
        g.drawText (pk(pL) + "   " + pk(pR), head, juce::Justification::centred);
    };
    meterCol (inCol,  "IN",  proc.inPeakL.load(),  proc.inPeakR.load());
    meterCol (outCol, "OUT", proc.outPeakL.load(), proc.outPeakR.load());

    // L/R por baixo dos meters
    auto lr = [&] (juce::Rectangle<int> col){
        g.setColour (th.sub); g.setFont (juce::Font (juce::FontOptions (8.0f, juce::Font::bold)));
        auto row = col.withTop (inMeter.getBottom()).removeFromTop (12);
        g.drawText ("L        R", row, juce::Justification::centred);
    };
    lr (inCol); lr (outCol);

    drawPanel (g, lowPanel,  "LOW",  "UPWARD COMPRESSION", th.blue);
    drawPanel (g, highPanel, "HIGH", "DOWNWARD LIMITING",  th.amber);

    // quadros centrais TRIM / MIX
    {
        auto cc = centerCol;
        auto topBox = cc.removeFromTop (cc.getHeight()/2).reduced (0, 2);
        auto botBox = cc.reduced (0, 2);
        for (auto box : { topBox, botBox })
        {
            auto bf = box.toFloat();
            juce::ColourGradient pg (th.panelHi, bf.getCentreX(), bf.getY(),
                                     th.panel, bf.getCentreX(), bf.getBottom(), false);
            g.setGradientFill (pg); g.fillRoundedRectangle (bf, 7.0f);
            g.setColour (th.stroke); g.drawRoundedRectangle (bf, 7.0f, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (th.dark ? 0.04f : 0.4f));
            g.drawRoundedRectangle (bf.reduced (0.6f).withTrimmedBottom (bf.getHeight()*0.6f), 7.0f, 1.0f);
        }
    }

    drawGroup (g, respGroup, "RESPONSE");
    drawGroup (g, osGroup,   "OVERSAMPLING");
    drawGroup (g, modeGroup, "STEREO MODE");
    drawGroup (g, utilGroup, "UTILITIES");

    drawKnobLabels (g);

    g.setColour (th.sub);
    g.setFont (juce::Font (juce::FontOptions (9.5f)));
    g.drawText ("Criador e Desenvolvedor  Ricardo Amorim / Juninho Piradex      LEVEL-X  v1.0.0",
                getLocalBounds().removeFromBottom (16), juce::Justification::centred);
}

//==============================================================================
void PiradexMVAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    headerRect = area.removeFromTop (72);
    area.removeFromBottom (16);                 // footer
    auto bar = area.removeFromBottom (70);      // bottom bar
    area.reduce (14, 10);

    // ---- header controls ----
    {
        auto band = headerRect.reduced (18, 0).withSizeKeepingCentre (headerRect.getWidth()-36, 30);
        band.removeFromLeft (300);              // título
        band.removeFromRight (165);             // logo
        band.removeFromRight (6);
        gearBtn.setBounds (band.removeFromRight (30)); band.removeFromRight (8);
        bBtn.setBounds (band.removeFromRight (28));
        aBtn.setBounds (band.removeFromRight (28)); band.removeFromRight (10);
        redoBtn.setBounds (band.removeFromRight (24));
        undoBtn.setBounds (band.removeFromRight (24)); band.removeFromRight (12);
        prevBtn.setBounds (band.removeFromLeft (26));
        nextBtn.setBounds (band.removeFromRight (26));
        presetNameArea = band;
    }

    // ---- corpo ----
    const int g = 10;
    inCol     = area.removeFromLeft (125); area.removeFromLeft (g);
    lowPanel  = area.removeFromLeft (345); area.removeFromLeft (g);
    centerCol = area.removeFromLeft (120); area.removeFromLeft (g);
    highPanel = area.removeFromLeft (345); area.removeFromLeft (g);
    outCol    = area;

    auto placeKnob = [] (juce::Slider& s, juce::Rectangle<int> cell, int reserve = 38)
    {
        int d = juce::jmin (cell.getWidth(), cell.getHeight() - reserve);
        d = juce::jlimit (44, 96, d);
        auto sq = juce::Rectangle<int>(0,0,d,d).withCentre ({ cell.getCentreX(), cell.getY() + d/2 + 4 });
        s.setBounds (sq);
    };

    // IN col
    {
        auto c = inCol;
        c.removeFromTop (40);
        auto knobArea = c.removeFromBottom (96);
        auto mArea = c.withTrimmedBottom (14);
        inMeter.setBounds (mArea.reduced (4, 2));
        placeKnob (inputTrimK, knobArea);
    }
    // OUT col
    {
        auto c = outCol;
        c.removeFromTop (40);
        auto knobArea = c.removeFromBottom (96);
        auto mArea = c.withTrimmedBottom (14);
        outMeter.setBounds (mArea.reduced (4, 2));
        placeKnob (outputK, knobArea);
    }

    // LOW panel
    {
        auto cont = lowPanel.reduced (12); cont.removeFromTop (36);
        auto fcol = cont.removeFromLeft (70);
        lowFader.setBounds (fcol.reduced (12, 6));
        cont.removeFromLeft (6);
        placeKnob (lowThreshK,  cont.removeFromTop (cont.getHeight()/2));
        placeKnob (lowMaxGainK, cont);
    }
    // HIGH panel
    {
        auto cont = highPanel.reduced (12); cont.removeFromTop (36);
        auto fcol = cont.removeFromLeft (70);
        highFader.setBounds (fcol.reduced (12, 6));
        cont.removeFromLeft (6);
        placeKnob (highThreshK, cont.removeFromTop (cont.getHeight()/2));
        placeKnob (highRatioK,  cont);
    }
    // center
    {
        auto c = centerCol; c.removeFromTop (6);
        placeKnob (trimK, c.removeFromTop (c.getHeight()/2));
        placeKnob (mixK,  c);
    }

    // ---- bottom bar ----
    bar.reduce (14, 8);
    respGroup = bar.removeFromLeft (190); bar.removeFromLeft (10);
    osGroup   = bar.removeFromLeft (220); bar.removeFromLeft (10);
    modeGroup = bar.removeFromLeft (250); bar.removeFromLeft (10);
    utilGroup = bar;

    auto segIn = [] (juce::Rectangle<int> grp){ grp.removeFromTop (17); return grp.reduced (10, 6).withSizeKeepingCentre (grp.getWidth()-20, 30); };
    respSeg.setBounds (segIn (respGroup));
    osSeg.setBounds   (segIn (osGroup));
    modeSeg.setBounds (segIn (modeGroup));

    {
        auto u = utilGroup; u.removeFromTop (16); u = u.reduced (8, 4);
        int bw = (u.getWidth() - 3*6) / 4;
        bypassBtn.setBounds (u.removeFromLeft (bw)); u.removeFromLeft (6);
        muteBtn.setBounds   (u.removeFromLeft (bw)); u.removeFromLeft (6);
        dimBtn.setBounds    (u.removeFromLeft (bw)); u.removeFromLeft (6);
        monoBtn.setBounds   (u.removeFromLeft (bw));
    }
}
