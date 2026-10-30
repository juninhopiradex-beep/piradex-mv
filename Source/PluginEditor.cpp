#include "PluginEditor.h"

//==============================================================================
namespace fmt
{
    static juce::String dbpm (double v) { return (v>0?"+":"") + juce::String (v,1) + " dB"; }
    static juce::String db   (double v) { return juce::String (v,1) + " dB"; }
    static juce::String gain (double v) { return "+" + juce::String (v,1) + " dB"; }
    static juce::String ratio(double v) { return juce::String (v,1) + ":1"; }
    static juce::String pct  (double v) { return juce::String ((int) v) + " %"; }
    static juce::String hz   (double v) { return v < 1.0 ? juce::String ("off") : juce::String ((int) v) + " Hz"; }
    static juce::String ms   (double v) { return juce::String (v,1) + " ms"; }
    static juce::String msI  (double v) { return juce::String ((int) v) + " ms"; }
    static juce::String dbtp (double v) { return juce::String (v,1) + " dBTP"; }
}

//==============================================================================
static std::vector<std::pair<juce::String, std::map<juce::String,float>>> makePresets()
{
    using M = std::map<juce::String,float>;
    std::vector<std::pair<juce::String, M>> v;
    v.push_back ({ "PRIME Mastering", M{{"inputTrim",0},{"lowAmt",62},{"lowThresh",-34},{"lowMaxGain",8}, {"trim",0},{"mix",100},{"highAmt",75},{"highThresh",-3},{"highRatio",6},{"output",0},{"os",5},{"response",0},{"mode",0}} });
    v.push_back ({ "Vocal Lift",      M{{"inputTrim",0},{"lowAmt",70},{"lowThresh",-28},{"lowMaxGain",12},{"trim",0},{"mix",100},{"highAmt",60},{"highThresh",-6},{"highRatio",4},{"output",-1},{"os",1},{"response",1},{"mode",0}} });
    v.push_back ({ "Vocal Smooth",    M{{"inputTrim",0},{"lowAmt",55},{"lowThresh",-30},{"lowMaxGain",10},{"trim",0},{"mix",100},{"highAmt",45},{"highThresh",-8},{"highRatio",3},{"output",-1},{"os",0},{"response",1},{"mode",0}} });
    v.push_back ({ "Vocal Presence",  M{{"inputTrim",0},{"lowAmt",80},{"lowThresh",-26},{"lowMaxGain",14},{"trim",0},{"mix",100},{"highAmt",55},{"highThresh",-5},{"highRatio",5},{"output",-1},{"os",1},{"response",0},{"mode",0}} });
    v.push_back ({ "Master Control",  M{{"inputTrim",0},{"lowAmt",40},{"lowThresh",-34},{"lowMaxGain",8}, {"trim",0},{"mix",100},{"highAmt",75},{"highThresh",-3},{"highRatio",6},{"output",0},{"os",1},{"response",2},{"mode",0}} });
    v.push_back ({ "Master Glue",     M{{"inputTrim",0},{"lowAmt",35},{"lowThresh",-38},{"lowMaxGain",6}, {"trim",0},{"mix",80}, {"highAmt",55},{"highThresh",-4},{"highRatio",4},{"output",-0.5f},{"os",2},{"response",2},{"mode",2}} });
    v.push_back ({ "Bus Transparent", M{{"inputTrim",0},{"lowAmt",30},{"lowThresh",-40},{"lowMaxGain",6}, {"trim",0},{"mix",100},{"highAmt",50},{"highThresh",-2},{"highRatio",4},{"output",0},{"os",2},{"response",2},{"mode",0}} });
    v.push_back ({ "Drum Glue",       M{{"inputTrim",0},{"lowAmt",50},{"lowThresh",-24},{"lowMaxGain",9}, {"trim",0},{"mix",100},{"highAmt",65},{"highThresh",-5},{"highRatio",8},{"output",-1},{"os",1},{"response",0},{"mode",1}} });
    v.push_back ({ "Drum Punch",      M{{"inputTrim",0},{"lowAmt",35},{"lowThresh",-22},{"lowMaxGain",7}, {"trim",0},{"mix",100},{"highAmt",80},{"highThresh",-6},{"highRatio",10},{"output",-1},{"os",2},{"response",0},{"mode",0}} });
    v.push_back ({ "Bass Tight",      M{{"inputTrim",0},{"lowAmt",45},{"lowThresh",-26},{"lowMaxGain",8}, {"trim",0},{"mix",100},{"highAmt",70},{"highThresh",-4},{"highRatio",6},{"output",-1},{"os",1},{"response",1},{"mode",1}} });
    v.push_back ({ "Acoustic Open",   M{{"inputTrim",0},{"lowAmt",65},{"lowThresh",-32},{"lowMaxGain",12},{"trim",0},{"mix",100},{"highAmt",40},{"highThresh",-8},{"highRatio",3},{"output",-1},{"os",1},{"response",1},{"mode",0}} });
    v.push_back ({ "Wide Side",       M{{"inputTrim",0},{"lowAmt",55},{"lowThresh",-30},{"lowMaxGain",10},{"trim",0},{"mix",100},{"highAmt",45},{"highThresh",-6},{"highRatio",4},{"output",-1},{"os",1},{"response",1},{"mode",2}} });
    v.push_back ({ "Loud & Proud",    M{{"inputTrim",0},{"lowAmt",60},{"lowThresh",-28},{"lowMaxGain",14},{"trim",0},{"mix",100},{"highAmt",85},{"highThresh",-3},{"highRatio",12},{"output",0},{"os",2},{"response",0},{"mode",0}} });
    return v;
}
static auto presetTable = makePresets();

