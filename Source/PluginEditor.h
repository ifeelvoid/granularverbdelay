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

        // Get the ACTUAL delay buffer being granulated
        auto& delayBuffer = processor.getDelayBuffer();
        int writePos = processor.getWritePosition();
        int bufferSize = delayBuffer.getNumSamples();

        if (bufferSize == 0) return;

        // Draw center line and grid
        g.setColour(juce::Colour(0xff2a2a2a));
        auto centerY = bounds.getCentreY();
        g.drawHorizontalLine(centerY, bounds.getX(), bounds.getRight());

        g.setColour(juce::Colour(0xff1f1f1f));
        for (int i = 1; i < 4; ++i)
        {
            auto y = bounds.getY() + (bounds.getHeight() * i / 4);
            g.drawHorizontalLine(y, bounds.getX(), bounds.getRight());
        }

        // Calculate how many samples to display (1 second of audio at most)
        int displaySamples = std::min(bufferSize, static_cast<int>(processor.getSampleRate() * 1.0));
        int startSample = (writePos - displaySamples + bufferSize) % bufferSize;

        // Draw waveform from the delay buffer with glow effect
        juce::Path waveformPath;
        waveformPath.startNewSubPath(bounds.getX(), centerY);

        // Downsample for display to avoid drawing too many points
        int step = std::max(1, displaySamples / bounds.getWidth());

        for (int i = 0; i < displaySamples; i += step)
        {
            int bufferIndex = (startSample + i) % bufferSize;
            float sample = delayBuffer.getSample(0, bufferIndex);

            // Apply soft clipping for display
            sample = std::tanh(sample * 2.0f) * 0.9f;

            auto x = juce::jmap(static_cast<float>(i), 0.0f, static_cast<float>(displaySamples),
                               static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));
            auto y = juce::jmap(sample, -1.0f, 1.0f,
                               static_cast<float>(bounds.getBottom()), static_cast<float>(bounds.getY()));

            waveformPath.lineTo(x, y);
        }

        waveformPath.lineTo(bounds.getRight(), centerY);
        waveformPath.closeSubPath();

        // Draw glow effect (multiple layers)
        g.setColour(juce::Colour(0xff00a8ff).withAlpha(0.15f));
        g.strokePath(waveformPath, juce::PathStrokeType(10.0f));

        g.setColour(juce::Colour(0xff00a8ff).withAlpha(0.4f));
        g.strokePath(waveformPath, juce::PathStrokeType(5.0f));

        // Draw bright waveform line
        g.setColour(juce::Colour(0xff00d4ff));
        g.strokePath(waveformPath, juce::PathStrokeType(2.5f));

        // Fill waveform area
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(0xff00a8ff).withAlpha(0.25f), bounds.getX(), bounds.getY(),
            juce::Colour(0xff00a8ff).withAlpha(0.05f), bounds.getX(), centerY, false));
        g.fillPath(waveformPath);

        // Draw write position indicator (playhead)
        float writeX = bounds.getRight() - 2.0f; // Write position is always at the right edge
        g.setColour(juce::Colour(0xff00ff00).withAlpha(0.6f));
        g.fillRect(writeX - 2.0f, static_cast<float>(bounds.getY()), 4.0f, static_cast<float>(bounds.getHeight()));

        // Draw grain positions - map them to the visible window
        auto& grainPositions = processor.getGrainPositions();

        for (size_t i = 0; i < grainPositions.size(); ++i)
        {
            auto grainPos = grainPositions[i];

            // Convert grain position to display position
            int grainSample = static_cast<int>(grainPos * bufferSize);
            int relativePos = (grainSample - startSample + bufferSize) % bufferSize;

            // Only draw if grain is in visible window
            if (relativePos < 0 || relativePos > displaySamples)
                continue;

            auto x = juce::jmap(static_cast<float>(relativePos), 0.0f, static_cast<float>(displaySamples),
                               static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));

            // Vary opacity for visual interest
            float alpha = 0.7f + 0.3f * (i % 3) / 3.0f;

            // Draw glow behind grain marker
            g.setColour(juce::Colour(0xffff6b35).withAlpha(alpha * 0.4f));
            g.fillEllipse(x - 12, bounds.getY() + 6, 24, 24);

            // Draw vertical line
            juce::ColourGradient gradient(
                juce::Colour(0xffff6b35).withAlpha(alpha),
                x, bounds.getY(),
                juce::Colour(0xffff6b35).withAlpha(alpha * 0.2f),
                x, bounds.getBottom(), false);
            g.setGradientFill(gradient);
            g.fillRect(x - 2.0f, static_cast<float>(bounds.getY()), 4.0f, static_cast<float>(bounds.getHeight()));

            // Draw grain marker circle
            g.setColour(juce::Colour(0xffff8c4f));
            g.fillEllipse(x - 6, bounds.getY() + 8, 12, 12);

            // Bright center dot
            g.setColour(juce::Colour(0xffffffff));
            g.fillEllipse(x - 3, bounds.getY() + 11, 6, 6);
        }

        // Draw border
        g.setColour(juce::Colour(0xff404040));
        g.drawRect(bounds, 2);

        // Draw info text
        g.setColour(juce::Colour(0xff888888));
        g.setFont(11.0f);
        g.drawText("← 1 second of delay buffer →", bounds.getX() + 5, bounds.getBottom() - 18, bounds.getWidth() - 10, 15, juce::Justification::centredLeft);
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
