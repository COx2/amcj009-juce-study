#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
    : gainSlider(std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow))
    , pitchSlider(std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow))
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
    addAndMakeVisible(gainSlider.get());
    
    pitchSlider->setRange(0.1, 10.0);
    addAndMakeVisible(pitchSlider.get());
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
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    const int slider_w = bounds.getWidth() * 0.3f;
    const int slider_h = bounds.getHeight() * 0.3f;

    gainSlider->setSize(slider_w, slider_h);
    gainSlider->setCentrePosition(bounds.getWidth() * 0.25f, bounds.getHeight() * 0.5f);

    pitchSlider->setSize(slider_w, slider_h);
    pitchSlider->setCentrePosition(bounds.getWidth() * 0.75f, bounds.getHeight() * 0.5f);
}

