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
    std::atomic<float> inPeakL  { 0.0f }, inPeakR  { 0.0f };
    std::atomic<float> outPeakL { 0.0f }, outPeakR { 0.0f };
    std::atomic<float> inPeakM  { 0.0f }, inPeakS  { 0.0f };
    std::atomic<float> outPeakM { 0.0f }, outPeakS { 0.0f };
    std::atomic<float> grLowVal { 0.0f }, grHighVal{ 0.0f };

    static juce::StringArray paramIDs();

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    MV2Engine engL, engR;

    std::unique_ptr<juce::dsp::Oversampling<float>> os2, os4, os8, os16, os32;
    int lastFactor = 1;
    double baseSR = 44100.0;

    juce::dsp::Oversampling<float>* currentOS (int factor);
    void setEngineRate (double effectiveSR);
    void processChannels (float* const* chans, int numCh, int ns, int mode);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PiradexMVAudioProcessor)
};
