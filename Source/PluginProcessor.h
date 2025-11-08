#pragma once

#include <JuceHeader.h>
#include <vector>
#include <random>

//==============================================================================
class GranularVerbDelayAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    GranularVerbDelayAudioProcessor();
    ~GranularVerbDelayAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Audio parameters
    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }

    // For visual feedback
    juce::AudioBuffer<float>& getVisualizationBuffer() { return visualizationBuffer; }
    std::vector<float>& getGrainPositions() { return grainPositions; }

private:
    //==============================================================================
    // Granular synthesis grain structure
    struct Grain
    {
        int position = 0;
        int age = 0;
        int lifetime = 0;
        float amplitude = 0.0f;
        int readPosition = 0;
        bool active = false;
    };

    // Audio processing members
    juce::AudioBuffer<float> delayBuffer;
    juce::AudioBuffer<float> visualizationBuffer;
    std::vector<Grain> grains;
    std::vector<float> grainPositions;

    int writePosition = 0;
    std::mt19937 randomEngine;
    std::uniform_real_distribution<float> randomDistribution;

    // DSP
    juce::dsp::Reverb reverb;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                     juce::dsp::IIR::Coefficients<float>> lowPassFilter;

    // Parameters
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Processing functions
    void processGranularDelay(juce::AudioBuffer<float>& buffer);
    void updateGrains(int numSamples);
    void processStereoWidth(juce::AudioBuffer<float>& buffer, float width);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularVerbDelayAudioProcessor)
};
