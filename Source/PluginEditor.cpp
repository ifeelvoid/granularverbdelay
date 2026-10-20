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
    setupSlider(grainPitchSlider, grainPitchLabel, "Grain Pitch");
    setupSlider(spraySlider, sprayLabel, "Spray");
    setupSlider(grainSizeVarSlider, grainSizeVarLabel, "Size Var");
    setupSlider(grainPitchVarSlider, grainPitchVarLabel, "Pitch Var");
    setupSlider(filePositionSlider, filePositionLabel, "File Position");
    setupSlider(reverbMixSlider, reverbMixLabel, "Reverb Mix");
    setupSlider(stereoWidthSlider, stereoWidthLabel, "Stereo Width");
    setupSlider(dryWetSlider, dryWetLabel, "Dry/Wet");
    setupSlider(feedbackSlider, feedbackLabel, "Feedback");

    // Setup combo boxes
    setupComboBox(delayModeBox, delayModeLabel, "Delay Mode", juce::StringArray("Stereo", "Ping Pong"));
    setupComboBox(reverbTypeBox, reverbTypeLabel, "Reverb Type", juce::StringArray("Plate", "Hall", "Room"));
    setupComboBox(grainDirectionBox, grainDirectionLabel, "Grain Direction", juce::StringArray("Forward", "Reverse", "Random"));

    // Setup freeze button
    setupButton(reverbFreezeButton, reverbFreezeLabel, "Reverb Freeze");

    // Setup preset controls
    addAndMakeVisible(savePresetButton);
    savePresetButton.setButtonText("Save Preset");
    savePresetButton.onClick = [this] { savePreset(); };
    savePresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00d4ff));
    savePresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff000000));

    addAndMakeVisible(loadPresetButton);
    loadPresetButton.setButtonText("Load");
    loadPresetButton.onClick = [this] {
        auto index = presetBox.getSelectedItemIndex();
        if (index >= 0) loadPreset(index);
    };
    loadPresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    loadPresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffdddddd));

    addAndMakeVisible(presetBox);
    presetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    presetBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffdddddd));
    presetBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));
    refreshPresetList();

    // Create attachments
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "delayTime", delayTimeSlider);
    grainSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "grainSize", grainSizeSlider);
    grainDensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "grainDensity", grainDensitySlider);
    grainPitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "grainPitch", grainPitchSlider);
    sprayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "spray", spraySlider);
    grainSizeVarAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "grainSizeVar", grainSizeVarSlider);
    grainPitchVarAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "grainPitchVar", grainPitchVarSlider);
    filePositionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "filePosition", filePositionSlider);
    reverbMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "reverbMix", reverbMixSlider);
    stereoWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "stereoWidth", stereoWidthSlider);
    dryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "dryWet", dryWetSlider);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "feedback", feedbackSlider);
    delayModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "delayMode", delayModeBox);
    reverbTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "reverbType", reverbTypeBox);
    grainDirectionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "grainDirection", grainDirectionBox);
    reverbFreezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), "reverbFreeze", reverbFreezeButton);

    // Set component size - taller to fit more controls (added preset bar)
    setSize (800, 780);

    // Setup window resizing
    resizeConstraints.setMinimumSize(600, 540);  // Minimum usable size
    resizeConstraints.setMaximumSize(1600, 1200); // Maximum size

    resizer = std::make_unique<juce::ResizableCornerComponent>(this, &resizeConstraints);
    addAndMakeVisible(resizer.get());

    // Make the window resizable
    setResizable(true, true);
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

void GranularVerbDelayAudioProcessorEditor::setupComboBox(juce::ComboBox& comboBox, juce::Label& label, const juce::String& labelText, const juce::StringArray& items)
{
    addAndMakeVisible(comboBox);
    comboBox.addItemList(items, 1);
    comboBox.setSelectedItemIndex(0);

    addAndMakeVisible(label);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&comboBox, false);

    // Modern styling
    comboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    comboBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffdddddd));
    comboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));
    comboBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff00d4ff));
    comboBox.setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff2a2a2a));

    label.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    label.setFont(juce::Font(14.0f, juce::Font::bold));
}

void GranularVerbDelayAudioProcessorEditor::setupButton(juce::TextButton& button, juce::Label& label, const juce::String& labelText)
{
    addAndMakeVisible(button);
    button.setClickingTogglesState(true);

    addAndMakeVisible(label);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&button, false);

    // Modern styling
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff00ff));
    button.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffdddddd));
    button.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff000000));

    label.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    label.setFont(juce::Font(14.0f, juce::Font::bold));
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
    g.fillRoundedRectangle(10, 250, getWidth() - 20, 470, 4.0f);

    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRoundedRectangle(10, 250, getWidth() - 20, 470, 4.0f, 1.0f);
}

