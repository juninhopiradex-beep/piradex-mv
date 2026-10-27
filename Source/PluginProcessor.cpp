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

    p.push_back (std::make_unique<FP>(juce::ParameterID{"low",1},  "Low Level",
                 juce::NormalisableRange<float>(0.0f,100.0f,0.1f), 0.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"high",1}, "High Level",
                 juce::NormalisableRange<float>(0.0f,100.0f,0.1f), 0.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"output",1},"Output",
                 juce::NormalisableRange<float>(-24.0f,12.0f,0.1f), 0.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"trim",1}, "Trim",
                 juce::NormalisableRange<float>(-12.0f,12.0f,0.1f), 0.0f));
    p.push_back (std::make_unique<FP>(juce::ParameterID{"mix",1},  "Mix",
                 juce::NormalisableRange<float>(0.0f,100.0f,0.1f), 100.0f));

    p.push_back (std::make_unique<BP>(juce::ParameterID{"lowlink",1},  "Low Link",  true));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"highlink",1}, "High Link", true));

    p.push_back (std::make_unique<BP>(juce::ParameterID{"mute",1},   "Mute",   false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"dim",1},    "Dim",    false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"mono",1},   "Mono",   false));
    p.push_back (std::make_unique<BP>(juce::ParameterID{"bypass",1}, "Bypass", false));

    p.push_back (std::make_unique<CP>(juce::ParameterID{"os",1}, "Oversampling",
                 juce::StringArray{"1x","2x","4x","8x"}, 0));
    p.push_back (std::make_unique<CP>(juce::ParameterID{"response",1}, "Response",
                 juce::StringArray{"Smooth","Balanced","Fast"}, 1));
    p.push_back (std::make_unique<CP>(juce::ParameterID{"mode",1}, "Output Mode",
                 juce::StringArray{"Classic","Modern"}, 1));

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
    engL.reset();
    engR.reset();
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

void PiradexMVAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    const bool  bypass = apvts.getRawParameterValue ("bypass")->load() > 0.5f;
    const float low    = apvts.getRawParameterValue ("low")->load()  / 100.0f;
    const float high   = apvts.getRawParameterValue ("high")->load() / 100.0f;
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

    if (factor != lastFactor)
    {
        setEngineRate (baseSR * factor);
        lastFactor = factor;
    }

    engL.setLow (low);  engL.setHigh (high);  engL.setResponse (resp);  engL.setMode (mode);
    engR.setLow (low);  engR.setHigh (high);  engR.setResponse (resp);  engR.setMode (mode);

    // bypass total
    if (bypass)
    {
        outPeakL.store (buffer.getMagnitude (0, 0, numSamples));
        if (numCh > 1) outPeakR.store (buffer.getMagnitude (1, 0, numSamples));
        return;
    }

    // guarda dry (para mix) + aplica trim
    juce::AudioBuffer<float> dry;
    dry.makeCopyOf (buffer);

    const float trimGain = juce::Decibels::decibelsToGain (trimDb);
    buffer.applyGain (trimGain);

    // ----------- processamento (com ou sem oversampling) -----------
    auto processDirect = [&] (juce::AudioBuffer<float>& buf)
    {
        const int ns = buf.getNumSamples();
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        {
            auto* d = buf.getWritePointer (ch);
            auto& eng = (ch == 0 ? engL : engR);
            for (int n = 0; n < ns; ++n)
                d[n] = eng.processSample (d[n]);
        }
    };

    if (factor == 1)
    {
        processDirect (buffer);
    }
    else if (auto* os = currentOS (factor))
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto up = os->processSamplesUp (block);

        const int upNs = (int) up.getNumSamples();
        const int upCh = (int) up.getNumChannels();
        for (int ch = 0; ch < upCh; ++ch)
        {
            auto* d = up.getChannelPointer ((size_t) ch);
            auto& eng = (ch == 0 ? engL : engR);
            for (int n = 0; n < upNs; ++n)
                d[n] = eng.processSample (d[n]);
        }
        os->processSamplesDown (block);
    }

    // ----------- output gain -----------
    buffer.applyGain (juce::Decibels::decibelsToGain (outDb));

    // ----------- mix dry/wet -----------
    if (mix < 0.999f)
    {
        const float wet = mix, dryG = 1.0f - mix;
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* w = buffer.getWritePointer (ch);
            auto* dptr = dry.getReadPointer (juce::jmin (ch, dry.getNumChannels()-1));
            for (int n = 0; n < numSamples; ++n)
                w[n] = w[n] * wet + dptr[n] * dryG;
        }
    }

    // ----------- mono -----------
    if (mono && numCh > 1)
    {
        auto* L = buffer.getWritePointer (0);
        auto* R = buffer.getWritePointer (1);
        for (int n = 0; n < numSamples; ++n)
        {
            float m = 0.5f * (L[n] + R[n]);
            L[n] = R[n] = m;
        }
    }

    // ----------- dim / mute -----------
    if (mute)       buffer.applyGain (0.0f);
    else if (dim)   buffer.applyGain (juce::Decibels::decibelsToGain (-18.0f));

    // ----------- meters -----------
    auto rms = [] (const float* d, int n) {
        double s = 0.0; for (int i=0;i<n;++i) s += (double) d[i]*d[i];
        return (float) std::sqrt (s / juce::jmax (1, n));
    };
    outPeakL.store (buffer.getMagnitude (0, 0, numSamples));
    outRmsL .store (rms (buffer.getReadPointer (0), numSamples));
    if (numCh > 1)
    {
        outPeakR.store (buffer.getMagnitude (1, 0, numSamples));
        outRmsR .store (rms (buffer.getReadPointer (1), numSamples));
    }
    grLow .store (engL.lastGrLow);
    grHigh.store (engL.lastGrHigh);
}

//==============================================================================
juce::AudioProcessorEditor* PiradexMVAudioProcessor::createEditor()
{
    return new PiradexMVAudioProcessorEditor (*this);
}

void PiradexMVAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
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
