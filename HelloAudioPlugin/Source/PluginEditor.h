/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class HelloAudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    HelloAudioPluginAudioProcessorEditor (HelloAudioPluginAudioProcessor&);
    ~HelloAudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    virtual void timerCallback() override;

    HelloAudioPluginAudioProcessor& audioProcessor;

    std::unique_ptr<juce::Slider> sliderGain;
    std::unique_ptr<juce::Slider> sliderFrequency;
    std::unique_ptr<juce::ComboBox> comboboxOscillator;
    std::unique_ptr<juce::Label> labelGain;
    std::unique_ptr<juce::Label> labelFrequency;
    std::unique_ptr<juce::Label> labelOscillator;

    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachments;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboboxAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HelloAudioPluginAudioProcessorEditor)
};
