#include "PluginEditor.h"

using namespace juce;

//==============================================================================
PiradexMVAudioProcessorEditor::PiradexMVAudioProcessorEditor (PiradexMVAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    auto fader = [this] (Slider& s, const String& suffix)
    {
        s.setSliderStyle (Slider::LinearVertical);
        s.setTextBoxStyle (Slider::TextBoxBelow, false, 70, 22);
        s.setTextValueSuffix (suffix);
        s.setColour (Slider::textBoxTextColourId, MVColours::gold);
        addAndMakeVisible (s);
    };
    fader (lowFader,  " %");
    fader (highFader, " %");

    auto knob = [this] (Slider& s, const String& suffix)
    {
        s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (Slider::TextBoxBelow, false, 72, 22);
        s.setTextValueSuffix (suffix);
        s.setColour (Slider::textBoxTextColourId, MVColours::gold);
        addAndMakeVisible (s);
    };
    knob (outputKnob, " dB");
    knob (mixKnob,    " %");
    knob (trimKnob,   " dB");

    auto toggle = [this] (TextButton& b)
    {
        b.setClickingTogglesState (true);
        addAndMakeVisible (b);
    };
    toggle (lowLink); toggle (highLink);
    toggle (muteBtn); toggle (dimBtn); toggle (monoBtn); toggle (bypassBtn);

    auto combo = [this] (ComboBox& c, const StringArray& items)
    {
        c.addItemList (items, 1);
        c.setJustificationType (Justification::centred);
        addAndMakeVisible (c);
    };
    combo (osBox,   {"1x","2x","4x","8x"});
    combo (respBox, {"Smooth","Balanced","Fast"});
    combo (modeBox, {"Classic","Modern"});

    auto& v = proc.apvts;
    lowA  = std::make_unique<SA>(v, "low",    lowFader);
    highA = std::make_unique<SA>(v, "high",   highFader);
    outA  = std::make_unique<SA>(v, "output", outputKnob);
    mixA  = std::make_unique<SA>(v, "mix",    mixKnob);
    trimA = std::make_unique<SA>(v, "trim",   trimKnob);

    lowLinkA  = std::make_unique<BA>(v, "lowlink",  lowLink);
    highLinkA = std::make_unique<BA>(v, "highlink", highLink);
    muteA   = std::make_unique<BA>(v, "mute",   muteBtn);
    dimA    = std::make_unique<BA>(v, "dim",    dimBtn);
    monoA   = std::make_unique<BA>(v, "mono",   monoBtn);
    bypassA = std::make_unique<BA>(v, "bypass", bypassBtn);

    osA   = std::make_unique<CA>(v, "os",       osBox);
    respA = std::make_unique<CA>(v, "response", respBox);
    modeA = std::make_unique<CA>(v, "mode",     modeBox);

    setSize (720, 540);
    startTimerHz (30);
}

PiradexMVAudioProcessorEditor::~PiradexMVAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void PiradexMVAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (MVColours::bg);

    auto area = getLocalBounds().reduced (10);

    // header
    auto header = area.removeFromTop (48);
    drawHeader (g, header);
    area.removeFromTop (8);

    // bottom bar
    auto bottom = area.removeFromBottom (92);

    // três painéis principais
    auto left   = area.removeFromLeft (230);
    auto right  = area.removeFromRight (210);
    auto centre = area;

    auto panel = [&] (Rectangle<int> r)
    {
        g.setColour (MVColours::panel);
        g.fillRoundedRectangle (r.toFloat(), 10.0f);
        g.setColour (MVColours::stroke);
        g.drawRoundedRectangle (r.toFloat(), 10.0f, 1.0f);
    };
    panel (left);
    panel (centre.reduced (6, 0));
    panel (right);

    // títulos das faixas
    g.setColour (MVColours::textDim);
    g.setFont (Font (12.0f, Font::bold));
    g.drawText ("LOW",  left.removeFromTop (22).withTrimmedLeft (34),  Justification::centredLeft);
    // (HIGH título é desenhado junto da escala)

    // escala + títulos dos faders
    drawFaderScale (g, left);

    // OUTPUT label + logo central
    auto cc = centre.reduced (6, 0);
    drawPiradexLogo (g, Rectangle<float> (cc.getCentreX()-16, cc.getY()+16, 32, 32), MVColours::blue);
    g.setColour (MVColours::textDim);
    g.setFont (Font (13.0f, Font::bold));
    g.drawText ("OUTPUT", cc.withY (cc.getY()+58).withHeight (18), Justification::centred);

    // meters
    g.setColour (MVColours::textDim);
    g.setFont (Font (12.0f, Font::bold));
    g.drawText ("OUTPUT LEVEL", right.withHeight (24).translated (0, 6), Justification::centred);
    drawMeters (g, right.withTrimmedTop (30).reduced (12, 8));

    // bottom labels
    g.setColour (MVColours::textDim);
    g.setFont (Font (10.5f, Font::bold));
    auto bl = bottom.reduced (6, 6);
    g.drawText ("OVERSAMPLING", bl.getX(),        bl.getY(), 120, 14, Justification::centred);
    g.drawText ("RESPONSE",     bl.getX()+150,    bl.getY(), 120, 14, Justification::centred);
    g.drawText ("OUTPUT MODE",  bl.getX()+300,    bl.getY(), 120, 14, Justification::centred);
    g.drawText ("MIX",          bl.getRight()-170, bl.getY(), 70, 14, Justification::centred);
    g.drawText ("TRIM",         bl.getRight()-85,  bl.getY(), 70, 14, Justification::centred);
}

