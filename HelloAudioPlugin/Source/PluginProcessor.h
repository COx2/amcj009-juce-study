/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
class WaveDrawBuffer
{
public:
    WaveDrawBuffer()
    {
        bufferToDraw.clear();
    }

    void push(const float* dataToPush, const int numSamples)
    {
        jassert(numSamples <= bufferToDraw.getNumSamples());

        int start1, size1, start2, size2;

        abstractFifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0)
            juce::FloatVectorOperations::copy(bufferToDraw.getWritePointer(start1), dataToPush, numSamples);

        abstractFifo.finishedWrite(size1);
    }

    void pop(float* outputBuffer, const int numSamples)
    {
        jassert(numSamples <= bufferToDraw.getNumSamples());

        int start1, size1, start2, size2;
        abstractFifo.prepareToRead(1, start1, size1, start2, size2);

        if (size1 > 0)
        {
            juce::FloatVectorOperations::copy(outputBuffer, bufferToDraw.getReadPointer(start1), numSamples);
            lastReadPointer = start1;
            resetCount = 0;
        }
        else
        {
            resetCount++;
            if (resetCount > resetThreshold)
                bufferToDraw.clear();

            juce::FloatVectorOperations::copy(outputBuffer, bufferToDraw.getReadPointer(lastReadPointer), numSamples);
        }

        abstractFifo.finishedRead(size1);
    }

    int getBufferSize() const
    {
        return bufferToDraw.getNumSamples();
    }

private:
    const int numBuffers{ 5 };
    const int bufferSize{ 1024 };
    int lastReadPointer{ 0 };
    const int resetThreshold{ 30 };
    int resetCount{ 0 };
    juce::AbstractFifo abstractFifo{ numBuffers };
    juce::AudioBuffer<float> bufferToDraw{ numBuffers, bufferSize };
};

class WaveSampleCollector
{
public:
    WaveSampleCollector(WaveDrawBuffer& bufferToUse)
        : drawBuffer(bufferToUse)
    {
        sampleCollecion.setSize(1, drawBuffer.getBufferSize());
    }

    void process(const float* samples, const int numSamples)
    {
        int index = 0;
        while (index++ < numSamples)
        {
            sampleCollecion.getWritePointer(0)[numCollected++] = *samples++;
            if (numCollected == drawBuffer.getBufferSize())
            {
                drawBuffer.push(sampleCollecion.getReadPointer(0), sampleCollecion.getNumSamples());
                numCollected = 0;
            }
        }
    }

private:
    WaveDrawBuffer& drawBuffer;
    juce::AudioBuffer<float> sampleCollecion;
    int numCollected{ 0 };
};

//==============================================================================
/**
*/
class HelloAudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    HelloAudioPluginAudioProcessor();
    ~HelloAudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    virtual void processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getProcessorState() { return apvts; }
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() const;

    WaveDrawBuffer& getWaveDrawBuffer() { return waveDrawBuffer; }

private:
    //==============================================================================
    enum OscillatorType
    {
        kUnknown = -1,
        kSine = 0,
        kSquare,
        kTriangle,
        kSaw,
        kNoise
    };
    const juce::StringArray oscillatorTypes{ "Sine", "Square", "Triangle", "Saw", "Noise" };

    juce::AudioProcessorValueTreeState apvts;

    float lastPhase{ 0.0f };
    float lastGain{ 0.0f };
    juce::Random random;

    // Wave shape visualizer
    WaveDrawBuffer waveDrawBuffer;
    WaveSampleCollector waveSampleCollector;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HelloAudioPluginAudioProcessor)
};
