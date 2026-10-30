#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "License.h"
#include <cstring>
#include <cmath>

//==============================================================================
PiradexMVAudioProcessor::PiradexMVAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    lic::License::get();   // inicializa o estado da licença na thread de mensagens
}

juce::StringArray PiradexMVAudioProcessor::paramIDs()
{
    return { "inputTrim","lowAmt","lowThresh","lowMaxGain","trim","mix",
             "highAmt","highThresh","highRatio","output",
             "os","response","mode","bypass","mute","dim","mono","monMid","monSide",
             "scHP","knee","attack","release","ceiling","lookahead",
             "limiter","dither","autoGain","delta" };
}

juce::AudioProcessorValueTreeState::ParameterLayout
PiradexMVAudioProcessor::createLayout()
{
    using FP = juce::AudioParameterFloat;
    using CP = juce::AudioParameterChoice;
    using BP = juce::AudioParameterBool;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto gain24 = juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f);
    auto thr    = juce::NormalisableRange<float>(-60.0f,  0.0f, 0.1f);

    p.push_back (std::make_unique<FP>(juce::ParameterID{"inputTrim",1}, "Input Trim", gain24, 0.0f));

    p.push_back (std::make_unique<FP>(juce::ParameterID{"lowAmt",1},    "Low",        juce::NormalisableRange<float>(0.0f,100.0f,0.1f), 50.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"lowThresh",1}, "Low Thresh", thr, -28.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"lowMaxGain",1},"Max Gain",   juce::NormalisableRange<float>(0.0f,24.0f,0.1f), 12.0f));

    p.push_back (std::make_unique<FP>(juce::ParameterID{"trim",1}, "Trim",
                 juce::NormalisableRange<float>(-12.0f,12.0f,0.1f), 0.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"mix",1},  "Mix",
                 juce::NormalisableRange<float>(0.0f,100.0f,0.1f), 100.0f));

    p.push_back (std::make_unique<FP>(juce::ParameterID{"highAmt",1},    "High",        juce::NormalisableRange<float>(0.0f,100.0f,0.1f), 50.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"highThresh",1}, "High Thresh", thr, -6.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"highRatio",1},  "Ratio",
                 juce::NormalisableRange<float>(1.0f,20.0f,0.1f), 4.0f));

    p.push_back (std::make_unique<FP>(juce::ParameterID{"output",1}, "Output", gain24, 0.0f));

    p.push_back (std::make_unique<CP>(juce::ParameterID{"os",1}, "Oversampling",
                 juce::StringArray{"1x","2x","4x","8x","16x","32x"}, 0));
    p.push_back (std::make_unique<CP>(juce::ParameterID{"response",1}, "Response",
                 juce::StringArray{"Fast","Medium","Slow","Manual"}, 1));
    p.push_back (std::make_unique<CP>(juce::ParameterID{"mode",1}, "Stereo Mode",
                 juce::StringArray{"Stereo","Dual Mono","Mid/Side"}, 0));

    p.push_back (std::make_unique<BP>(juce::ParameterID{"bypass",1}, "Bypass", false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"mute",1},   "Mute",   false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"dim",1},    "Dim",    false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"mono",1},   "Mono",   false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"monMid",1}, "Mid",    false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"monSide",1},"Side",   false));

    // --- avançado / mastering ---
    p.push_back (std::make_unique<FP>(juce::ParameterID{"scHP",1}, "SC HP",
                 juce::NormalisableRange<float>(0.0f, 500.0f, 1.0f, 0.5f), 0.0f));          // 0 = off
    p.push_back (std::make_unique<FP>(juce::ParameterID{"knee",1}, "Knee",
                 juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 6.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"attack",1}, "Attack",
                 juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.4f), 8.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"release",1}, "Release",
                 juce::NormalisableRange<float>(5.0f, 1000.0f, 1.0f, 0.4f), 140.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"ceiling",1}, "Ceiling",
                 juce::NormalisableRange<float>(-6.0f, 0.0f, 0.1f), -1.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"lookahead",1}, "Lookahead",
                 juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 2.0f));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"limiter",1},  "Limiter",  false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"dither",1},   "Dither",   false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"autoGain",1}, "Auto Gain",false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"delta",1},    "Delta",    false));

    return { p.begin(), p.end() };
}