//==============================================================================
LevelXUI::LevelXUI (PiradexMVAudioProcessor& p)
    : proc (p)
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
    for (auto* k : { &inputTrimK,&lowThreshK,&lowMaxGainK,&trimK,&mixK,&highThreshK,&highRatioK,&outputK,
                     &scHPK,&kneeK,&attackK,&releaseK,&ceilingK,&lookaheadK })
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
    osSeg.setItems   ({ "1x","2x","4x","8x","16x","32x" });
    modeSeg.setItems ({ "STEREO","DUAL MONO","MID-SIDE" });

    for (auto* b : { &bypassBtn,&muteBtn,&dimBtn,&monoBtn,&midBtn,&sideBtn }) addAndMakeVisible (*b);
    for (auto* b : { &limiterBtn,&ditherBtn,&autoGainBtn,&deltaBtn }) addAndMakeVisible (*b);
    for (auto* b : { &prevBtn,&nextBtn,&undoBtn,&redoBtn,&aBtn,&bBtn,&cBtn,&dBtn,&gearBtn }) addAndMakeVisible (*b);
    addAndMakeVisible (inMeter); addAndMakeVisible (outMeter); addAndMakeVisible (analyser);

    // attachments (+ duplo-clique repõe o default)
    auto& v = proc.apvts;
    auto SAmk = [&] (const char* id, juce::Slider& s){
        sAtt.push_back (std::make_unique<SA>(v,id,s));
        if (auto* p = v.getParameter (id)) s.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    };
    SAmk ("inputTrim", inputTrimK); SAmk ("lowThresh", lowThreshK); SAmk ("lowMaxGain", lowMaxGainK);
    SAmk ("trim", trimK); SAmk ("mix", mixK); SAmk ("highThresh", highThreshK);
    SAmk ("highRatio", highRatioK); SAmk ("output", outputK);
    SAmk ("lowAmt", lowFader); SAmk ("highAmt", highFader);
    SAmk ("scHP", scHPK); SAmk ("knee", kneeK); SAmk ("attack", attackK);
    SAmk ("release", releaseK); SAmk ("ceiling", ceilingK); SAmk ("lookahead", lookaheadK);
    auto BAmk = [&] (const char* id, juce::Button& b){ bAtt.push_back (std::make_unique<BA>(v,id,b)); };
    BAmk ("bypass", bypassBtn); BAmk ("mute", muteBtn); BAmk ("dim", dimBtn); BAmk ("mono", monoBtn);
    BAmk ("monMid", midBtn); BAmk ("monSide", sideBtn);
    BAmk ("limiter", limiterBtn); BAmk ("dither", ditherBtn);
    BAmk ("autoGain", autoGainBtn); BAmk ("delta", deltaBtn);

    // MID / SIDE / MONO mutuamente exclusivos
    auto excl = [this] (juce::Button* keep) {
        for (auto* b : { &monoBtn, &midBtn, &sideBtn }) if (b != keep && b->getToggleState()) b->setToggleState (false, juce::sendNotification);
    };
    monoBtn.onClick = [this,excl]{ if (monoBtn.getToggleState()) excl (&monoBtn); };
    midBtn .onClick = [this,excl]{ if (midBtn .getToggleState()) excl (&midBtn ); };
    sideBtn.onClick = [this,excl]{ if (sideBtn.getToggleState()) excl (&sideBtn); };

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
        { &scHPK,      "SC HP",      fmt::hz,   "", ""   },
        { &kneeK,      "KNEE",       fmt::db,   "", ""   },
        { &attackK,    "ATTACK",     fmt::ms,   "", ""   },
        { &releaseK,   "RELEASE",    fmt::msI,  "", ""   },
        { &ceilingK,   "CEILING",    fmt::dbtp, "", ""   },
        { &lookaheadK, "LOOKAHEAD",  fmt::ms,   "", ""   },
    };

    // presets
    for (auto& pr : presetTable) presetNames.add (pr.first);

    // header buttons
    prevBtn.onClick = [this]{ applyPreset ((currentPreset - 1 + (int)presetTable.size()) % (int)presetTable.size()); };
    nextBtn.onClick = [this]{ applyPreset ((currentPreset + 1) % (int)presetTable.size()); };
    aBtn.onClick = [this]{ toggleAB (0); };
    bBtn.onClick = [this]{ toggleAB (1); };
    cBtn.onClick = [this]{ toggleAB (2); };
    dBtn.onClick = [this]{ toggleAB (3); };
    gearBtn.onClick = [this]
    {
        th = th.dark ? Theme::lightTheme() : Theme::darkTheme();
        proc.apvts.state.setProperty ("uiDark", th.dark, nullptr);
        gearBtn.active = !th.dark;
        applyThemeToAll();
    };

    aBtn.active = true;
    { auto s = captureState(); for (auto& sl : abSlots) sl = s; }

    applyThemeToAll();
    setSize (1260, 904);
    startTimerHz (30);
}

