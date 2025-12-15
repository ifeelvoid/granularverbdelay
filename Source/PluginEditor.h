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
        startTimerHz(15); // 15 FPS update rate - smoother, less frenetic
        setInterceptsMouseClicks(true, false);
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        // Check if clicking on spawn marker to drag it
        auto spawnX = getSpawnMarkerX();
        if (spawnX >= 0 && std::abs(event.x - spawnX) < 15)
        {
            isDraggingSpawnMarker = true;
            setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
        else
        {
            updateGrainPositionFromMouse(event);
        }
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        updateGrainPositionFromMouse(event);
    }

    void mouseUp(const juce::MouseEvent& event) override
    {
        isDraggingSpawnMarker = false;
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }

    void mouseEnter(const juce::MouseEvent&) override
    {
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        isDraggingSpawnMarker = false;
    }

    void mouseMove(const juce::MouseEvent& event) override
    {
        // Change cursor when hovering over spawn marker
        auto spawnX = getSpawnMarkerX();
        if (spawnX >= 0 && std::abs(event.x - spawnX) < 15)
        {
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        }
        else
        {
            setMouseCursor(juce::MouseCursor::CrosshairCursor);
        }
    }

    float getSpawnMarkerX()
    {
        auto bounds = getLocalBounds();
        int bufferSize = processor.getDelayBuffer().getNumSamples();
        if (bufferSize == 0) return -1;

        int writePos = processor.getWritePosition();
        int displaySamples = std::min(bufferSize, static_cast<int>(processor.getSampleRate() * 2.0));
        int startSample = (writePos - displaySamples + bufferSize) % bufferSize;

        auto filePosition = processor.getValueTreeState().getRawParameterValue("filePosition")->load();
        auto delayTime = processor.getValueTreeState().getRawParameterValue("delayTime")->load();

        int filePositionSamples = static_cast<int>(filePosition * delayTime * processor.getSampleRate());
        int fileReadPos = (writePos - filePositionSamples + bufferSize) % bufferSize;
        int relativeFilePos = (fileReadPos - startSample + bufferSize) % bufferSize;

        if (relativeFilePos >= 0 && relativeFilePos <= displaySamples)
        {
            return juce::jmap(static_cast<float>(relativeFilePos), 0.0f, static_cast<float>(displaySamples),
                            static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));
        }
        return -1;
    }

    void updateGrainPositionFromMouse(const juce::MouseEvent& event)
    {
        auto bounds = getLocalBounds();
        int bufferSize = processor.getDelayBuffer().getNumSamples();
        if (bufferSize == 0) return;

        // Calculate which position in the buffer was clicked
        // The display shows 2 seconds ending at writePos
        int writePos = processor.getWritePosition();
        int displaySamples = std::min(bufferSize, static_cast<int>(processor.getSampleRate() * 2.0));
        int startSample = (writePos - displaySamples + bufferSize) % bufferSize;

        // Convert mouse X position to sample position
        float normalizedX = juce::jmap(static_cast<float>(event.x),
                                       static_cast<float>(bounds.getX()),
                                       static_cast<float>(bounds.getRight()),
                                       0.0f, 1.0f);

        // Map to the visible window
        int clickedSample = static_cast<int>(normalizedX * displaySamples);
        int absoluteSample = (startSample + clickedSample) % bufferSize;

        // Convert to file position (0.0 = oldest, 1.0 = newest)
        // absoluteSample is where user clicked, writePos is newest
        int samplesFromWrite = (writePos - absoluteSample + bufferSize) % bufferSize;

        // Get delay time to scale properly
        auto delayTime = processor.getValueTreeState().getRawParameterValue("delayTime")->load();
        int maxDelaySamples = static_cast<int>(delayTime * processor.getSampleRate());

        // Calculate file position (how far back from write position)
        float filePosition = 1.0f - (static_cast<float>(samplesFromWrite) / maxDelaySamples);
        filePosition = juce::jlimit(0.0f, 1.0f, filePosition);

        // Set the file position parameter
        if (auto* param = processor.getValueTreeState().getParameter("filePosition"))
        {
            param->setValueNotifyingHost(filePosition);
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Draw dark background
        g.setColour(juce::Colour(0xff0a0a0a));
        g.fillRect(bounds);

        // Get the ACTUAL delay buffer being granulated
        auto& delayBuffer = processor.getDelayBuffer();
        int writePos = processor.getWritePosition();
        int bufferSize = delayBuffer.getNumSamples();

        if (bufferSize == 0) return;

        // Calculate how many samples to display (2 seconds of audio)
        int displaySamples = std::min(bufferSize, static_cast<int>(processor.getSampleRate() * 2.0));
        int startSample = (writePos - displaySamples + bufferSize) % bufferSize;

        // Split display into stereo channels
        auto leftChannelBounds = bounds.removeFromTop(bounds.getHeight() / 2).reduced(4, 2);
        auto rightChannelBounds = bounds.reduced(4, 2);

        // Draw both channels
        drawChannel(g, leftChannelBounds, delayBuffer, 0, startSample, displaySamples, bufferSize, "L");
        drawChannel(g, rightChannelBounds, delayBuffer, 1, startSample, displaySamples, bufferSize, "R");

        // Draw spray zone overlay (like Ableton Granulator)
        auto filePosition = processor.getValueTreeState().getRawParameterValue("filePosition")->load();
        auto delayTime = processor.getValueTreeState().getRawParameterValue("delayTime")->load();
        auto spray = processor.getValueTreeState().getRawParameterValue("spray")->load();

        // Calculate spawn position and spray zone
        int filePositionSamples = static_cast<int>(filePosition * delayTime * processor.getSampleRate());
        int fileReadPos = (writePos - filePositionSamples + bufferSize) % bufferSize;
        int relativeFilePos = (fileReadPos - startSample + bufferSize) % bufferSize;

        // Convert spray from milliseconds to samples
        int spraySamples = static_cast<int>(spray * processor.getSampleRate() / 1000.0f);

        if (relativeFilePos >= 0 && relativeFilePos <= displaySamples)
        {
            auto fullBounds = getLocalBounds();

            // Calculate spray zone boundaries
            int sprayStart = std::max(0, relativeFilePos - spraySamples);
            int sprayEnd = std::min(displaySamples, relativeFilePos + spraySamples);

            float sprayStartX = juce::jmap(static_cast<float>(sprayStart), 0.0f, static_cast<float>(displaySamples),
                                          static_cast<float>(fullBounds.getX()), static_cast<float>(fullBounds.getRight()));
            float sprayEndX = juce::jmap(static_cast<float>(sprayEnd), 0.0f, static_cast<float>(displaySamples),
                                        static_cast<float>(fullBounds.getX()), static_cast<float>(fullBounds.getRight()));

            // Draw spray zone as translucent overlay
            juce::Rectangle<float> sprayZone(sprayStartX, fullBounds.getY(),
                                            sprayEndX - sprayStartX, fullBounds.getHeight());

            // Gradient overlay for spray zone
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(0xffff00ff).withAlpha(0.15f), sprayZone.getCentreX(), fullBounds.getY(),
                juce::Colour(0xffff00ff).withAlpha(0.05f), sprayZone.getCentreX(), fullBounds.getBottom(), false));
            g.fillRect(sprayZone);

            // Draw spray zone borders
            g.setColour(juce::Colour(0xffff00ff).withAlpha(0.4f));
            g.drawVerticalLine(sprayStartX, fullBounds.getY(), fullBounds.getBottom());
            g.drawVerticalLine(sprayEndX, fullBounds.getY(), fullBounds.getBottom());

            // Draw dotted lines for spray boundaries
            float dash[] = {3.0f, 3.0f};
            g.setColour(juce::Colour(0xffff00ff).withAlpha(0.6f));
            for (float y = fullBounds.getY(); y < fullBounds.getBottom(); y += 6.0f)
            {
                g.drawLine(sprayStartX, y, sprayStartX, y + 3.0f, 1.0f);
                g.drawLine(sprayEndX, y, sprayEndX, y + 3.0f, 1.0f);
            }
        }

        // Draw grain positions on top
        auto fullBounds = getLocalBounds();
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
                               static_cast<float>(fullBounds.getX()), static_cast<float>(fullBounds.getRight()));

            // Draw bright vertical line for grain position
            g.setColour(juce::Colour(0xffff9500).withAlpha(0.85f));
            g.drawLine(x, fullBounds.getY(), x, fullBounds.getBottom(), 2.0f);

            // Draw marker dot at top
            g.setColour(juce::Colour(0xffffaa00));
            g.fillEllipse(x - 4, fullBounds.getY() + 4, 8, 8);
            g.setColour(juce::Colour(0xffffffff));
            g.fillEllipse(x - 2, fullBounds.getY() + 6, 4, 4);
        }

        // Draw playhead at write position (right edge)
        float writeX = fullBounds.getRight() - 4.0f;
        g.setColour(juce::Colour(0xff00ff00).withAlpha(0.8f));
        g.drawLine(writeX, fullBounds.getY(), writeX, fullBounds.getBottom(), 3.0f);

        // Draw file position marker (where grains spawn) - Ableton style
        filePosition = processor.getValueTreeState().getRawParameterValue("filePosition")->load();
        delayTime = processor.getValueTreeState().getRawParameterValue("delayTime")->load();

        // Calculate where file position appears on screen
        filePositionSamples = static_cast<int>(filePosition * delayTime * processor.getSampleRate());
        fileReadPos = (writePos - filePositionSamples + bufferSize) % bufferSize;
        relativeFilePos = (fileReadPos - startSample + bufferSize) % bufferSize;

        if (relativeFilePos >= 0 && relativeFilePos <= displaySamples)
        {
            auto fileX = juce::jmap(static_cast<float>(relativeFilePos), 0.0f, static_cast<float>(displaySamples),
                                   static_cast<float>(fullBounds.getX()), static_cast<float>(fullBounds.getRight()));

            // Draw thick bright marker line
            g.setColour(juce::Colour(0xffff00ff));
            g.drawLine(fileX, fullBounds.getY(), fileX, fullBounds.getBottom(), 3.0f);

            // Draw draggable handle at top (triangle)
            juce::Path triangleHandle;
            triangleHandle.addTriangle(fileX - 8, fullBounds.getY() + 2,
                                      fileX + 8, fullBounds.getY() + 2,
                                      fileX, fullBounds.getY() + 14);

            g.setColour(juce::Colour(0xffff00ff));
            g.fillPath(triangleHandle);

            // White outline for handle
            g.setColour(juce::Colour(0xffffffff));
            g.strokePath(triangleHandle, juce::PathStrokeType(1.5f));

            // Draw label at bottom
            g.setColour(juce::Colour(0xffff00ff));
            g.setFont(juce::Font(11.0f, juce::Font::bold));

            // Background for label
            juce::Rectangle<float> labelBg(fileX - 28, fullBounds.getBottom() - 18, 56, 16);
            g.setColour(juce::Colour(0xff000000).withAlpha(0.7f));
            g.fillRoundedRectangle(labelBg, 3.0f);

            g.setColour(juce::Colour(0xffff00ff));
            g.drawText("SPAWN", fileX - 28, fullBounds.getBottom() - 18, 56, 16, juce::Justification::centred);
        }

        // Draw border
        g.setColour(juce::Colour(0xff404040));
        g.drawRect(getLocalBounds(), 2);

        // Draw info text with better instructions
        g.setColour(juce::Colour(0xffaaaaaa));
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText("← 2 SECONDS →", fullBounds.getX() + 8, fullBounds.getY() + 4, 100, 12, juce::Justification::left);

        g.setColour(juce::Colour(0xffff00ff).withAlpha(0.8f));
        g.drawText("DRAG SPAWN MARKER OR CLICK", fullBounds.getRight() - 195, fullBounds.getY() + 4, 187, 12, juce::Justification::right);

        // Show spray zone width
        g.setColour(juce::Colour(0xffaaaaaa).withAlpha(0.8f));
        g.setFont(juce::Font(9.0f));
        auto sprayValue = processor.getValueTreeState().getRawParameterValue("spray")->load();
        g.drawText(juce::String("Spray: ±") + juce::String(sprayValue, 1) + " ms",
                  fullBounds.getX() + 8, fullBounds.getY() + 18, 120, 12, juce::Justification::left);
    }

    void drawChannel(juce::Graphics& g, juce::Rectangle<int> bounds,
                     const juce::AudioBuffer<float>& buffer, int channel,
                     int startSample, int displaySamples, int bufferSize, const juce::String& label)
    {
        if (bounds.getWidth() <= 0 || bounds.getHeight() <= 0) return;

        auto centerY = bounds.getCentreY();

        // Draw grid lines
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawHorizontalLine(centerY, bounds.getX(), bounds.getRight());
        g.drawHorizontalLine(bounds.getY() + bounds.getHeight() * 0.25f, bounds.getX(), bounds.getRight());
        g.drawHorizontalLine(bounds.getY() + bounds.getHeight() * 0.75f, bounds.getX(), bounds.getRight());

        // Draw channel label
        g.setColour(juce::Colour(0xff666666));
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(label, bounds.getX() + 4, bounds.getY() + 2, 20, 12, juce::Justification::left);

        // Calculate samples per pixel for RMS/peak display
        int samplesPerPixel = std::max(1, displaySamples / bounds.getWidth());

        // Draw waveform bars (DAW-style)
        for (int x = 0; x < bounds.getWidth(); ++x)
        {
            // Calculate which samples this pixel represents
            int sampleStart = x * samplesPerPixel;
            int sampleEnd = std::min(sampleStart + samplesPerPixel, displaySamples);

            // Find peak and RMS for this range
            float peak = 0.0f;
            float rms = 0.0f;

            for (int s = sampleStart; s < sampleEnd; ++s)
            {
                int bufferIndex = (startSample + s) % bufferSize;
                float sample = buffer.getSample(channel, bufferIndex);

                peak = std::max(peak, std::abs(sample));
                rms += sample * sample;
            }

            rms = std::sqrt(rms / (sampleEnd - sampleStart));

            // Normalize and clip
            peak = std::min(1.0f, peak);
            rms = std::min(1.0f, rms);

            // Calculate bar heights
            float peakHeight = peak * (bounds.getHeight() / 2.0f);
            float rmsHeight = rms * (bounds.getHeight() / 2.0f);

            float pixelX = bounds.getX() + x;

            // Draw peak (brighter, thinner)
            if (peak > 0.01f)
            {
                // Color based on level (green -> yellow -> red)
                juce::Colour peakColor;
                if (peak < 0.7f)
                    peakColor = juce::Colour(0xff00d4ff); // Cyan for normal levels
                else if (peak < 0.9f)
                    peakColor = juce::Colour(0xffffa500); // Orange for hot levels
                else
                    peakColor = juce::Colour(0xffff3333); // Red for clipping

                // Draw positive peak
                g.setColour(peakColor.withAlpha(0.9f));
                g.drawVerticalLine(pixelX, centerY - peakHeight, centerY);

                // Draw negative peak
                g.drawVerticalLine(pixelX, centerY, centerY + peakHeight);
            }

            // Draw RMS (dimmer, shows average level)
            if (rms > 0.005f)
            {
                g.setColour(juce::Colour(0xff008fb3).withAlpha(0.6f));
                g.drawVerticalLine(pixelX, centerY - rmsHeight, centerY + rmsHeight);
            }
        }
    }

    void timerCallback() override
    {
        repaint();
    }