//==============================================================================
void PiradexMVAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    baseSR = sampleRate;
    const size_t nCh = (size_t) juce::jmax (1, getTotalNumInputChannels());

    using OS = juce::dsp::Oversampling<float>;
    os2  = std::make_unique<OS>(nCh, 1, OS::filterHalfBandPolyphaseIIR);
    os4  = std::make_unique<OS>(nCh, 2, OS::filterHalfBandPolyphaseIIR);
    os8  = std::make_unique<OS>(nCh, 3, OS::filterHalfBandPolyphaseIIR);
    os16 = std::make_unique<OS>(nCh, 4, OS::filterHalfBandPolyphaseIIR);
    os32 = std::make_unique<OS>(nCh, 5, OS::filterHalfBandPolyphaseIIR);
    os2->initProcessing  ((size_t) samplesPerBlock);
    os4->initProcessing  ((size_t) samplesPerBlock);
    os8->initProcessing  ((size_t) samplesPerBlock);
    os16->initProcessing ((size_t) samplesPerBlock);
    os32->initProcessing ((size_t) samplesPerBlock);

    setEngineRate (sampleRate);
    lastFactor = 1;
    engL.reset(); engR.reset();

    loudness.prepare (sampleRate);
    tpMeter.prepare (sampleRate);
    limiter.prepare (sampleRate, samplesPerBlock);
    autoGainState = 1.0f;

    const float look = apvts.getRawParameterValue ("lookahead")->load();
    const bool  limOn= apvts.getRawParameterValue ("limiter")->load() > 0.5f;
    limiter.setParams (apvts.getRawParameterValue ("ceiling")->load(), look, 150.0f);
    setLatencySamples (limOn ? limiter.latency() : 0);
}

bool PiradexMVAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

juce::dsp::Oversampling<float>* PiradexMVAudioProcessor::currentOS (int factor)
{
    switch (factor) { case 2: return os2.get();  case 4:  return os4.get();
                      case 8: return os8.get();  case 16: return os16.get();
                      case 32: return os32.get(); default: return nullptr; }
}

void PiradexMVAudioProcessor::setEngineRate (double effectiveSR)
{
    engL.prepare (effectiveSR);
    engR.prepare (effectiveSR);
}

void PiradexMVAudioProcessor::processChannels (float* const* chans, int numCh, int ns, int mode)
{
    if (numCh < 2)
    {
        auto* d = chans[0];
        for (int n = 0; n < ns; ++n) d[n] *= engL.computeGain (d[n]);
        return;
    }

    auto* L = chans[0];
    auto* R = chans[1];

    if (mode == 2) // Mid/Side
    {
        for (int n = 0; n < ns; ++n)
        {
            float m = 0.5f * (L[n] + R[n]);
            float s = 0.5f * (L[n] - R[n]);
            m *= engL.computeGain (m);
            s *= engR.computeGain (s);
            L[n] = m + s;
            R[n] = m - s;
        }
    }
    else if (mode == 0) // Stereo (linkado)
    {
        for (int n = 0; n < ns; ++n)
        {
            float key = juce::jmax (std::abs (L[n]), std::abs (R[n]));
            float g = engL.computeGain (key);
            L[n] *= g; R[n] *= g;
        }
    }
    else // Dual Mono
    {
        for (int n = 0; n < ns; ++n)
        {
            L[n] *= engL.computeGain (L[n]);
            R[n] *= engR.computeGain (R[n]);
        }
    }
}

void PiradexMVAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Bloqueado (a partir de 01/01/2027 sem senha válida): áudio passa transparente.
    if (! lic::License::get().isUnlocked())
        return;

    auto pv = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const bool  bypass = pv("bypass") > 0.5f;
    const float inTrim = pv("inputTrim");
    const float lowAmt = pv("lowAmt") / 100.0f;
    const float lowThr = pv("lowThresh");
    const float lowMax = pv("lowMaxGain");
    const float trimDb = pv("trim");
    const float mix    = pv("mix") / 100.0f;
    const float hiAmt  = pv("highAmt") / 100.0f;
    const float hiThr  = pv("highThresh");
    const float hiRat  = pv("highRatio");
    const float outDb  = pv("output");
    const bool  mute   = pv("mute") > 0.5f;
    const bool  dim    = pv("dim")  > 0.5f;
    const bool  mono   = pv("mono") > 0.5f;
    const bool  monMid = pv("monMid")  > 0.5f;
    const bool  monSide= pv("monSide") > 0.5f;
    const int   osSel  = (int) pv("os");
    const int   resp   = (int) pv("response");
    const int   mode   = (int) pv("mode");
    const float scHP   = pv("scHP");
    const float knee   = pv("knee");
    const float atk    = pv("attack");
    const float rel    = pv("release");
    const float ceilDb = pv("ceiling");
    const float lookMs = pv("lookahead");
    const bool  limOn  = pv("limiter")  > 0.5f;
    const bool  ditOn  = pv("dither")   > 0.5f;
    const bool  autoG  = pv("autoGain") > 0.5f;
    const bool  delta  = pv("delta")    > 0.5f;

    // ganho de entrada (afeta o IN meter e a deteção)
    buffer.applyGain (juce::Decibels::decibelsToGain (inTrim));

    auto msPeak = [] (const juce::AudioBuffer<float>& b, int ns, float& mPk, float& sPk)
    {
        mPk = sPk = 0.0f;
        if (b.getNumChannels() < 2) { mPk = b.getMagnitude (0, 0, ns); sPk = 0.0f; return; }
        const float* L = b.getReadPointer (0);
        const float* R = b.getReadPointer (1);
        for (int n = 0; n < ns; ++n)
        {
            mPk = juce::jmax (mPk, std::abs (0.5f * (L[n] + R[n])));
            sPk = juce::jmax (sPk, std::abs (0.5f * (L[n] - R[n])));
        }
    };

    inPeakL.store (buffer.getMagnitude (0, 0, numSamples));
    if (numCh > 1) inPeakR.store (buffer.getMagnitude (1, 0, numSamples));
    { float m,s; msPeak (buffer, numSamples, m, s); inPeakM.store (m); inPeakS.store (s); }

    const int factor = (osSel == 1 ? 2 : osSel == 2 ? 4 : osSel == 3 ? 8
                        : osSel == 4 ? 16 : osSel == 5 ? 32 : 1);
    if (factor != lastFactor) { setEngineRate (baseSR * factor); lastFactor = factor; }

    for (auto* e : { &engL, &engR })
    {
        e->setLowThresh (lowThr); e->setLowMaxGain (lowMax); e->setLowAmount (lowAmt);
        e->setHighThresh (hiThr); e->setHighRatio (hiRat);   e->setHighAmount (hiAmt);
        e->setKnee (knee);
        e->setDetectorHP (scHP);
        if (resp == 3) e->setTimes (atk, rel);   // Manual
        else           e->setResponse (resp);
    }

    if (bypass)
    {
        outPeakL.store (buffer.getMagnitude (0, 0, numSamples));
        if (numCh > 1) outPeakR.store (buffer.getMagnitude (1, 0, numSamples));
        return;
    }

    juce::AudioBuffer<float> dry;
    dry.makeCopyOf (buffer);

    buffer.applyGain (juce::Decibels::decibelsToGain (trimDb));

    if (factor == 1)
    {
        processChannels (buffer.getArrayOfWritePointers(), numCh, numSamples, mode);
    }
    else if (auto* os = currentOS (factor))
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto up = os->processSamplesUp (block);
        const int upNs = (int) up.getNumSamples();
        const int upCh = (int) up.getNumChannels();
        std::vector<float*> ptrs ((size_t) upCh);
        for (int ch = 0; ch < upCh; ++ch) ptrs[(size_t) ch] = up.getChannelPointer ((size_t) ch);
        processChannels (ptrs.data(), upCh, upNs, mode);
        os->processSamplesDown (block);
    }

    buffer.applyGain (juce::Decibels::decibelsToGain (outDb));

    if (mix < 0.999f)
    {
        const float wet = mix, dryG = 1.0f - mix;
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* w = buffer.getWritePointer (ch);
            auto* dptr = dry.getReadPointer (juce::jmin (ch, dry.getNumChannels()-1));
            for (int n = 0; n < numSamples; ++n) w[n] = w[n]*wet + dptr[n]*dryG;
        }
    }

    // DELTA: ouvir só o que foi alterado (processado - original)
    if (delta)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* w = buffer.getWritePointer (ch);
            auto* dptr = dry.getReadPointer (juce::jmin (ch, dry.getNumChannels()-1));
            for (int n = 0; n < numSamples; ++n) w[n] -= dptr[n];
        }
    }
    else
    {
        // AUTO-GAIN: iguala o RMS do processado ao do original
        if (autoG)
        {
            auto rmsOf = [] (const juce::AudioBuffer<float>& b, int ns){
                double s=0.0; int ch=b.getNumChannels();
                for (int c=0;c<ch;++c){ const float* d=b.getReadPointer(c);
                    for (int n=0;n<ns;++n) s+=(double)d[n]*d[n]; }
                return (float) std::sqrt (s / juce::jmax (1, ns*ch)); };
            float dRms = rmsOf (dry, numSamples), wRms = rmsOf (buffer, numSamples);
            float target = juce::jlimit (0.25f, 4.0f, dRms / (wRms + 1.0e-6f));
            autoGainState += (target - autoGainState) * 0.10f;
            buffer.applyGain (autoGainState);
        }
        else autoGainState = 1.0f;

        // CEILING: limiter brickwall com lookahead
        int wantLat = 0;
        if (limOn)
        {
            limiter.setParams (ceilDb, lookMs, 150.0f);
            limiter.process (buffer.getWritePointer (0),
                             numCh > 1 ? buffer.getWritePointer (1) : nullptr, numSamples);
            wantLat = limiter.latency();
        }
        if (wantLat != reportedLatency) { reportedLatency = wantLat; setLatencySamples (wantLat); }
    }

    // monitorização: SIDE isola os lados, MID/MONO isolam o centro
    if (numCh > 1 && (monSide || monMid || mono))
    {
        auto* L = buffer.getWritePointer (0);
        auto* R = buffer.getWritePointer (1);
        for (int n = 0; n < numSamples; ++n)
        {
            float v = monSide ? 0.5f*(L[n]-R[n]) : 0.5f*(L[n]+R[n]);
            L[n] = R[n] = v;
        }
    }

    if (mute)      buffer.applyGain (0.0f);
    else if (dim)  buffer.applyGain (juce::Decibels::decibelsToGain (-12.0f));

    // DITHER TPDF no output
    if (ditOn)
    {
        ditherL.setEnabled (true); ditherR.setEnabled (true);
        auto* L = buffer.getWritePointer (0);
        auto* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;
        for (int n = 0; n < numSamples; ++n) { L[n] = ditherL.process (L[n]); if (R) R[n] = ditherR.process (R[n]); }
    }

    // --- medição do output ---
    {
        const float* L = buffer.getReadPointer (0);
        const float* R = numCh > 1 ? buffer.getReadPointer (1) : L;
        for (int n = 0; n < numSamples; ++n)
            pushSampleToFifo (0.5f * (L[n] + R[n]));

        loudness.process (L, numCh > 1 ? R : nullptr, numSamples);
        lufsMval.store (loudness.getM()); lufsSval.store (loudness.getS()); lufsIval.store (loudness.getI());

        tpMeter.decay (0.97f);
        tpMeter.process (L, numSamples);
        if (numCh > 1) tpMeter.process (R, numSamples);
        truePeakDb.store (juce::Decibels::gainToDecibels (tpMeter.getPeak() + 1.0e-9f));

        bool clip = false;
        for (int ch = 0; ch < numCh; ++ch)
            if (buffer.getMagnitude (ch, 0, numSamples) > 1.0f) { clip = true; break; }
        if (clip) clipFlag.store (true);
    }

    outPeakL.store (buffer.getMagnitude (0, 0, numSamples));
    if (numCh > 1) outPeakR.store (buffer.getMagnitude (1, 0, numSamples));
    { float m,s; msPeak (buffer, numSamples, m, s); outPeakM.store (m); outPeakS.store (s); }
    grLowVal .store (engL.grLow);
    grHighVal.store (engL.grHigh);
}