LevelXUI::~LevelXUI() { setLookAndFeel (nullptr); }

//==============================================================================
PiradexMVAudioProcessorEditor::PiradexMVAudioProcessorEditor (PiradexMVAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), ui (p)
{
    addAndMakeVisible (ui);
    login = lvlx::makeLoginOverlay ([this] { if (login) login->setVisible (false); });
    addChildComponent (*login);
    login->setVisible (! lic::License::get().isUnlocked());
    startTimerHz (4);
    setResizable (true, true);
    setResizeLimits ((int) (baseW*0.7), (int) (baseH*0.7), (int) (baseW*1.7), (int) (baseH*1.7));
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) baseW / (double) baseH);
    int w = juce::jlimit ((int)(baseW*0.7), (int)(baseW*1.7),
                          (int) proc.apvts.state.getProperty ("uiW", baseW));
    setSize (w, juce::roundToInt (w * (double) baseH / baseW));
}

void PiradexMVAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / (float) baseW;
    ui.setTransform (juce::AffineTransform::scale (s));
    ui.setBounds (0, 0, baseW, baseH);
    if (login) { login->setBounds (getLocalBounds()); if (login->isVisible()) login->toFront (false); }
    proc.apvts.state.setProperty ("uiW", getWidth(), nullptr);
}

void PiradexMVAudioProcessorEditor::timerCallback()
{
    lic::License::get().checkExpiry();
    const bool locked = ! lic::License::get().isUnlocked();
    if (login && locked != login->isVisible())
    {
        login->setVisible (locked);
        if (locked) { login->setBounds (getLocalBounds()); login->toFront (false); }
    }
}

