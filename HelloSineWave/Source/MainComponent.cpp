#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : sliderGain(std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow))
    , sliderFrequency(std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow))
    , comboboxOscillator(std::make_unique<juce::ComboBox>())
    , waveSampleCollector(waveDrawBuffer)
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }

    sliderGain->setRange(0.0, 1.0);
    sliderGain->setNumDecimalPlacesToDisplay(2);
    sliderGain->setValue(0.5);
    addAndMakeVisible(sliderGain.get());
    
    sliderFrequency->setRange(20.0, 2000.0);
    sliderFrequency->setNumDecimalPlacesToDisplay(0);
    sliderFrequency->setValue(440.0);
    addAndMakeVisible(sliderFrequency.get());
    
    comboboxOscillator->addItemList(oscillatorTypes, 1);
    comboboxOscillator->setSelectedItemIndex(0);
    addAndMakeVisible(comboboxOscillator.get());

    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Get parameter value
    gain = (float)sliderGain->getValue();
    frequency = (float)sliderFrequency->getValue();
    oscillator = (OscillatorType)comboboxOscillator->getSelectedItemIndex();

    // Clear buffer
    bufferToFill.clearActiveBufferRegion();
    auto* buffer = bufferToFill.buffer;

    // Calculate phase delta for wanted frequency
    float phase = 0.0f;
    const float base_rad = juce::MathConstants<float>::twoPi / (float)currentSampleRate;
    const float phase_delta = base_rad * frequency;

    // Render audio data
    for (int channel = 0; channel < buffer->getNumChannels(); ++channel)
    {
        float* channelData = buffer->getWritePointer(channel);
        phase = lastPhase;
        for (int sample = 0; sample < buffer->getNumSamples(); ++sample)
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
    buffer->applyGainRamp(0, buffer->getNumSamples(), lastGain, gain);
    lastGain = gain;

    // Collect wave sample
    waveSampleCollector.process(buffer->getReadPointer(0), buffer->getNumSamples());
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
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

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    const auto bounds = getLocalBounds();

    // Draw text
    const juce::Rectangle<float> textArea = { bounds.getWidth() * 0.1f, bounds.getHeight() * 0.05f, bounds.getWidth() * 0.8f, bounds.getHeight() * 0.1f };
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("Hello Audio Programming", textArea.toNearestInt(), juce::Justification::centred, 1);

    // Draw wave shape background
    const juce::Rectangle<float> drawArea = { bounds.getWidth() * 0.1f, bounds.getHeight() * 0.6f, bounds.getWidth() * 0.8f, bounds.getHeight() * 0.3f };
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(drawArea);

    // Draw wave shape
    juce::AudioBuffer<float> samplesToDraw(1, waveDrawBuffer.getBufferSize());
    waveDrawBuffer.pop(samplesToDraw.getWritePointer(0), samplesToDraw.getNumSamples());
    drawWaveShape(g, drawArea, samplesToDraw.getWritePointer(0), samplesToDraw.getNumSamples());
}

void MainComponent::resized()
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

void MainComponent::timerCallback()
{
    repaint();
}