//==============================================================================
void PiradexMVAudioProcessor::pushSampleToFifo (float s) noexcept
{
    if (fifoIndex == fftSize)
    {
        doFFT();
        fifoIndex = 0;
    }
    fifo[fifoIndex++] = s;
}

void PiradexMVAudioProcessor::doFFT()
{
    juce::zeromem (fftData, sizeof (fftData));
    std::memcpy (fftData, fifo, sizeof (float) * (size_t) fftSize);
    window.multiplyWithWindowingTable (fftData, (size_t) fftSize);
    forwardFFT.performFrequencyOnlyForwardTransform (fftData);

    const int half = fftSize / 2;
    for (int i = 0; i < numBins; ++i)
    {
        // eixo logarítmico: mapeia o bin de display para um bin do FFT
        float f = (float) i / (float) numBins;
        int b0 = (int) std::round (std::pow ((float) half, f));
        b0 = juce::jlimit (1, half - 1, b0);
        float mag = fftData[b0];
        float db  = juce::Decibels::gainToDecibels (mag / (float) fftSize + 1.0e-9f);
        float norm = juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 90.0f);   // -90..0 dB
        float prev = spectrum[i].load();
        spectrum[i].store (norm > prev ? norm : prev * 0.82f + norm * 0.18f); // decay suave
    }
}

//==============================================================================
juce::AudioProcessorEditor* PiradexMVAudioProcessor::createEditor()
{
    return new PiradexMVAudioProcessorEditor (*this);
}

void PiradexMVAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void PiradexMVAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PiradexMVAudioProcessor();
}