//==============================================================================
void LevelXUI::applyThemeToAll()
{
    lnf.setTheme (th);
    setLookAndFeel (&lnf);

    for (auto* c : std::initializer_list<Themed*>{ &respSeg,&osSeg,&modeSeg,
            &bypassBtn,&muteBtn,&dimBtn,&monoBtn,&midBtn,&sideBtn,
            &limiterBtn,&ditherBtn,&autoGainBtn,&deltaBtn,
            &prevBtn,&nextBtn,&undoBtn,&redoBtn,&aBtn,&bBtn,&cBtn,&dBtn,&gearBtn,&inMeter,&outMeter,&analyser })
        c->th = &th;

    auto setAccent = [] (juce::Slider& s, juce::Colour col){ s.getProperties().set ("accent", (int) col.getARGB()); };
    setAccent (lowFader, th.blue);   setAccent (highFader, th.amber);
    setAccent (lowThreshK, th.blue); setAccent (lowMaxGainK, th.blue);
    setAccent (highThreshK, th.amber); setAccent (highRatioK, th.amber);
    const juce::Colour centreWhite { 0xffe9eef4 };               // knobs do meio (TRIM/MIX)
    setAccent (inputTrimK, th.blue); setAccent (trimK, centreWhite);
    setAccent (mixK, centreWhite);   setAccent (outputK, th.blue);
    setAccent (scHPK, th.blue); setAccent (kneeK, th.blue); setAccent (attackK, th.blue);
    setAccent (releaseK, th.blue); setAccent (ceilingK, th.amber); setAccent (lookaheadK, th.amber);

    for (auto* s : { &inputTrimK,&lowThreshK,&lowMaxGainK,&trimK,&mixK,&highThreshK,&highRatioK,&outputK,&lowFader,&highFader,
                     &scHPK,&kneeK,&attackK,&releaseK,&ceilingK,&lookaheadK })
        s->setLookAndFeel (&lnf);

    repaint();
}

void LevelXUI::bindChoice (SegBar& seg, const juce::String& id)
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
std::map<juce::String,float> LevelXUI::captureState()
{
    std::map<juce::String,float> m;
    for (auto& id : PiradexMVAudioProcessor::paramIDs())
        if (auto* rp = proc.apvts.getRawParameterValue (id)) m[id] = rp->load();
    return m;
}

void LevelXUI::loadState (const std::map<juce::String,float>& m)
{
    for (auto& kv : m)
        if (auto* p = proc.apvts.getParameter (kv.first))
            p->setValueNotifyingHost (p->convertTo0to1 (kv.second));
    respSeg.setIndex ((int) proc.apvts.getRawParameterValue ("response")->load());
    osSeg.setIndex   ((int) proc.apvts.getRawParameterValue ("os")->load());
    modeSeg.setIndex ((int) proc.apvts.getRawParameterValue ("mode")->load());
}

void LevelXUI::applyPreset (int idx)
{
    idx = juce::jlimit (0, (int) presetTable.size()-1, idx);
    currentPreset = idx;
    loadState (presetTable[(size_t) idx].second);
    repaint();
}

void LevelXUI::toggleAB (int slot)
{
    slot = juce::jlimit (0, 3, slot);
    if (slot == abIndex) return;
    abSlots[abIndex] = captureState();     // guarda o atual
    abIndex = slot;
    loadState (abSlots[abIndex]);
    aBtn.active = (abIndex==0); bBtn.active = (abIndex==1);
    cBtn.active = (abIndex==2); dBtn.active = (abIndex==3);
    repaint();
}

juce::File LevelXUI::userPresetDir() const
{
    auto d = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                .getChildFile ("Piradex").getChildFile ("LEVEL-X").getChildFile ("Presets");
    d.createDirectory();
    return d;
}

void LevelXUI::saveStateToFile (const juce::File& f)
{
    if (auto xml = proc.apvts.copyState().createXml())
        xml->writeTo (f.withFileExtension ("levelx"));
}

void LevelXUI::loadStateFromFile (const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse (f))
    {
        proc.apvts.replaceState (juce::ValueTree::fromXml (*xml));
        respSeg.setIndex ((int) proc.apvts.getRawParameterValue ("response")->load());
        osSeg.setIndex   ((int) proc.apvts.getRawParameterValue ("os")->load());
        modeSeg.setIndex ((int) proc.apvts.getRawParameterValue ("mode")->load());
        repaint();
    }
}

