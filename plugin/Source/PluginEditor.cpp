#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VocAudioProcessorEditor::VocAudioProcessorEditor (VocAudioProcessor& p)
    : gin::ProcessorEditor (p), proc (p)
{
    for (auto pp : p.getPluginParameters())
    {
        auto c = pp->isOnOff() ? (gin::ParamComponent*)new gin::Switch (pp) : (gin::ParamComponent*)new gin::Knob (pp);
        
        addAndMakeVisible (c);
        controls.add (c);
    }
    
    addAndMakeVisible (&scope);
    
    setGridSize (7, 2);
    
    scope.setNumSamplesPerPixel (2);
    scope.setVerticalZoomFactor (3.0f);
    scope.setColour (gin::TriggeredScope::lineColourId, findColour (gin::PluginLookAndFeel::grey45ColourId));
    scope.setColour (gin::TriggeredScope::traceColourId + 0, findColour (gin::PluginLookAndFeel::accentColourId));
    scope.setColour (gin::TriggeredScope::envelopeColourId + 0, juce::Colours::transparentBlack);
    scope.setColour (gin::TriggeredScope::traceColourId + 1, findColour (gin::PluginLookAndFeel::accentColourId));
    scope.setColour (gin::TriggeredScope::envelopeColourId + 1, juce::Colours::transparentBlack);
}

VocAudioProcessorEditor::~VocAudioProcessorEditor()
{
}

//==============================================================================
void VocAudioProcessorEditor::paint (juce::Graphics& g)
{
    gin::ProcessorEditor::paint (g);
}

void VocAudioProcessorEditor::resized()
{
    using AP = VocAudioProcessor;
    
    gin::ProcessorEditor::resized();
    
    componentForId (AP::paramTenseness)->setBounds (getGridArea (0, 0));
    componentForId (AP::paramSmoothing)->setBounds (getGridArea (1, 0));
    componentForId (AP::paramConstrictionPosition)->setBounds (getGridArea (2, 0));
    componentForId (AP::paramConstrictionAmount)->setBounds (getGridArea (3, 0));

    componentForId (AP::paramAttack)->setBounds (getGridArea (0, 1));
    componentForId (AP::paramDecay)->setBounds (getGridArea (1, 1));
    componentForId (AP::paramSustain)->setBounds (getGridArea (2, 1));
    componentForId (AP::paramRelease)->setBounds (getGridArea (3, 1));
    
    componentForId (AP::paramGlide)->setBounds (getGridArea (6, 0));
    componentForId (AP::paramOutput)->setBounds (getGridArea (6, 1));

    scope.setBounds (getGridArea (4, 0, 2, 2).reduced (5));
}