juce::File GranularVerbDelayAudioProcessorEditor::getPresetDirectory()
{
    auto presetDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("GranularVerbDelay").getChildFile("Presets");
    if (!presetDir.exists())
        presetDir.createDirectory();
    return presetDir;
}

void GranularVerbDelayAudioProcessorEditor::savePreset()
{
    juce::FileChooser chooser("Save Preset", getPresetDirectory(), "*.gvd");
    if (chooser.browseForFileToSave(true))
    {
        auto file = chooser.getResult();
        juce::MemoryBlock data;
        audioProcessor.getStateInformation(data);
        file.replaceWithData(data.getData(), data.getSize());
        refreshPresetList();
    }
}

void GranularVerbDelayAudioProcessorEditor::loadPreset(int presetIndex)
{
    auto presetFiles = getPresetDirectory().findChildFiles(juce::File::findFiles, false, "*.gvd");
    if (presetIndex >= 0 && presetIndex < presetFiles.size())
    {
        auto file = presetFiles[presetIndex];
        juce::MemoryBlock data;
        if (file.loadFileAsData(data))
        {
            audioProcessor.setStateInformation(data.getData(), (int)data.getSize());
        }
    }
}

void GranularVerbDelayAudioProcessorEditor::refreshPresetList()
{
    presetBox.clear();
    auto presetFiles = getPresetDirectory().findChildFiles(juce::File::findFiles, false, "*.gvd");
    for (int i = 0; i < presetFiles.size(); ++i)
    {
        presetBox.addItem(presetFiles[i].getFileNameWithoutExtension(), i + 1);
    }
    if (presetFiles.size() == 0)
        presetBox.addItem("No presets", 1);
}

void GranularVerbDelayAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(60); // Title space

    // Preset controls at very top
    auto presetBar = bounds.removeFromTop(35).reduced(10, 5);
    savePresetButton.setBounds(presetBar.removeFromLeft(100));
    presetBar.removeFromLeft(5);
    presetBox.setBounds(presetBar.removeFromLeft(presetBar.getWidth() - 55));
    presetBar.removeFromLeft(5);
    loadPresetButton.setBounds(presetBar);

    bounds.removeFromTop(5); // Small spacing

    // Waveform display at top
    waveformDisplay.setBounds(bounds.removeFromTop(180).reduced(10));

    bounds.removeFromTop(10); // Spacing

    // Control section
    auto controlBounds = bounds.reduced(20);

    // Create 3 rows of 4 controls each + 1 row for combo boxes
    auto row1 = controlBounds.removeFromTop(140);
    auto row2 = controlBounds.removeFromTop(140);
    auto row3 = controlBounds.removeFromTop(140);
    auto row4 = controlBounds.removeFromTop(60);

    auto sliderWidth = row1.getWidth() / 4;

    // Row 1: Core Granular Controls
    delayTimeSlider.setBounds(row1.removeFromLeft(sliderWidth).reduced(10));
    grainSizeSlider.setBounds(row1.removeFromLeft(sliderWidth).reduced(10));
    grainDensitySlider.setBounds(row1.removeFromLeft(sliderWidth).reduced(10));
    grainPitchSlider.setBounds(row1.removeFromLeft(sliderWidth).reduced(10));

    // Row 2: Granulator III-Style Controls (Spray, Variations, Position)
    spraySlider.setBounds(row2.removeFromLeft(sliderWidth).reduced(10));
    grainSizeVarSlider.setBounds(row2.removeFromLeft(sliderWidth).reduced(10));
    grainPitchVarSlider.setBounds(row2.removeFromLeft(sliderWidth).reduced(10));
    filePositionSlider.setBounds(row2.removeFromLeft(sliderWidth).reduced(10));

    // Row 3: Mix/Effects Controls
    reverbMixSlider.setBounds(row3.removeFromLeft(sliderWidth).reduced(10));
    stereoWidthSlider.setBounds(row3.removeFromLeft(sliderWidth).reduced(10));
    feedbackSlider.setBounds(row3.removeFromLeft(sliderWidth).reduced(10));
    dryWetSlider.setBounds(row3.removeFromLeft(sliderWidth).reduced(10));

    // Row 4: Mode/Type Selectors (combo boxes and button)
    auto comboWidth = row4.getWidth() / 4;
    delayModeBox.setBounds(row4.removeFromLeft(comboWidth).reduced(10, 5));
    reverbTypeBox.setBounds(row4.removeFromLeft(comboWidth).reduced(10, 5));
    grainDirectionBox.setBounds(row4.removeFromLeft(comboWidth).reduced(10, 5));
    reverbFreezeButton.setBounds(row4.removeFromLeft(comboWidth).reduced(10, 5));

    // Position resize corner
    if (resizer)
        resizer->setBounds(getWidth() - 16, getHeight() - 16, 16, 16);
}