//==============================================================================
void PiradexMVAudioProcessorEditor::drawHeader (Graphics& g, Rectangle<int> h)
{
    g.setColour (MVColours::panel);
    g.fillRoundedRectangle (h.toFloat(), 10.0f);
    g.setColour (MVColours::stroke);
    g.drawRoundedRectangle (h.toFloat(), 10.0f, 1.0f);

    auto r = h.reduced (14, 0);
    drawPiradexLogo (g, Rectangle<float> ((float) r.getX(), r.getCentreY()-11, 22, 22), MVColours::blue);

    g.setColour (MVColours::text);
    g.setFont (Font (20.0f, Font::bold));
    g.drawText ("PIRADEX", r.withTrimmedLeft (32), Justification::centredLeft);
    auto tw = g.getCurrentFont().getStringWidth ("PIRADEX ");
    g.setColour (MVColours::gold);
    g.drawText ("MV", r.withTrimmedLeft (32 + tw), Justification::centredLeft);

    // VST3 à direita
    g.setColour (MVColours::blue);
    g.setFont (Font (12.0f, Font::bold));
    g.drawText ("VST3 / AU", r.removeFromRight (80), Justification::centredRight);

    // preset box (decorativo)
    auto pb = h.withSizeKeepingCentre (220, 24);
    g.setColour (MVColours::bg);
    g.fillRoundedRectangle (pb.toFloat(), 6.0f);
    g.setColour (MVColours::stroke);
    g.drawRoundedRectangle (pb.toFloat(), 6.0f, 1.0f);
    g.setColour (MVColours::textDim);
    g.setFont (Font (12.0f));
    g.drawText ("Default Preset", pb, Justification::centred);
}

//==============================================================================
void PiradexMVAudioProcessorEditor::drawFaderScale (Graphics& g, Rectangle<int> left)
{
    // escala partilhada entre LOW e HIGH (decorativa, dB de referência)
    const int marks[] = { 12, 9, 6, 3, 0, -3, -6, -9, -12, -18, -24, -36, -48, -60 };
    auto scaleArea = faderArea;
    if (scaleArea.isEmpty()) return;

    g.setColour (MVColours::textDim);
    g.setFont (Font (9.0f));

    int top = scaleArea.getY();
    int bot = scaleArea.getBottom();
    int n = (int) (sizeof(marks)/sizeof(int));
    for (int i = 0; i < n; ++i)
    {
        float t = (float) i / (float) (n - 1);
        int yy = top + (int) (t * (bot - top));
        g.drawText (String (marks[i]),
                    scaleArea.getX() - 2, yy - 6, 22, 12, Justification::centredRight);
    }
}

//==============================================================================
void PiradexMVAudioProcessorEditor::drawMeters (Graphics& g, Rectangle<int> area)
{
    meterArea = area;
    const int barW = 20;
    auto l = area.removeFromLeft (barW);
    area.removeFromLeft (6);
    auto scale = area.removeFromLeft (area.getWidth() - barW - 6);
    auto r = area.removeFromRight (barW);

    auto drawBar = [&] (Rectangle<int> bar, float level01)
    {
        g.setColour (MVColours::bg);
        g.fillRoundedRectangle (bar.toFloat(), 3.0f);

        const int segs = 28;
        float segH = bar.getHeight() / (float) segs;
        int lit = (int) std::round (level01 * segs);
        for (int i = 0; i < segs; ++i)
        {
            float frac = (float) i / (float) (segs - 1);   // 0 baixo, 1 topo
            Colour c = frac > 0.86f ? MVColours::meterRed
                     : frac > 0.62f ? MVColours::meterYel
                                    : MVColours::meterGrn;
            bool on = i < lit;
            Rectangle<float> seg (bar.getX()+2.0f,
                                  bar.getBottom() - (i+1)*segH + 1.0f,
                                  bar.getWidth()-4.0f, segH - 1.5f);
            g.setColour (on ? c : c.withAlpha (0.12f));
            g.fillRoundedRectangle (seg, 1.5f);
        }
        g.setColour (MVColours::stroke);
        g.drawRoundedRectangle (bar.toFloat(), 3.0f, 1.0f);
    };

    drawBar (l, mL);
    drawBar (r, mR);

    // escala central
    const int db[] = { 0,-3,-6,-9,-12,-18,-24,-36,-48,-60 };
    g.setColour (MVColours::textDim);
    g.setFont (Font (9.0f));
    int n = (int) (sizeof(db)/sizeof(int));
    for (int i=0;i<n;++i)
    {
        float t = (float) i/(float)(n-1);
        int yy = l.getY() + (int)(t * l.getHeight());
        g.drawText (String (db[i]), scale.getX(), yy-6, scale.getWidth(), 12, Justification::centred);
    }

    // readouts
    auto dbStr = [] (float peak) {
        if (peak < 1.0e-5f) return String ("-inf");
        return String (Decibels::gainToDecibels (peak), 1);
    };
    g.setColour (MVColours::gold);
    g.setFont (Font (12.0f, Font::bold));
    g.drawText (dbStr (proc.outPeakL.load()), l.getX()-4, l.getBottom()+4, barW+8, 16, Justification::centred);
    g.drawText (dbStr (proc.outPeakR.load()), r.getX()-4, r.getBottom()+4, barW+8, 16, Justification::centred);
    g.setColour (MVColours::textDim);
    g.setFont (Font (9.0f, Font::bold));
    g.drawText ("PEAK", scale.getX(), l.getBottom()+6, scale.getWidth(), 12, Justification::centred);
}

