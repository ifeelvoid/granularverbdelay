#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GranularVerbDelayAudioProcessorEditor::GranularVerbDelayAudioProcessorEditor (GranularVerbDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), waveformDisplay(p)
{
    // Setup waveform display
    addAndMakeVisible(waveformDisplay);

    // Setup sliders and labels
    setupSlider(delayTimeSlider, delayTimeLabel, "Delay Time");
    setupSlider(grainSizeSlider, grainSizeLabel, "Grain Size");
    setupSlider(grainDensitySlider, grainDensityLabel, "Grain Density");
    setupSlider(reverbMixSlider, reverbMixLabel, "Reverb Mix");
    setupSlider(stereoWidthSlider, stereoWidthLabel, "Stereo Width");
    setupSlider(dryWetSlider, dryWetLabel, "Dry/Wet");
    setupSlider(feedbackSlider, feedbackLabel, "Feedback");
    setupSlider(grainPitchSlider, grainPitchLabel, "Grain Pitch");

    // Create attachments
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "delayTime", delayTimeSlider);
    grainSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "grainSize", grainSizeSlider);
    grainDensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "grainDensity", grainDensitySlider);
    reverbMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "reverbMix", reverbMixSlider);
    stereoWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "stereoWidth", stereoWidthSlider);
    dryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "dryWet", dryWetSlider);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "feedback", feedbackSlider);
    grainPitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "grainPitch", grainPitchSlider);

    // Set component size
    setSize (800, 600);
}

GranularVerbDelayAudioProcessorEditor::~GranularVerbDelayAudioProcessorEditor()
{
}

void GranularVerbDelayAudioProcessorEditor::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 85, 22);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f,
                                true);

    addAndMakeVisible(label);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&slider, false);

    // Modern styling with gradients
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00d4ff));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2a2a));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffffff));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdddddd));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1a1a1a));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3a3a3a));

    label.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    label.setFont(juce::Font(14.0f, juce::Font::bold));

    // Sensitivity adjustment for better feel
    slider.setMouseDragSensitivity(150);
}

//==============================================================================
void GranularVerbDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background with subtle gradient
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff0a0a0a), 0, 0,
        juce::Colour(0xff151515), 0, getHeight(), false));
    g.fillAll();

    auto bounds = getLocalBounds();

    // Draw title with glow
    g.setColour(juce::Colour(0xff00d4ff).withAlpha(0.1f));
    g.setFont(juce::Font(32.0f, juce::Font::bold));
    g.drawText("GRANULAR VERB DELAY", bounds.getX(), 10, bounds.getWidth(), 50,
               juce::Justification::centred);

    g.setColour(juce::Colour(0xffffffff));
    g.setFont(juce::Font(28.0f, juce::Font::bold));
    g.drawText("GRANULAR VERB DELAY", bounds.getX(), 12, bounds.getWidth(), 50,
               juce::Justification::centred);

    // Draw control sections with subtle borders
    g.setColour(juce::Colour(0xff1f1f1f));
    g.fillRoundedRectangle(10, 250, getWidth() - 20, 330, 4.0f);

    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRoundedRectangle(10, 250, getWidth() - 20, 330, 4.0f, 1.0f);
}

void GranularVerbDelayAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(60); // Title space

    // Waveform display at top
    waveformDisplay.setBounds(bounds.removeFromTop(180).reduced(10));

    bounds.removeFromTop(10); // Spacing

    // Control section
    auto controlBounds = bounds.reduced(20);

    // Create 2 rows of 4 controls
    auto row1 = controlBounds.removeFromTop(150);
    auto row2 = controlBounds.removeFromTop(150);

    auto sliderWidth = row1.getWidth() / 4;

    // Row 1: Delay, Grain Size, Grain Density, Grain Pitch
    delayTimeSlider.setBounds(row1.removeFromLeft(sliderWidth).reduced(10));
    grainSizeSlider.setBounds(row1.removeFromLeft(sliderWidth).reduced(10));
    grainDensitySlider.setBounds(row1.removeFromLeft(sliderWidth).reduced(10));
    grainPitchSlider.setBounds(row1.removeFromLeft(sliderWidth).reduced(10));

    // Row 2: Reverb, Stereo Width, Feedback, Dry/Wet
    reverbMixSlider.setBounds(row2.removeFromLeft(sliderWidth).reduced(10));
    stereoWidthSlider.setBounds(row2.removeFromLeft(sliderWidth).reduced(10));
    feedbackSlider.setBounds(row2.removeFromLeft(sliderWidth).reduced(10));
    dryWetSlider.setBounds(row2.removeFromLeft(sliderWidth).reduced(10));
}
