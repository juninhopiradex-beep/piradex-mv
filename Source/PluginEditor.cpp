#include "PluginEditor.h"

//==============================================================================
static juce::String dbText (double v)
{
    if (v <= -59.5) return "-inf";
    return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB";
}

//==============================================================================
PiradexMVAudioProcessorEditor::PiradexMVAudioProcessorEditor (PiradexMVAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    auto setupFader = [this] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 20);
        s.setTextValueSuffix ("");
        s.textFromValueFunction = [] (double v) { return dbText (v); };
        s.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
        addAndMakeVisible (s);
    };
    setupFader (lowFader);
    setupFader (highFader);

    auto setupKnob = [this] (juce::Slider& s, bool db)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
        if (db) s.textFromValueFunction = [] (double v) { return dbText (v); };
        else    s.textFromValueFunction = [] (double v) { return juce::String ((int) v) + " %"; };
        addAndMakeVisible (s);
    };
    setupKnob (outputKnob, true);
    setupKnob (mixKnob,  false);
    setupKnob (trimKnob, true);

    for (auto* b : { &muteBtn, &dimBtn, &monoBtn, &bypassBtn })
        addAndMakeVisible (b);

    osSeg.setItems   ({ "1x","2x","4x","8x" });
    respSeg.setItems ({ "FAST","MED","SLOW" });
    modeSeg.setItems ({ "STEREO","DUAL","M/S" });
    addAndMakeVisible (osSeg);
    addAndMakeVisible (respSeg);
    addAndMakeVisible (modeSeg);

    auto& v = proc.apvts;
    lowA  = std::make_unique<SA>(v, "low",    lowFader);
    highA = std::make_unique<SA>(v, "high",   highFader);
    outA  = std::make_unique<SA>(v, "output", outputKnob);
    mixA  = std::make_unique<SA>(v, "mix",    mixKnob);
    trimA = std::make_unique<SA>(v, "trim",   trimKnob);
    muteA   = std::make_unique<BA>(v, "mute",   muteBtn);
    dimA    = std::make_unique<BA>(v, "dim",    dimBtn);
    monoA   = std::make_unique<BA>(v, "mono",   monoBtn);
    bypassA = std::make_unique<BA>(v, "bypass", bypassBtn);

    bindChoice (osSeg,   "os");
    bindChoice (respSeg, "response");
    bindChoice (modeSeg, "mode");

    setSize (780, 540);
    startTimerHz (30);
}

PiradexMVAudioProcessorEditor::~PiradexMVAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void PiradexMVAudioProcessorEditor::bindChoice (SegmentedControl& seg, const juce::String& paramID)
{
    if (auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (paramID)))
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
void PiradexMVAudioProcessorEditor::timerCallback()
{
    auto norm = [] (float peak)
    {
        float db = juce::Decibels::gainToDecibels (peak + 1.0e-6f);
        return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f); // -60..+6 dB
    };
    float tL = norm (proc.outPeakL.load());
    float tR = norm (proc.outPeakR.load());
    mL += (tL - mL) * (tL > mL ? 0.6f : 0.12f);
    mR += (tR - mR) * (tR > mR ? 0.6f : 0.12f);

    float gl = proc.grLow.load(), gh = proc.grHigh.load();
    grLowS  += (gl - grLowS)  * 0.3f;
    grHighS += (gh - grHighS) * 0.3f;

    // sincroniza segmentos com automação do host
    auto sync = [this] (SegmentedControl& s, const juce::String& id)
    {
        if (auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
            if (pc->getIndex() != s.getIndex()) s.setIndex (pc->getIndex());
    };
    sync (osSeg, "os"); sync (respSeg, "response"); sync (modeSeg, "mode");

    repaint();
}