//==============================================================================
void PiradexMVAudioProcessorEditor::drawPiradexLogo (Graphics& g, Rectangle<float> b, Colour col)
{
    // losango estilizado "P" do Piradex
    Path dia;
    dia.startNewSubPath (b.getCentreX(), b.getY());
    dia.lineTo (b.getRight(),   b.getCentreY());
    dia.lineTo (b.getCentreX(), b.getBottom());
    dia.lineTo (b.getX(),       b.getCentreY());
    dia.closeSubPath();
    g.setColour (col);
    g.strokePath (dia, PathStrokeType (2.0f));
    g.setFont (Font (b.getHeight()*0.5f, Font::bold));
    g.drawText ("P", b, Justification::centred);
}

//==============================================================================
void PiradexMVAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (48 + 8);          // header
    auto bottom = area.removeFromBottom (92);

    auto left   = area.removeFromLeft (230);
    auto right  = area.removeFromRight (210);
    auto centre = area.reduced (6, 0);

    // ----- faders LOW/HIGH -----
    auto lf = left.reduced (10, 6);
    lf.removeFromTop (18);                 // títulos
    auto faderBody = lf.removeFromTop (lf.getHeight() - 70);
    faderArea = faderBody.withTrimmedLeft (24);   // guardar p/ escala

    int colW = (faderBody.getWidth() - 24) / 2;
    lowFader .setBounds (faderBody.getX()+24,        faderBody.getY(), colW, faderBody.getHeight());
    highFader.setBounds (faderBody.getX()+24+colW,   faderBody.getY(), colW, faderBody.getHeight());

    auto linkRow = lf.removeFromTop (40);
    lowLink .setBounds (faderBody.getX()+24+6,      linkRow.getY()+22, colW-12, 18);
    highLink.setBounds (faderBody.getX()+24+colW+6, linkRow.getY()+22, colW-12, 18);

    // ----- centro: OUTPUT + toggles -----
    auto cc = centre;
    cc.removeFromTop (80);
    outputKnob.setBounds (cc.getCentreX()-60, cc.getY(), 120, 120);
    auto toggles = cc.withY (cc.getY()+150).withHeight (34).reduced (14, 0);
    int bw = (toggles.getWidth() - 18) / 4;
    muteBtn  .setBounds (toggles.getX(),            toggles.getY(), bw, 30);
    dimBtn   .setBounds (toggles.getX()+(bw+6),     toggles.getY(), bw, 30);
    monoBtn  .setBounds (toggles.getX()+2*(bw+6),   toggles.getY(), bw, 30);
    bypassBtn.setBounds (toggles.getX()+3*(bw+6),   toggles.getY(), bw, 30);

    // ----- bottom: combos + mix/trim -----
    auto bl = bottom.reduced (6, 6);
    bl.removeFromTop (16);
    osBox  .setBounds (bl.getX(),      bl.getY()+6, 110, 30);
    respBox.setBounds (bl.getX()+150,  bl.getY()+6, 110, 30);
    modeBox.setBounds (bl.getX()+300,  bl.getY()+6, 110, 30);
    mixKnob .setBounds (bl.getRight()-168, bl.getY()-4, 66, 66);
    trimKnob.setBounds (bl.getRight()-84,  bl.getY()-4, 66, 66);
}

//==============================================================================
void PiradexMVAudioProcessorEditor::timerCallback()
{
    auto toLevel = [] (float peak)
    {
        float db = peak < 1.0e-5f ? -70.0f : Decibels::gainToDecibels (peak);
        return jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    };
    float tL = toLevel (proc.outPeakL.load());
    float tR = toLevel (proc.outPeakR.load());
    // ballistics (sobe rápido, desce suave)
    mL = tL > mL ? tL : mL * 0.82f + tL * 0.18f;
    mR = tR > mR ? tR : mR * 0.82f + tR * 0.18f;
    repaint();
}