private:
    GranularVerbDelayAudioProcessor& processor;
    bool isDraggingSpawnMarker = false;
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
    juce::Slider grainPitchSlider;
    juce::Slider spraySlider;
    juce::Slider grainSizeVarSlider;
    juce::Slider grainPitchVarSlider;
    juce::Slider filePositionSlider;
    juce::Slider reverbMixSlider;
    juce::Slider stereoWidthSlider;
    juce::Slider dryWetSlider;
    juce::Slider feedbackSlider;

    juce::ComboBox delayModeBox;
    juce::ComboBox reverbTypeBox;

    juce::Label delayTimeLabel;
    juce::Label grainSizeLabel;
    juce::Label grainDensityLabel;
    juce::Label grainPitchLabel;
    juce::Label sprayLabel;
    juce::Label grainSizeVarLabel;
    juce::Label grainPitchVarLabel;
    juce::Label filePositionLabel;
    juce::Label reverbMixLabel;
    juce::Label stereoWidthLabel;
    juce::Label dryWetLabel;
    juce::Label feedbackLabel;
    juce::Label delayModeLabel;
    juce::Label reverbTypeLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainDensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainPitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sprayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainSizeVarAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> grainPitchVarAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filePositionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> delayModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> reverbTypeAttachment;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText);
    void setupComboBox(juce::ComboBox& comboBox, juce::Label& label, const juce::String& labelText, const juce::StringArray& items);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularVerbDelayAudioProcessorEditor)
};
