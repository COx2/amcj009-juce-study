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
{
    sliderGain->setRange(0.0, 1.0);
    addAndMakeVisible(sliderGain.get());

    sliderAttachments.add(std::make_unique<juce::SliderParameterAttachment>
        (*audioProcessor.getProcessorState().getParameter("Gain"), *sliderGain));

    setSize (400, 300);
}

HelloAudioPluginAudioProcessorEditor::~HelloAudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void HelloAudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello Audio Plugin", getLocalBounds(), juce::Justification::centred, 1);
}

void HelloAudioPluginAudioProcessorEditor::resized()
{
    const auto panelCenter = getBounds().getCentre();

    sliderGain->setSize(300, 300);
    sliderGain->setCentrePosition(panelCenter);
}
