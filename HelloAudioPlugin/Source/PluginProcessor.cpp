/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout HelloAudioPluginAudioProcessor::createParameterLayout() const
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>("Gain", "Gain", juce::NormalisableRange<float>{ 0.0f, 1.0f, 0.01f }, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Frequency", "Frequency", juce::NormalisableRange<float>{ 20.0f, 2000.0f, 1.0f }, 440.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("Oscillator", "Oscillator", oscillatorTypes, 0));
    return layout;
}

//==============================================================================
HelloAudioPluginAudioProcessor::HelloAudioPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
    , apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
    , waveSampleCollector(waveDrawBuffer)
{
}

HelloAudioPluginAudioProcessor::~HelloAudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String HelloAudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HelloAudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool HelloAudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool HelloAudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double HelloAudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HelloAudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int HelloAudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void HelloAudioPluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String HelloAudioPluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void HelloAudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void HelloAudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void HelloAudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HelloAudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void HelloAudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const auto* freq_param = apvts.getParameter("Frequency");
    const float frequency = freq_param->getNormalisableRange().convertFrom0to1(freq_param->getValue());

    const auto* osc_param = apvts.getParameter("Oscillator");
    const OscillatorType oscillator = (OscillatorType)osc_param->getNormalisableRange().convertFrom0to1(osc_param->getValue());

    // Calculate phase delta for wanted frequency
    float phase = 0.0f;
    const float base_rad = juce::MathConstants<float>::twoPi / (float)getSampleRate();
    const float phase_delta = base_rad * frequency;

    // Render audio data
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        phase = lastPhase;
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            phase += phase_delta;

            if (phase > juce::MathConstants<float>::twoPi)
                phase -= juce::MathConstants<float>::twoPi;

            // Render sine wave
            switch (oscillator)
            {
            case kSine:
                channelData[sample] = sinf(phase);
                break;
            case kSquare:
                channelData[sample] = copysignf(1.0f, sinf(phase));
                break;
            case kTriangle:
                channelData[sample] = (acos(cos(phase)) / juce::MathConstants<float>::pi - 0.5f) * 2.0f;
                break;
            case kSaw:
                channelData[sample] = juce::jmap<float>(phase, 0.0f, juce::MathConstants<float>::twoPi, -1.0f, 1.0f);
                break;
            case kNoise:
                channelData[sample] = juce::jmap<float>(random.nextFloat(), -1.0f, 1.0f);
                break;
            default:
                break;
            }
        }
    }
    lastPhase = phase;

    // Apply gain
    const auto* gain_param = apvts.getParameter("Gain");
    const float gain = gain_param->getNormalisableRange().convertFrom0to1(gain_param->getValue());
    buffer.applyGainRamp(0, buffer.getNumSamples(), lastGain, gain);
    lastGain = gain;

    // Collect wave sample
    waveSampleCollector.process(buffer.getReadPointer(0), buffer.getNumSamples());
}

void HelloAudioPluginAudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::AudioProcessor::processBlockBypassed(buffer, midiMessages);

    // Collect wave sample
    waveSampleCollector.process(buffer.getReadPointer(0), buffer.getNumSamples());
}

//==============================================================================
bool HelloAudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* HelloAudioPluginAudioProcessor::createEditor()
{
    return new HelloAudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void HelloAudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void HelloAudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HelloAudioPluginAudioProcessor();
}