void LevelXUI::mouseDown (const juce::MouseEvent& e)
{
    if (outCol.contains (e.getPosition())) { proc.clipFlag.store (false); repaint(); return; }
    if (! presetNameArea.contains (e.getPosition())) return;
    juce::PopupMenu m;
    m.setLookAndFeel (&lnf);
    for (int i = 0; i < presetNames.size(); ++i)
        m.addItem (i + 1, presetNames[i], true, i == currentPreset);

    auto files = userPresetDir().findChildFiles (juce::File::findFiles, false, "*.levelx");
    if (! files.isEmpty())
    {
        juce::PopupMenu um;
        for (int i = 0; i < files.size(); ++i) um.addItem (1000 + i, files[i].getFileNameWithoutExtension());
        m.addSeparator(); m.addSubMenu ("User Presets", um);
    }
    m.addSeparator();
    m.addItem (9001, "Save preset...");
    m.addItem (9002, "Load preset...");

    m.showMenuAsync (juce::PopupMenu::Options()
                        .withTargetScreenArea (localAreaToGlobal (presetNameArea)),
        [this, files] (int r)
        {
            if (r <= 0) return;
            if (r >= 1 && r <= presetNames.size()) applyPreset (r - 1);
            else if (r >= 1000 && r < 1000 + files.size()) loadStateFromFile (files[r - 1000]);
            else if (r == 9001)
            {
                fileChooser = std::make_unique<juce::FileChooser> ("Guardar preset LEVEL-X", userPresetDir(), "*.levelx");
                fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this] (const juce::FileChooser& fc){ auto f=fc.getResult(); if (f != juce::File()) saveStateToFile (f); });
            }
            else if (r == 9002)
            {
                fileChooser = std::make_unique<juce::FileChooser> ("Carregar preset LEVEL-X", userPresetDir(), "*.levelx");
                fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this] (const juce::FileChooser& fc){ auto f=fc.getResult(); if (f.existsAsFile()) loadStateFromFile (f); });
            }
        });
}

//==============================================================================
void LevelXUI::timerCallback()
{
    auto norm = [] (float peak){ float db = juce::Decibels::gainToDecibels (peak + 1.0e-6f);
                                 return juce::jlimit (0.0f, 1.0f, (db + 72.0f) / 78.0f); }; // -72..+6
    auto sm = [] (float& s, float t){ s += (t - s) * (t > s ? 0.6f : 0.22f); };            // mais viva
    sm (mInL, norm (proc.inPeakL.load()));  sm (mInR, norm (proc.inPeakR.load()));
    sm (mOutL,norm (proc.outPeakL.load())); sm (mOutR,norm (proc.outPeakR.load()));
    sm (mInM, norm (proc.inPeakM.load()));  sm (mInS, norm (proc.inPeakS.load()));
    sm (mOutM,norm (proc.outPeakM.load())); sm (mOutS,norm (proc.outPeakS.load()));

    analyser.update();

    // meters mostram L/R ou M/S conforme o Output Mode
    const int mode = (int) proc.apvts.getRawParameterValue ("mode")->load();
    if (mode == 2) // Mid/Side
    {
        meterLblL = "M"; meterLblR = "S";
        inMeter.setLevels (mInM, mInS);
        outMeter.setLevels (mOutM, mOutS);
    }
    else
    {
        meterLblL = "L"; meterLblR = "R";
        inMeter.setLevels (mInL, mInR);
        outMeter.setLevels (mOutL, mOutR);
    }
    repaint();
}

//==============================================================================
void LevelXUI::drawBeatfreakLogo (juce::Graphics& g, juce::Rectangle<float> r)
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

