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
        juce::NormalisableRange<float>(0.01f, 2.0f, 0.01f), 0.25f)); // 250ms - shorter, more manageable

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grainSize", "Grain Size",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f), 80.0f)); // 80ms - medium grains

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grainDensity", "Grain Density",
        juce::NormalisableRange<float>(1.0f, 100.0f, 1.0f), 8.0f)); // 8 grains/sec - less intense

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grainPitch", "Grain Pitch",
        juce::NormalisableRange<float>(0.5f, 2.0f, 0.01f), 1.0f)); // Normal pitch

    // Granulator III-style controls
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spray", "Spray",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 5.0f, // 5ms - minimal randomization
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; }));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grainSizeVar", "Size Variation",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 0.0f, // No variation by default
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " %"; }));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grainPitchVar", "Pitch Variation",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 0.0f, // No variation by default
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " %"; }));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "filePosition", "File Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.75f, // 75% - closer to recent audio
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value * 100.0f, 1) + " %"; }));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "reverbMix", "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.15f)); // 15% - subtle reverb

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "stereoWidth", "Stereo Width",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f)); // 100% - normal stereo

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "dryWet", "Dry/Wet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.35f)); // 35% - more dry

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "feedback", "Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.25f)); // 25% - less feedback

    // Delay mode (stereo/ping pong)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "delayMode", "Delay Mode",
        juce::StringArray("Stereo", "Ping Pong"), 0)); // Default: Stereo

    // Reverb type (plate/hall)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "reverbType", "Reverb Type",
        juce::StringArray("Plate", "Hall", "Room"), 0)); // Default: Plate

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
    auto grainPitch = apvts.getRawParameterValue("grainPitch")->load();
    auto delayTime = apvts.getRawParameterValue("delayTime")->load();

    // Granulator III-style parameters
    auto spray = apvts.getRawParameterValue("spray")->load();
    auto grainSizeVar = apvts.getRawParameterValue("grainSizeVar")->load();
    auto grainPitchVar = apvts.getRawParameterValue("grainPitchVar")->load();
    auto filePosition = apvts.getRawParameterValue("filePosition")->load();

    grainPositions.clear();

    // Calculate grains per second, then samples between grain spawns
    float grainsPerSecond = grainDensity;
    float samplesPerGrain = getSampleRate() / std::max(0.1f, grainsPerSecond);
    float spawnChance = 1.0f / samplesPerGrain;

    const int delayBufferSize = delayBuffer.getNumSamples();

    // File position determines where in the delay buffer grains spawn
    // 0.0 = oldest audio, 1.0 = newest audio (like M4L's file position)
    int filePositionSamples = static_cast<int>(filePosition * delayTime * getSampleRate());
    int baseReadPos = (writePosition - filePositionSamples + delayBufferSize) % delayBufferSize;

    for (auto& grain : grains)
    {
        if (grain.active)
        {
            // Collect positions for visualization
            if (grain.age < grain.lifetime * 0.5)  // Only show first half for cleaner visual
            {
                int visualPos = static_cast<int>(grain.readPosition) % delayBufferSize;
                grainPositions.push_back(static_cast<float>(visualPos) / delayBufferSize);
            }
        }
        else if (randomDistribution(randomEngine) < spawnChance)
        {
            // Spawn new grain
            grain.active = true;
            grain.age = 0.0;

            // Grain size with variation (like M4L)
            float sizeVariation = 1.0f + ((randomDistribution(randomEngine) - 0.5f) * 2.0f * grainSizeVar / 100.0f);
            grain.lifetime = (grainSize * sizeVariation / 1000.0) * getSampleRate();
            grain.lifetime = std::max(10.0, grain.lifetime);  // Minimum 10 samples

            // Grain pitch with variation (like M4L)
            float pitchVariation = 1.0f + ((randomDistribution(randomEngine) - 0.5f) * 2.0f * grainPitchVar / 100.0f);
            grain.playbackSpeed = grainPitch * pitchVariation;
            grain.playbackSpeed = juce::jlimit(0.1, 4.0, grain.playbackSpeed);  // Clamp

            // Amplitude randomization for organic sound
            grain.amplitude = 0.8f + randomDistribution(randomEngine) * 0.4f;  // 0.8 to 1.2

            // Spray parameter - position randomization in milliseconds
            float sprayAmount = (randomDistribution(randomEngine) - 0.5f) * 2.0f;  // -1 to +1
            int sprayOffset = static_cast<int>(sprayAmount * spray * getSampleRate() / 1000.0f);

            // Set initial read position with spray randomization
            grain.readPosition = (baseReadPos + sprayOffset + delayBufferSize) % delayBufferSize;

            grainPositions.push_back(static_cast<float>(grain.readPosition) / delayBufferSize);
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
    feedback = juce::jlimit(0.0f, 0.95f, feedback);  // Clamp feedback to prevent runaway

    auto delayMode = apvts.getRawParameterValue("delayMode")->load();
    bool isPingPong = (static_cast<int>(delayMode) == 1); // 1 = Ping Pong

    updateGrains(numSamples);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* delayData = delayBuffer.getWritePointer(channel);

        for (int i = 0; i < numSamples; ++i)
        {
            // First, write input to delay buffer
            int writeIndex = (writePosition + i) % delayBufferSize;
            float inputSample = channelData[i];

            // Read from active grains
            float grainOutput = 0.0f;
            int activeGrainCount = 0;

            // For ping pong, alternate which channel reads grains
            int grainChannel = channel;
            if (isPingPong && buffer.getNumChannels() == 2)
            {
                // Ping pong effect: grains alternate between L and R
                grainChannel = ((writePosition + i) / 100) % 2; // Change channel every 100 samples
            }

            for (auto& grain : grains)
            {
                if (grain.active)
                {
                    // Calculate envelope (Hann window for smooth fades)
                    double phase = grain.age / grain.lifetime;

                    if (phase >= 1.0)
                    {
                        grain.active = false;
                        continue;
                    }

                    float envelope = 0.5f * (1.0f - std::cos(2.0 * juce::MathConstants<double>::pi * phase));

                    // Linear interpolation for smooth grain playback
                    double readPos = grain.readPosition;
                    int readIndex0 = static_cast<int>(readPos) % delayBufferSize;
                    int readIndex1 = (readIndex0 + 1) % delayBufferSize;
                    float frac = static_cast<float>(readPos - std::floor(readPos));

                    // For ping pong, read from appropriate channel based on grain position
                    int readChannel = channel;
                    if (isPingPong && buffer.getNumChannels() == 2)
                    {
                        readChannel = grainChannel;
                    }

                    auto* readDelayData = delayBuffer.getReadPointer(readChannel);

                    // Interpolated read from delay buffer
                    float sample0 = readDelayData[readIndex0];
                    float sample1 = readDelayData[readIndex1];
                    float sample = sample0 + frac * (sample1 - sample0);

                    grainOutput += sample * envelope * grain.amplitude;
                    activeGrainCount++;

                    // Advance grain playback position
                    grain.readPosition += grain.playbackSpeed;
                    grain.age += 1.0;

                    // Wrap around buffer
                    if (grain.readPosition >= delayBufferSize)
                        grain.readPosition -= delayBufferSize;
                    else if (grain.readPosition < 0)
                        grain.readPosition += delayBufferSize;
                }
            }

            // Normalize grain output to prevent volume buildup
            if (activeGrainCount > 0)
                grainOutput *= 1.0f / std::sqrt(static_cast<float>(activeGrainCount));

            // For ping pong, cross-feed between channels
            if (isPingPong && buffer.getNumChannels() == 2)
            {
                int otherChannel = 1 - channel;
                auto* otherDelayData = delayBuffer.getWritePointer(otherChannel);
                int otherWriteIndex = writeIndex;

                // Write to opposite channel with feedback for ping pong effect
                otherDelayData[otherWriteIndex] = inputSample * 0.5f + grainOutput * feedback * 0.7f;
            }

            // Write grain output back to delay buffer with feedback
            delayData[writeIndex] = inputSample + grainOutput * feedback;

            // Output the grain result
            channelData[i] = grainOutput;

            // Apply soft clipping to prevent overflow
            channelData[i] = std::tanh(channelData[i]);
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
    auto reverbType = static_cast<int>(apvts.getRawParameterValue("reverbType")->load());

    if (reverbMix > 0.01f)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        juce::Reverb::Parameters reverbParams;

        // Configure reverb based on type
        switch (reverbType)
        {
            case 0: // Plate
                reverbParams.roomSize = 0.5f;
                reverbParams.damping = 0.3f;
                reverbParams.width = 0.95f;
                reverbParams.freezeMode = 0.0f;
                break;

            case 1: // Hall
                reverbParams.roomSize = 0.85f;
                reverbParams.damping = 0.6f;
                reverbParams.width = 1.0f;
                reverbParams.freezeMode = 0.0f;
                break;

            case 2: // Room
                reverbParams.roomSize = 0.3f;
                reverbParams.damping = 0.5f;
                reverbParams.width = 0.8f;
                reverbParams.freezeMode = 0.0f;
                break;

            default:
                reverbParams.roomSize = 0.5f;
                reverbParams.damping = 0.5f;
                reverbParams.width = 1.0f;
                reverbParams.freezeMode = 0.0f;
                break;
        }

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