//==============================================================================
void PiradexMVAudioProcessorEditor::drawDiamond (juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::Path d;
    d.startNewSubPath (r.getCentreX(), r.getY());
    d.lineTo (r.getRight(), r.getCentreY());
    d.lineTo (r.getCentreX(), r.getBottom());
    d.lineTo (r.getX(), r.getCentreY());
    d.closeSubPath();

    juce::ColourGradient grad (MVColours::accentHi, r.getX(), r.getY(),
                               MVColours::accentDeep, r.getRight(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillPath (d);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.strokePath (d, juce::PathStrokeType (1.2f));
}

void PiradexMVAudioProcessorEditor::drawHeader (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto a = area.toFloat();
    juce::ColourGradient hg (MVColours::panelHi, a.getX(), a.getY(),
                             MVColours::panel, a.getX(), a.getBottom(), false);
    g.setGradientFill (hg);
    g.fillRect (a);
    g.setColour (MVColours::accent.withAlpha (0.55f));
    g.fillRect (a.getX(), a.getBottom()-2.0f, a.getWidth(), 2.0f);

    auto row = area.reduced (16, 0);
    auto diamond = row.removeFromLeft (34).toFloat().withSizeKeepingCentre (22, 22);
    drawDiamond (g, diamond);

    auto titleArea = row.removeFromLeft (260);
    g.setColour (MVColours::text);
    g.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
    g.drawText ("LEVEL-X", titleArea.removeFromTop ((int) (area.getHeight()*0.62f)).withTrimmedTop (8),
                juce::Justification::bottomLeft);
    g.setColour (MVColours::accentHi);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("by Piradex", titleArea, juce::Justification::topLeft);

    // navegador de preset "Modern Master"
    auto nav = row.removeFromRight (240).reduced (0, 12).toFloat();
    g.setColour (MVColours::slot);
    g.fillRoundedRectangle (nav, 6.0f);
    g.setColour (MVColours::stroke);
    g.drawRoundedRectangle (nav, 6.0f, 1.0f);
    g.setColour (MVColours::accentHi);
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.drawText ("Modern Master", nav, juce::Justification::centred);
    g.setColour (MVColours::textDim);
    g.setFont (juce::Font (juce::FontOptions (16.0f)));
    g.drawText ("<", nav.removeFromLeft (22), juce::Justification::centred);
    g.drawText (">", nav.removeFromRight (22), juce::Justification::centred);
}

//==============================================================================
void PiradexMVAudioProcessorEditor::drawFaderColumn (juce::Graphics& g, juce::Rectangle<int> col,
                                                     const juce::String& title, float grDb)
{
    auto a = col.toFloat();
    g.setColour (MVColours::panelHi.withAlpha (0.5f));
    g.fillRoundedRectangle (a, 8.0f);
    g.setColour (MVColours::stroke);
    g.drawRoundedRectangle (a, 8.0f, 1.0f);

    g.setColour (MVColours::accentHi);
    g.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
    g.drawText (title, col.removeFromTop (26), juce::Justification::centred);

    g.setColour (MVColours::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    juce::String gr = (std::abs (grDb) < 0.1f) ? "0.0" : juce::String (grDb, 1);
    g.drawText (gr + " dB", col.removeFromBottom (16), juce::Justification::centred);
}

void PiradexMVAudioProcessorEditor::drawMeters (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto a = area.toFloat();
    g.setColour (MVColours::slot);
    g.fillRoundedRectangle (a, 6.0f);
    g.setColour (MVColours::stroke);
    g.drawRoundedRectangle (a, 6.0f, 1.0f);

    auto inner = a.reduced (6.0f);
    const float gap = 5.0f;
    float barW = (inner.getWidth() - gap) * 0.5f;
    auto drawBar = [&] (juce::Rectangle<float> r, float level)
    {
        // escala arco-íris: verde -> amarelo -> laranja -> vermelho
        juce::ColourGradient grad (juce::Colour (0xff2ecc71), r.getX(), r.getBottom(),
                                   juce::Colour (0xffe74c3c), r.getX(), r.getY(), false);
        grad.addColour (0.55, juce::Colour (0xfff1c40f));
        grad.addColour (0.80, juce::Colour (0xffe67e22));
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (r, 3.0f);
        float h = r.getHeight() * juce::jlimit (0.0f, 1.0f, level);
        juce::Rectangle<float> fill (r.getX(), r.getBottom() - h, r.getWidth(), h);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, 3.0f);
    };
    drawBar (inner.removeFromLeft (barW), mL);
    inner.removeFromLeft (gap);
    drawBar (inner, mR);
}

//==============================================================================
void PiradexMVAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (MVColours::panel);

    auto area = getLocalBounds();
    drawHeader (g, area.removeFromTop (64));

    drawFaderColumn (g, lowCol,  "LOW",  grLowS);
    drawFaderColumn (g, highCol, "HIGH", grHighS);
    drawMeters (g, meterArea);

    // rótulos dos selectores + knobs
    g.setColour (MVColours::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    auto lbl = [&] (juce::Component& c, const juce::String& t)
    {
        auto r = c.getBounds();
        g.drawText (t, r.getX(), r.getY()-15, r.getWidth(), 13, juce::Justification::centred);
    };
    lbl (osSeg,   "OVERSAMPLING");
    lbl (respSeg, "RESPONSE");
    lbl (modeSeg, "OUTPUT MODE");
    lbl (outputKnob, "OUTPUT");
    lbl (mixKnob,  "MIX");
    lbl (trimKnob, "TRIM");

    g.setColour (MVColours::textDim.withAlpha (0.8f));
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.drawText ("Criador e Desenvolvedor  Ricardo Amorim / Juninho Piradex   -   v1.0.0",
                getLocalBounds().removeFromBottom (18), juce::Justification::centred);
}

//==============================================================================
void PiradexMVAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (64);        // header
    area.removeFromBottom (18);     // footer
    area.reduce (16, 14);

    // esquerda: dois faders + meters
    auto left = area.removeFromLeft (320);
    {
        auto colW = 96;
        lowCol  = left.removeFromLeft (colW);
        left.removeFromLeft (10);
        highCol = left.removeFromLeft (colW);
        left.removeFromLeft (10);
        meterArea = left.removeFromLeft (40).reduced (0, 26);

        auto fL = lowCol.reduced (8); fL.removeFromTop (26); fL.removeFromBottom (16);
        lowFader.setBounds (fL);
        auto fH = highCol.reduced (8); fH.removeFromTop (26); fH.removeFromBottom (16);
        highFader.setBounds (fH);
    }

    area.removeFromLeft (20);

    // direita: selectores (topo), knobs (meio), botões (fundo)
    auto right = area;

    auto segRow = right.removeFromTop (46);
    segRow.removeFromTop (16);
    auto segH = 30;
    osSeg.setBounds   (segRow.removeFromLeft (150).withHeight (segH));
    modeSeg.setBounds (segRow.removeFromRight (170).withHeight (segH));
    respSeg.setBounds (segRow.reduced (12, 0).withHeight (segH));

    right.removeFromTop (28);

    auto knobRow = right.removeFromTop (118);
    auto kw = knobRow.getWidth() / 3;
    outputKnob.setBounds (knobRow.removeFromLeft (kw).reduced (6));
    mixKnob.setBounds    (knobRow.removeFromLeft (kw).reduced (6));
    trimKnob.setBounds   (knobRow.reduced (6));

    right.removeFromTop (20);

    auto btnRow = right.removeFromTop (44);
    auto bw = (btnRow.getWidth() - 3*8) / 4;
    muteBtn.setBounds   (btnRow.removeFromLeft (bw)); btnRow.removeFromLeft (8);
    dimBtn.setBounds    (btnRow.removeFromLeft (bw)); btnRow.removeFromLeft (8);
    monoBtn.setBounds   (btnRow.removeFromLeft (bw)); btnRow.removeFromLeft (8);
    bypassBtn.setBounds (btnRow.removeFromLeft (bw));
}