void LevelXUI::drawHeader (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto a = area.toFloat();
    juce::ColourGradient hg (th.header.brighter (0.04f), a.getX(), a.getY(),
                             th.header, a.getX(), a.getBottom(), false);
    g.setGradientFill (hg); g.fillRect (a);
    g.setColour (th.blue.withAlpha (0.5f));
    g.fillRect (a.getX(), a.getBottom()-1.5f, a.getWidth(), 1.5f);

    auto hr = area.reduced (18, 0);
    // título "LEVEL-X by Piradex" (juntos)
    auto title = hr.removeFromLeft (330);
    auto nameRow = title.removeFromTop (44).withTrimmedTop (8);
    juce::Font f26 (juce::FontOptions (28.0f, juce::Font::bold));
    float wname = f26.getStringWidthFloat ("LEVEL-X");
    g.setColour (th.text); g.setFont (f26);
    g.drawText ("LEVEL-X", nameRow.withWidth ((int) wname + 4), juce::Justification::bottomLeft);
    g.setColour (th.blue);
    g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
    g.drawText ("by Piradex", nameRow.withTrimmedLeft ((int) wname + 12), juce::Justification::bottomLeft);
    g.setColour (th.sub);
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    g.drawText ("DUAL-LEVEL DYNAMIC PROCESSOR", title, juce::Justification::topLeft);

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
void LevelXUI::drawPanel (juce::Graphics& g, juce::Rectangle<int> rr,
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

void LevelXUI::drawGroup (juce::Graphics& g, juce::Rectangle<int> rr, const juce::String& title)
{
    auto a = rr.toFloat();
    g.setColour (th.panel);  g.fillRoundedRectangle (a, 7.0f);
    g.setColour (th.stroke); g.drawRoundedRectangle (a, 7.0f, 1.0f);
    g.setColour (th.textDim);
    g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    g.drawText (title, rr.removeFromTop (17), juce::Justification::centred);
}

void LevelXUI::drawKnobLabels (juce::Graphics& g)
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
void LevelXUI::paint (juce::Graphics& g)
{
    g.fillAll (th.bg);
    // brilho radial subtil no topo (Obsidian)
    {
        auto top = getLocalBounds().toFloat();
        juce::ColourGradient bgg (th.panelHi.withAlpha (th.dark ? 0.55f : 0.0f),
                                  top.getCentreX(), -120.0f,
                                  th.bg.withAlpha (0.0f), top.getCentreX(), top.getHeight()*0.6f, true);
        g.setGradientFill (bgg); g.fillRect (top);
    }
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
    const bool ms = (int) proc.apvts.getRawParameterValue ("mode")->load() == 2;
    meterCol (inCol,  "IN",  ms?proc.inPeakM.load():proc.inPeakL.load(),  ms?proc.inPeakS.load():proc.inPeakR.load());
    meterCol (outCol, "OUT", ms?proc.outPeakM.load():proc.outPeakL.load(), ms?proc.outPeakS.load():proc.outPeakR.load());

    // L/R (ou M/S) por baixo dos meters
    auto lr = [&] (juce::Rectangle<int> col){
        g.setColour (th.sub); g.setFont (juce::Font (juce::FontOptions (8.0f, juce::Font::bold)));
        auto row = col.withTop (inMeter.getBottom()).removeFromTop (12);
        g.drawText (meterLblL + "        " + meterLblR, row, juce::Justification::centred);
    };
    lr (inCol); lr (outCol);

    // indicador de clip (OUT) — clicar na coluna OUT reinicia
    if (proc.clipFlag.load())
    {
        g.setColour (th.meterHi);
        g.fillRoundedRectangle ((float) outCol.getCentreX()-20, (float) outCol.getY()+2, 40.0f, 13.0f, 3.0f);
        g.setColour (juce::Colours::white); g.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
        g.drawText ("CLIP", outCol.getX(), outCol.getY()+2, outCol.getWidth(), 13, juce::Justification::centred);
    }

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

    drawGroup (g, advRect,   "ADVANCED");
    drawGroup (g, respGroup, "RESPONSE");
    drawGroup (g, osGroup,   "OVERSAMPLING");
    drawGroup (g, modeGroup, "STEREO MODE");
    drawGroup (g, utilGroup, "UTILITIES");

    drawKnobLabels (g);

    // valor dos faders (AMOUNT %)
    auto faderVal = [&] (juce::Slider& f)
    {
        auto b = f.getBounds(); int cx = b.getCentreX();
        g.setColour (th.textDim);
        g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        g.drawText ("AMOUNT", cx-40, b.getBottom()+4, 80, 12, juce::Justification::centred);
        juce::Rectangle<float> vb ((float) cx-28, (float) b.getBottom()+16, 56.0f, 17.0f);
        g.setColour (th.slot);   g.fillRoundedRectangle (vb, 3.5f);
        g.setColour (th.stroke); g.drawRoundedRectangle (vb, 3.5f, 1.0f);
        g.setColour (th.text);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        g.drawText (juce::String ((int) f.getValue()) + " %", vb, juce::Justification::centred);
    };
    faderVal (lowFader); faderVal (highFader);

    g.setColour (th.sub);
    g.setFont (juce::Font (juce::FontOptions (9.5f)));
    g.drawText ("Criador e Desenvolvedor  Ricardo Amorim / Juninho Piradex      LEVEL-X  v1.0.0",
                getLocalBounds().removeFromBottom (16), juce::Justification::centred);
}

//==============================================================================
void LevelXUI::resized()
{
    auto area = getLocalBounds();
    headerRect = area.removeFromTop (72);
    area.removeFromBottom (16);                 // footer
    auto bar = area.removeFromBottom (70);      // bottom bar
    auto adv = area.removeFromBottom (104);     // secção avançada
    area.reduce (14, 10);

    // ---- header controls ----
    {
        auto band = headerRect.reduced (18, 0).withSizeKeepingCentre (headerRect.getWidth()-36, 30);
        band.removeFromLeft (330);              // título "LEVEL-X by Piradex"
        band.removeFromRight (10);              // margem direita
        band.removeFromRight (6);
        gearBtn.setBounds (band.removeFromRight (30)); band.removeFromRight (8);
        dBtn.setBounds (band.removeFromRight (26));
        cBtn.setBounds (band.removeFromRight (26));
        bBtn.setBounds (band.removeFromRight (26));
        aBtn.setBounds (band.removeFromRight (26)); band.removeFromRight (10);
        redoBtn.setBounds (band.removeFromRight (24));
        undoBtn.setBounds (band.removeFromRight (24)); band.removeFromRight (12);
        prevBtn.setBounds (band.removeFromLeft (26));
        nextBtn.setBounds (band.removeFromRight (26));
        presetNameArea = band;
    }

    // ---- visualizador (topo) ----
    vizRect = area.removeFromTop (196);
    analyser.setBounds (vizRect);
    area.removeFromTop (12);

    // ---- corpo ----
    const int g = 10;
    inCol  = area.removeFromLeft (132);  area.removeFromLeft (g);
    outCol = area.removeFromRight (132); area.removeFromRight (g);
    // 'area' = low | center | high
    const int cw = 140;
    int side = (area.getWidth() - cw - 2*g) / 2;
    lowPanel  = area.removeFromLeft (side); area.removeFromLeft (g);
    centerCol = area.removeFromLeft (cw);   area.removeFromLeft (g);
    highPanel = area;

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
        { auto fc = fcol.reduced (12, 6); fc.removeFromBottom (36); lowFader.setBounds (fc); }
        cont.removeFromLeft (6);
        placeKnob (lowThreshK,  cont.removeFromTop (cont.getHeight()/2));
        placeKnob (lowMaxGainK, cont);
    }
    // HIGH panel
    {
        auto cont = highPanel.reduced (12); cont.removeFromTop (36);
        auto fcol = cont.removeFromLeft (70);
        { auto fc = fcol.reduced (12, 6); fc.removeFromBottom (36); highFader.setBounds (fc); }
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

    // ---- secção avançada ----
    adv.reduce (14, 6);
    advRect = adv;
    {
        auto a2 = adv.reduced (8, 0); a2.removeFromTop (16);   // espaço p/ título
        auto togArea = a2.removeFromRight (330);
        // 6 knobs
        juce::Slider* ak[] = { &scHPK,&kneeK,&attackK,&releaseK,&ceilingK,&lookaheadK };
        int kw = a2.getWidth() / 6;
        for (int i=0;i<6;++i)
        {
            auto cell = a2.removeFromLeft (kw);
            int d = juce::jlimit (36, 52, juce::jmin (cell.getWidth(), cell.getHeight()-36));
            ak[i]->setBounds (juce::Rectangle<int>(0,0,d,d).withCentre ({ cell.getCentreX(), cell.getY()+d/2+2 }));
        }
        // 4 toggles (2x2)
        togArea.reduce (8, 4);
        int tw = (togArea.getWidth()-8)/2, thh = (togArea.getHeight()-8)/2;
        limiterBtn .setBounds (togArea.getX(),          togArea.getY(),           tw, thh);
        ditherBtn  .setBounds (togArea.getX()+tw+8,     togArea.getY(),           tw, thh);
        autoGainBtn.setBounds (togArea.getX(),          togArea.getY()+thh+8,     tw, thh);
        deltaBtn   .setBounds (togArea.getX()+tw+8,     togArea.getY()+thh+8,     tw, thh);
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
        IconToggle* ub[] = { &bypassBtn,&muteBtn,&dimBtn,&monoBtn,&midBtn,&sideBtn };
        int bw = (u.getWidth() - 5*5) / 6;
        for (int i=0;i<6;++i){ ub[i]->setBounds (u.removeFromLeft (bw)); if (i<5) u.removeFromLeft (5); }
    }
}
