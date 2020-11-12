#pragma once

#include <JuceHeader.h>

//==============================================================================
class WaveDrawBuffer
{
public:
    void push(const float* dataToPush, int numSamples)
    {
        jassert(numSamples <= bufferToDraw.getNumSamples());

        int start1, size1, start2, size2;

        abstractFifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0)
            juce::FloatVectorOperations::copy(bufferToDraw.getWritePointer(start1), dataToPush, numSamples);

        abstractFifo.finishedWrite(size1);
    }

    void pop(float* outputBuffer, int numSamples)
    {
        jassert(numSamples <= bufferToDraw.getNumSamples());

        int start1, size1, start2, size2;
        abstractFifo.prepareToRead(1, start1, size1, start2, size2);

        if (size1 > 0)
        {
            juce::FloatVectorOperations::copy(outputBuffer, bufferToDraw.getReadPointer(start1), numSamples);
            lastReadPointer = start1;
        }
        else
            juce::FloatVectorOperations::copy(outputBuffer, bufferToDraw.getReadPointer(lastReadPointer), numSamples);

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

    void process(const float* samples, int numSamples)
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
    juce::AudioBuffer<float> sampleCollecion;
    int numCollected{0};
    WaveDrawBuffer& drawBuffer;
};

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  : public juce::AudioAppComponent, public juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    virtual void timerCallback() override;

    float gain{ 0.5f };
    float pitch{ 1.0f };
    float lastPhase{ 0.0f };

    std::unique_ptr<juce::Slider> gainSlider;
    std::unique_ptr<juce::Slider> pitchSlider;

    WaveDrawBuffer waveDrawBuffer;
    WaveSampleCollector waveSampleCollector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
