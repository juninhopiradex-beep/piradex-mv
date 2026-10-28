#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PiradexMVAudioProcessor::PiradexMVAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
PiradexMVAudioProcessor::createLayout()
{
    using FP = juce::AudioParameterFloat;
    using CP = juce::AudioParameterChoice;
    using BP = juce::AudioParameterBool;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto dbRange = juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f);

    p.push_back (std::make_unique<FP>(juce::ParameterID{"low",1},  "Low",    dbRange, 0.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"high",1}, "High",   dbRange, 0.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"output",1},"Output", dbRange, 0.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"trim",1}, "Trim",
                 juce::NormalisableRange<float>(-12.0f,12.0f,0.1f), 0.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"mix",1},  "Mix",
                 juce::NormalisableRange<float>(0.0f,100.0f,0.1f), 100.0f));

    p.push_back (std::make_unique<BP>(juce::ParameterID{"link",1},   "Link",   false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"mute",1},   "Mute",   false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"dim",1},    "Dim",    false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"mono",1},   "Mono",   false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"bypass",1}, "Bypass", false));

    p.push_back (std::make_unique<CP>(juce::ParameterID{"os",1}, "Oversampling",
                 juce::StringArray{"1x","2x","4x","8x"}, 0));
    p.push_back (std::make_unique<CP>(juce::ParameterID{"response",1}, "Response",
                 juce::StringArray{"Fast","Medium","Slow"}, 1));
    p.push_back (std::make_unique<CP>(juce::ParameterID{"mode",1}, "Output Mode",
                 juce::StringArray{"Stereo","Dual Mono","Mid/Side"}, 0));

    return { p.begin(), p.end() };
}

//==============================================================================
void PiradexMVAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    baseSR = sampleRate;
    const size_t nCh = (size_t) juce::jmax (1, getTotalNumInputChannels());

    os2 = std::make_unique<juce::dsp::Oversampling<float>>(nCh, 1,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    os4 = std::make_unique<juce::dsp::Oversampling<float>>(nCh, 2,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    os8 = std::make_unique<juce::dsp::Oversampling<float>>(nCh, 3,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);
    os2->initProcessing ((size_t) samplesPerBlock);
    os4->initProcessing ((size_t) samplesPerBlock);
    os8->initProcessing ((size_t) samplesPerBlock);

    setEngineRate (sampleRate);
    lastFactor = 1;
    engL.reset(); engR.reset();
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
    switch (factor) { case 2: return os2.get(); case 4: return os4.get();
                      case 8: return os8.get(); default: return nullptr; }
}

void PiradexMVAudioProcessor::setEngineRate (double effectiveSR)
{
    engL.prepare (effectiveSR);
    engR.prepare (effectiveSR);
}

// dB do fader -> quantidade 0..1 do efeito (abaixo de -24 dB praticamente off)
static inline float dbToAmount (float db)
{
    return juce::jlimit (0.0f, 1.0f, (db + 24.0f) / 36.0f);
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
    else if (mode == 0) // Stereo (linkado — mesmo ganho nos dois canais)
    {
        for (int n = 0; n < ns; ++n)
        {
            float key = juce::jmax (std::abs (L[n]), std::abs (R[n]));
            float g = engL.computeGain (key);
            L[n] *= g; R[n] *= g;
        }
    }
    else // Dual Mono (independente)
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

    const bool  bypass = apvts.getRawParameterValue ("bypass")->load() > 0.5f;
    const float lowDb  = apvts.getRawParameterValue ("low")->load();
    const float highDb = apvts.getRawParameterValue ("high")->load();
    const float outDb  = apvts.getRawParameterValue ("output")->load();
    const float trimDb = apvts.getRawParameterValue ("trim")->load();
    const float mix    = apvts.getRawParameterValue ("mix")->load() / 100.0f;
    const bool  mute   = apvts.getRawParameterValue ("mute")->load() > 0.5f;
    const bool  dim    = apvts.getRawParameterValue ("dim")->load()  > 0.5f;
    const bool  mono   = apvts.getRawParameterValue ("mono")->load() > 0.5f;
    const int   osSel  = (int) apvts.getRawParameterValue ("os")->load();
    const int   resp   = (int) apvts.getRawParameterValue ("response")->load();
    const int   mode   = (int) apvts.getRawParameterValue ("mode")->load();

    const int factor = (osSel == 1 ? 2 : osSel == 2 ? 4 : osSel == 3 ? 8 : 1);
    if (factor != lastFactor) { setEngineRate (baseSR * factor); lastFactor = factor; }

    const float la = dbToAmount (lowDb), ha = dbToAmount (highDb);
    engL.setLow (la); engL.setHigh (ha); engL.setResponse (resp);
    engR.setLow (la); engR.setHigh (ha); engR.setResponse (resp);

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

    if (mono && numCh > 1)
    {
        auto* L = buffer.getWritePointer (0);
        auto* R = buffer.getWritePointer (1);
        for (int n = 0; n < numSamples; ++n) { float m = 0.5f*(L[n]+R[n]); L[n]=R[n]=m; }
    }

    if (mute)      buffer.applyGain (0.0f);
    else if (dim)  buffer.applyGain (juce::Decibels::decibelsToGain (-12.0f));

    auto rms = [] (const float* d, int n) {
        double s=0.0; for (int i=0;i<n;++i) s += (double)d[i]*d[i];
        return (float) std::sqrt (s / juce::jmax (1,n));
    };
    outPeakL.store (buffer.getMagnitude (0, 0, numSamples));
    outRmsL .store (rms (buffer.getReadPointer (0), numSamples));
    if (numCh > 1)
    {
        outPeakR.store (buffer.getMagnitude (1, 0, numSamples));
        outRmsR .store (rms (buffer.getReadPointer (1), numSamples));
    }
    grLow .store (engL.grLow);
    grHigh.store (engL.grHigh);
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
