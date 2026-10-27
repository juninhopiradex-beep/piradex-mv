#pragma once
#include <JuceHeader.h>
#include "MV2DSP.h"

//==============================================================================
class PiradexMVAudioProcessor : public juce::AudioProcessor
{
public:
    PiradexMVAudioProcessor();
    ~PiradexMVAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // meters (lidos pela UI, sem locks)
    std::atomic<float> outPeakL { 0.0f }, outPeakR { 0.0f };
    std::atomic<float> outRmsL  { 0.0f }, outRmsR  { 0.0f };
    std::atomic<float> grLow    { 0.0f }, grHigh   { 0.0f };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    MV2Engine engL, engR;

    // oversampling: 1x (direto), 2x, 4x, 8x
    std::unique_ptr<juce::dsp::Oversampling<float>> os2, os4, os8;
    int lastFactor = 1;
    double baseSR = 44100.0;

    juce::dsp::Oversampling<float>* currentOS (int factor);
    void setEngineRate (double effectiveSR);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PiradexMVAudioProcessor)
};
