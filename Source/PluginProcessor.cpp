#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GranularVerbDelayAudioProcessor::GranularVerbDelayAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    apvts(*this, nullptr, "Parameters", createParameterLayout()),
    randomDistribution(0.0f, 1.0f)
{
    randomEngine.seed(std::random_device()());
    grains.resize(64); // Maximum 64 simultaneous grains
    grainPositions.reserve(64);
}

GranularVerbDelayAudioProcessor::~GranularVerbDelayAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout GranularVerbDelayAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delayTime", "Delay Time",
        juce::NormalisableRange<float>(0.01f, 2.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grainSize", "Grain Size",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f), 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grainDensity", "Grain Density",
        juce::NormalisableRange<float>(1.0f, 100.0f, 1.0f), 20.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "reverbMix", "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "stereoWidth", "Stereo Width",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "dryWet", "Dry/Wet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "feedback", "Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.4f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grainPitch", "Grain Pitch",
        juce::NormalisableRange<float>(0.5f, 2.0f, 0.01f), 1.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String GranularVerbDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GranularVerbDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GranularVerbDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GranularVerbDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GranularVerbDelayAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GranularVerbDelayAudioProcessor::getNumPrograms()
{
    return 1;
}

int GranularVerbDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GranularVerbDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String GranularVerbDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void GranularVerbDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void GranularVerbDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Allocate delay buffer (4 seconds max)
    delayBuffer.setSize(2, static_cast<int>(sampleRate * 4.0));
    delayBuffer.clear();

    // Visualization buffer
    visualizationBuffer.setSize(2, samplesPerBlock);

    // Setup reverb
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;

    reverb.prepare(spec);
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.7f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = 0.3f;
    reverbParams.dryLevel = 0.7f;
    reverbParams.width = 1.0f;
    reverb.setParameters(reverbParams);

    // Setup low-pass filter
    lowPassFilter.prepare(spec);
    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 8000.0f);

    writePosition = 0;
}

void GranularVerbDelayAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GranularVerbDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
  #endif
}
#endif

void GranularVerbDelayAudioProcessor::updateGrains(int numSamples)
{
    auto grainDensity = apvts.getRawParameterValue("grainDensity")->load();
    auto grainSize = apvts.getRawParameterValue("grainSize")->load();

    grainPositions.clear();

    // Calculate grain spawn probability based on density
    float spawnProbability = grainDensity / 100.0f;

    for (auto& grain : grains)
    {
        if (grain.active)
        {
            grain.age++;
            if (grain.age >= grain.lifetime)
            {
                grain.active = false;
            }
            else
            {
                grainPositions.push_back(static_cast<float>(grain.readPosition) / delayBuffer.getNumSamples());
            }
        }
        else if (randomDistribution(randomEngine) < spawnProbability * 0.01f)
        {
            // Spawn new grain
            grain.active = true;
            grain.age = 0;
            grain.lifetime = static_cast<int>(grainSize * getSampleRate() / 1000.0f);
            grain.amplitude = 0.5f + randomDistribution(randomEngine) * 0.5f;

            auto delayTime = apvts.getRawParameterValue("delayTime")->load();
            int maxDelay = static_cast<int>(delayTime * getSampleRate());
            grain.readPosition = (writePosition - maxDelay + delayBuffer.getNumSamples()) % delayBuffer.getNumSamples();

            grainPositions.push_back(static_cast<float>(grain.readPosition) / delayBuffer.getNumSamples());
        }
    }
}

void GranularVerbDelayAudioProcessor::processStereoWidth(juce::AudioBuffer<float>& buffer, float width)
{
    if (buffer.getNumChannels() < 2)
        return;

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float mid = (leftChannel[i] + rightChannel[i]) * 0.5f;
        float side = (leftChannel[i] - rightChannel[i]) * 0.5f;

        // Apply width
        side *= width;

        leftChannel[i] = mid + side;
        rightChannel[i] = mid - side;
    }
}

void GranularVerbDelayAudioProcessor::processGranularDelay(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int delayBufferSize = delayBuffer.getNumSamples();

    auto feedback = apvts.getRawParameterValue("feedback")->load();
    auto grainPitch = apvts.getRawParameterValue("grainPitch")->load();

    updateGrains(numSamples);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* delayData = delayBuffer.getWritePointer(channel);

        for (int i = 0; i < numSamples; ++i)
        {
            // Write to delay buffer with feedback
            int writeIndex = (writePosition + i) % delayBufferSize;
            delayData[writeIndex] = channelData[i] + delayData[writeIndex] * feedback;

            // Read from active grains
            float grainOutput = 0.0f;
            int activeGrains = 0;

            for (auto& grain : grains)
            {
                if (grain.active)
                {
                    // Apply envelope (Hann window)
                    float phase = static_cast<float>(grain.age) / grain.lifetime;
                    float envelope = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * phase));

                    // Read from delay buffer with pitch shifting
                    float readPos = grain.readPosition + (grain.age * grainPitch);
                    int readIndex = static_cast<int>(readPos) % delayBufferSize;

                    grainOutput += delayData[readIndex] * envelope * grain.amplitude;
                    activeGrains++;
                }
            }

            if (activeGrains > 0)
                grainOutput /= std::sqrt(static_cast<float>(activeGrains)); // Normalize

            channelData[i] = grainOutput;
        }
    }

    writePosition = (writePosition + numSamples) % delayBufferSize;
}

void GranularVerbDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Copy input for visualization
    visualizationBuffer.makeCopyOf(buffer);

    // Store dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Process granular delay
    processGranularDelay(buffer);

    // Apply reverb
    auto reverbMix = apvts.getRawParameterValue("reverbMix")->load();
    if (reverbMix > 0.01f)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        juce::Reverb::Parameters reverbParams = reverb.getParameters();
        reverbParams.wetLevel = reverbMix;
        reverbParams.dryLevel = 1.0f - reverbMix;
        reverb.setParameters(reverbParams);

        reverb.process(context);
    }

    // Apply stereo width
    auto stereoWidth = apvts.getRawParameterValue("stereoWidth")->load();
    processStereoWidth(buffer, stereoWidth);

    // Apply dry/wet mix
    auto dryWet = apvts.getRawParameterValue("dryWet")->load();
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* wetData = buffer.getWritePointer(channel);
        auto* dryData = dryBuffer.getReadPointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            wetData[i] = dryData[i] * (1.0f - dryWet) + wetData[i] * dryWet;
        }
    }
}

//==============================================================================
bool GranularVerbDelayAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GranularVerbDelayAudioProcessor::createEditor()
{
    return new GranularVerbDelayAudioProcessorEditor (*this);
}

//==============================================================================
void GranularVerbDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GranularVerbDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranularVerbDelayAudioProcessor();
}
