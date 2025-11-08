#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

//==============================================================================
class WaveformDisplay : public juce::Component, public juce::Timer
{
public:
    WaveformDisplay(GranularVerbDelayAudioProcessor& p) : processor(p)
    {
        startTimerHz(30); // 30 FPS update rate
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Draw background
        g.fillAll(juce::Colour(0xff1a1a1a));

        // Draw border
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(bounds, 2);

        // Draw waveform
        auto& buffer = processor.getVisualizationBuffer();
        if (buffer.getNumSamples() > 0)
        {
            g.setColour(juce::Colour(0xff4a9eff));

            juce::Path waveformPath;
            auto numSamples = buffer.getNumSamples();
            auto width = bounds.getWidth();
            auto height = bounds.getHeight();
            auto centerY = bounds.getCentreY();

            // Draw left channel
            for (int i = 0; i < numSamples; ++i)
            {
                auto sample = buffer.getSample(0, i);
                auto x = juce::jmap(static_cast<float>(i), 0.0f, static_cast<float>(numSamples),
                                   static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));
                auto y = juce::jmap(sample, -1.0f, 1.0f,
                                   static_cast<float>(bounds.getBottom()), static_cast<float>(bounds.getY()));

                if (i == 0)
                    waveformPath.startNewSubPath(x, y);
                else
                    waveformPath.lineTo(x, y);
            }

            g.strokePath(waveformPath, juce::PathStrokeType(1.5f));
        }

        // Draw grain positions
        auto& grainPositions = processor.getGrainPositions();
        g.setColour(juce::Colour(0xffff6b6b).withAlpha(0.7f));

        for (auto grainPos : grainPositions)
        {
            auto x = bounds.getX() + grainPos * bounds.getWidth();

            // Draw vertical line for grain position
            g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 2.0f);

            // Draw circle at top
            g.fillEllipse(x - 4, bounds.getY() + 5, 8, 8);
        }

        // Draw grid
        g.setColour(juce::Colour(0xff2a2a2a));
        for (int i = 1; i < 4; ++i)
        {
            auto y = bounds.getY() + (bounds.getHeight() * i / 4);
            g.drawHorizontalLine(y, bounds.getX(), bounds.getRight());
        }
    }

    void timerCallback() override
    {
        repaint();
    }

private:
    GranularVerbDelayAudioProcessor& processor;
};

//==============================================================================
class GranularVerbDelayAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    GranularVerbDelayAudioProcessorEditor (GranularVerbDelayAudioProcessor&);
    ~GranularVerbDelayAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    GranularVerbDelayAudioProcessor& audioProcessor;

    // UI Components
    WaveformDisplay waveformDisplay;

    juce::Slider delayTimeSlider;
    juce::Slider grainSizeSlider;
    juce::Slider grainDensitySlider;
    juce::Slider reverbMixSlider;
    juce::Slider stereoWidthSlider;
    juce::Slider dryWetSlider;
    juce::Slider feedbackSlider;
    juce::Slider grainPitchSlider;

    juce::Label delayTimeLabel;
    juce::Label grainSizeLabel;
    juce::Label grainDensityLabel;
    juce::Label reverbMixLabel;
    juce::Label stereoWidthLabel;
    juce::Label dryWetLabel;
    juce::Label feedbackLabel;
    juce::Label grainPitchLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainDensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainPitchAttachment;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularVerbDelayAudioProcessorEditor)
};
