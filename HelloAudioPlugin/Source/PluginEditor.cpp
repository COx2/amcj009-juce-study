/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
HelloAudioPluginAudioProcessorEditor::HelloAudioPluginAudioProcessorEditor (HelloAudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
    , sliderGain(std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow))
    , sliderFrequency(std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow))
    , comboboxOscillator(std::make_unique<juce::ComboBox>())
    , labelGain(std::make_unique<juce::Label>("Gain", "GAIN"))
    , labelFrequency(std::make_unique<juce::Label>("Frequency", "FREQ"))
    , labelOscillator(std::make_unique<juce::Label>("Oscillator", "OSC"))
{
    labelGain->attachToComponent(sliderGain.get(), false);
    labelGain->setJustificationType(juce::Justification::centred);

    sliderAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.getProcessorState(), "Gain", *sliderGain));
    addAndMakeVisible(sliderGain.get());

    labelFrequency->attachToComponent(sliderFrequency.get(), false);
    labelFrequency->setJustificationType(juce::Justification::centred);

    sliderAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.getProcessorState(), "Frequency", *sliderFrequency));
    addAndMakeVisible(sliderFrequency.get());

    labelOscillator->attachToComponent(comboboxOscillator.get(), false);
    labelOscillator->setJustificationType(juce::Justification::centred);

    comboboxAttachments.add(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        (audioProcessor.getProcessorState(),"Oscillator", *comboboxOscillator));
    comboboxOscillator->setJustificationType(juce::Justification::centred);
    comboboxOscillator->addItemList(audioProcessor.getProcessorState().getParameter("Oscillator")->getAllValueStrings(), 1);
    comboboxOscillator->setText(audioProcessor.getProcessorState().getParameter("Oscillator")->getCurrentValueAsText());
    addAndMakeVisible(comboboxOscillator.get());

    setSize (600, 400);

    startTimerHz(30);
}

HelloAudioPluginAudioProcessorEditor::~HelloAudioPluginAudioProcessorEditor()
{
    sliderAttachments.clear();
    comboboxAttachments.clear();
}

//==============================================================================
void drawWaveShape(juce::Graphics& g, const juce::Rectangle<float>& drawArea, const float* plotData, const int numSamples)
{
    juce::Path wavePath;
    const float x0 = drawArea.getX();
    const float cloped_sample0 = juce::jmax<float>(-1.0f, juce::jmin<float>(1.0f, plotData[0]));
    const float y0 = juce::jmap<float>(cloped_sample0, -1.0f, 1.0f, drawArea.getBottom(), drawArea.getY());
    wavePath.startNewSubPath(x0, y0);

    for (int i = 1; i < numSamples; ++i)
    {
        const float x = juce::jmap<float>(i, 0, numSamples, x0, x0 + drawArea.getWidth());
        const float cloped_sample = juce::jmax<float>(-1.0f, juce::jmin<float>(1.0f, plotData[i]));
        const float y = juce::jmap<float>(cloped_sample, -1.0f, 1.0f, drawArea.getBottom(), drawArea.getY());
        wavePath.lineTo(x, y);
    }
    g.setColour(juce::Colours::cyan);
    g.strokePath(wavePath, juce::PathStrokeType(2.0f));
}

void HelloAudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    const auto bounds = getLocalBounds();

    // Draw text
    const juce::Rectangle<float> textArea = { bounds.getWidth() * 0.1f, bounds.getHeight() * 0.02f, bounds.getWidth() * 0.8f, bounds.getHeight() * 0.1f };
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("Hello Audio Plugin", textArea.toNearestInt(), juce::Justification::centred, 1);

    // Draw wave shape background
    const juce::Rectangle<float> drawArea = { bounds.getWidth() * 0.1f, bounds.getHeight() * 0.6f, bounds.getWidth() * 0.8f, bounds.getHeight() * 0.3f };
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(drawArea);

    // Draw wave shape
    juce::AudioBuffer<float> samplesToDraw(1, audioProcessor.getWaveDrawBuffer().getBufferSize());
    audioProcessor.getWaveDrawBuffer().pop(samplesToDraw.getWritePointer(0), samplesToDraw.getNumSamples());
    drawWaveShape(g, drawArea, samplesToDraw.getWritePointer(0), samplesToDraw.getNumSamples());
}

void HelloAudioPluginAudioProcessorEditor::resized()
{
    const auto bounds = getLocalBounds();

    const int slider_w = bounds.getWidth() * 0.3f;
    const int slider_h = bounds.getHeight() * 0.3f;
    const int combobox_w = bounds.getWidth() * 0.2f;
    const int combobox_h = bounds.getHeight() * 0.1f;

    sliderGain->setSize(slider_w, slider_h);
    sliderGain->setCentrePosition(bounds.getWidth() * 0.2f, bounds.getHeight() * 0.3f);

    sliderFrequency->setSize(slider_w, slider_h);
    sliderFrequency->setCentrePosition(bounds.getWidth() * 0.8f, bounds.getHeight() * 0.3f);

    comboboxOscillator->setSize(combobox_w, combobox_h);
    comboboxOscillator->setCentrePosition(bounds.getWidth() * 0.5f, bounds.getHeight() * 0.3f);
}

void HelloAudioPluginAudioProcessorEditor::timerCallback()
{
    repaint();
}
