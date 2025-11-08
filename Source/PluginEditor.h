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

        // Draw background with gradient
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(0xff0e0e0e), bounds.getX(), bounds.getY(),
            juce::Colour(0xff1a1a1a), bounds.getX(), bounds.getBottom(), false));
        g.fillRect(bounds);

        // Draw center line
        g.setColour(juce::Colour(0xff2a2a2a));
        auto centerY = bounds.getCentreY();
        g.drawHorizontalLine(centerY, bounds.getX(), bounds.getRight());

        // Draw grid lines
        g.setColour(juce::Colour(0xff1f1f1f));
        for (int i = 1; i < 4; ++i)
        {
            auto y = bounds.getY() + (bounds.getHeight() * i / 4);
            g.drawHorizontalLine(y, bounds.getX(), bounds.getRight());
        }

        // Draw waveform with glow effect
        auto& buffer = processor.getVisualizationBuffer();
        if (buffer.getNumSamples() > 0)
        {
            juce::Path waveformPath;
            auto numSamples = buffer.getNumSamples();

            // Draw filled waveform area
            waveformPath.startNewSubPath(bounds.getX(), centerY);

            for (int i = 0; i < numSamples; ++i)
            {
                auto sample = buffer.getSample(0, i);
                auto x = juce::jmap(static_cast<float>(i), 0.0f, static_cast<float>(numSamples),
                                   static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));
                auto y = juce::jmap(sample, -1.0f, 1.0f,
                                   static_cast<float>(bounds.getBottom()), static_cast<float>(bounds.getY()));

                waveformPath.lineTo(x, y);
            }

            waveformPath.lineTo(bounds.getRight(), centerY);
            waveformPath.closeSubPath();

            // Draw glow effect (multiple layers)
            g.setColour(juce::Colour(0xff00a8ff).withAlpha(0.1f));
            g.strokePath(waveformPath, juce::PathStrokeType(8.0f));

            g.setColour(juce::Colour(0xff00a8ff).withAlpha(0.3f));
            g.strokePath(waveformPath, juce::PathStrokeType(4.0f));

            // Draw bright waveform line
            g.setColour(juce::Colour(0xff00d4ff));
            g.strokePath(waveformPath, juce::PathStrokeType(2.5f));

            // Fill waveform area with subtle gradient
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(0xff00a8ff).withAlpha(0.2f), bounds.getX(), bounds.getY(),
                juce::Colour(0xff00a8ff).withAlpha(0.05f), bounds.getX(), centerY, false));
            g.fillPath(waveformPath);
        }

        // Draw grain positions with pulsing effect
        auto& grainPositions = processor.getGrainPositions();

        for (size_t i = 0; i < grainPositions.size(); ++i)
        {
            auto grainPos = grainPositions[i];
            auto x = bounds.getX() + grainPos * bounds.getWidth();

            // Vary opacity based on grain index for visual interest
            float alpha = 0.6f + 0.4f * (i % 3) / 3.0f;

            // Draw glow behind grain marker
            g.setColour(juce::Colour(0xffff6b35).withAlpha(alpha * 0.3f));
            g.fillEllipse(x - 8, bounds.getY() + 8, 16, 16);

            // Draw vertical line
            juce::ColourGradient gradient(
                juce::Colour(0xffff6b35).withAlpha(alpha),
                x, bounds.getY(),
                juce::Colour(0xffff6b35).withAlpha(alpha * 0.3f),
                x, bounds.getBottom(), false);
            g.setGradientFill(gradient);
            g.fillRect(x - 1.5f, static_cast<float>(bounds.getY()), 3.0f, static_cast<float>(bounds.getHeight()));

            // Draw grain marker circle
            g.setColour(juce::Colour(0xffff8c4f));
            g.fillEllipse(x - 5, bounds.getY() + 10, 10, 10);

            // Bright center dot
            g.setColour(juce::Colour(0xffffffff));
            g.fillEllipse(x - 2, bounds.getY() + 13, 4, 4);
        }

        // Draw border
        g.setColour(juce::Colour(0xff404040));
        g.drawRect(bounds, 2);
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
