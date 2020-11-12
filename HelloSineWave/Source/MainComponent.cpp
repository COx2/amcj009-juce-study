#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : gainSlider(std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow))
    , pitchSlider(std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow))
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

    gainSlider->setRange(0.0, 1.0);
    gainSlider->setValue(0.5f);
    addAndMakeVisible(gainSlider.get());
    
    pitchSlider->setRange(0.1, 10.0);
    pitchSlider->setValue(1.0f);
    addAndMakeVisible(pitchSlider.get());

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
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    gain = gainSlider->getValue();
    pitch = pitchSlider->getValue();

    bufferToFill.clearActiveBufferRegion();
    auto* buffer = bufferToFill.buffer;

    float phase;
    const float phase_delta = juce::MathConstants<float>::twoPi / buffer->getNumSamples() * pitch;

    for (int channel = 0; channel < buffer->getNumChannels(); ++channel)
    {
        float* channelData = buffer->getWritePointer(channel);
        phase = lastPhase;
        for (int sample = 0; sample < buffer->getNumSamples(); ++sample)
        {
            phase += phase_delta;

            if (phase > juce::MathConstants<float>::twoPi)
                phase -= juce::MathConstants<float>::twoPi;

            channelData[sample] = sinf(phase) * gain;
        }
    }
    lastPhase = phase;

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
    const auto w = drawArea.getWidth();
    const auto h = drawArea.getHeight();

    juce::Path wavePath;
    const float x0 = drawArea.getX();
    const float cloped_sample0 = juce::jmax<float>(-1.0f, juce::jmin<float>(1.0f, plotData[0]));
    const float y0 = juce::jmap<float>(cloped_sample0, -1.0f, 1.0f, drawArea.getBottom(), drawArea.getY());
    wavePath.startNewSubPath(x0, y0);

    for (int i = 1; i < numSamples; ++i)
    {
        const float x = juce::jmap<float>(i, 0, numSamples, x0, x0 + w);
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

    auto bounds = getLocalBounds();

    juce::Rectangle<float> textArea = { bounds.getWidth() * 0.1f, bounds.getHeight() * 0.05f, bounds.getWidth() * 0.8f, bounds.getHeight() * 0.1f };
    g.setColour(juce::Colours::white);
    g.setFont(24.0f);
    g.drawFittedText("Hello, sine wave", textArea.toNearestInt(), juce::Justification::centred, 1);

    juce::Rectangle<float> drawArea = { bounds.getWidth() * 0.1f, bounds.getHeight() * 0.6f, bounds.getWidth() * 0.8f, bounds.getHeight() * 0.3f };
    g.setColour(juce::Colours::darkgrey);
    g.fillRect(drawArea);

    // Draw wave shape
    juce::AudioBuffer<float> samplesToDraw(1, waveDrawBuffer.getBufferSize());
    waveDrawBuffer.pop(samplesToDraw.getWritePointer(0), samplesToDraw.getNumSamples());
    drawWaveShape(g, drawArea, samplesToDraw.getWritePointer(0), samplesToDraw.getNumSamples());
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    const int slider_w = bounds.getWidth() * 0.3f;
    const int slider_h = bounds.getHeight() * 0.3f;

    gainSlider->setSize(slider_w, slider_h);
    gainSlider->setCentrePosition(bounds.getWidth() * 0.25f, bounds.getHeight() * 0.3f);

    pitchSlider->setSize(slider_w, slider_h);
    pitchSlider->setCentrePosition(bounds.getWidth() * 0.75f, bounds.getHeight() * 0.3f);
}

void MainComponent::timerCallback()
{
    repaint();
}

