#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <time.h>

const char* VocAudioProcessor::paramTenseness               = "tenseness";
const char* VocAudioProcessor::paramConstrictionPosition    = "constrictionP";
const char* VocAudioProcessor::paramConstrictionAmount      = "constrictionA";
const char* VocAudioProcessor::paramSmoothing               = "smoothing";
const char* VocAudioProcessor::paramGlide                   = "glide";
const char* VocAudioProcessor::paramOutput                  = "output";

const char* VocAudioProcessor::paramAttack                  = "attack";
const char* VocAudioProcessor::paramDecay                   = "decay";
const char* VocAudioProcessor::paramSustain                 = "sustain";
const char* VocAudioProcessor::paramRelease                 = "release";

//==============================================================================
juce::String percentTextFunction (const gin::Parameter& p, float v)
{
    return juce::String::formatted ("%.0f%%", v / p.getUserRangeEnd() * 100);
}

juce::String glideTextFunction (const gin::Parameter&, float v)
{
    return juce::String::formatted ("%.2f", v);
}

juce::String onOffTextFunction (const gin::Parameter&, float v)
{
    return v > 0.0f ? "On" : "Off";
}

//==============================================================================
VocAudioProcessor::VocAudioProcessor()
    : gin::Processor (false, gin::ProcessorOptions().withAdditionalCredits({"Neil Thapen"}))
{
    addExtParam (paramTenseness,            "Tenseness",             "Tenseness",   "", {0.0f,  1.0f, 0.0f, 1.0f},  0.0f, 0.0f, percentTextFunction);
    addExtParam (paramConstrictionPosition, "Constriction Position", "Const Pos" ,  "", {0.0f,  1.0f, 0.0f, 1.0f},  0.0f, 0.0f, percentTextFunction);
    addExtParam (paramConstrictionAmount,   "Constriction Amount",   "Const Amt",   "", {0.0f,  1.0f, 0.0f, 1.0f},  0.0f, 0.0f, percentTextFunction);
    addExtParam (paramSmoothing,            "Smoothing",             "Smoothing",   "", {0.0f,  1.0f, 0.0f, 1.0f},  0.0f, 0.0f, percentTextFunction);
    addExtParam (paramGlide,                "Glide",                 "Glide",      "s", {0.0f,  0.5f, 0.0f, 1.0f},  1.0f, 0.0f, glideTextFunction);
    addExtParam (paramOutput,               "Output",                "Output",      "", {0.0f,  1.0f, 0.0f, 1.0f},  1.0f, 0.0f, percentTextFunction);
    addExtParam (paramAttack,               "Attack",                "A",          "s", {0.0f, 10.0f, 0.0f, 0.4f}, 0.01f, 0.0f);
    addExtParam (paramDecay,                "Decay",                 "D",          "s", {0.0f, 10.0f, 0.0f, 0.4f}, 0.01f, 0.0f);
    addExtParam (paramSustain,              "Sustain",               "S",          "%", {0.0f,  1.0f, 0.0f, 1.0f},  1.0f, 0.0f);
    addExtParam (paramRelease,              "Release",               "R",          "s", {0.0f, 10.0f, 0.0f, 0.4f}, 0.01f, 0.0f);

    init();
}

VocAudioProcessor::~VocAudioProcessor()
{
    if (voc != nullptr)
        voc_shutdown (voc);
}

//==============================================================================
void VocAudioProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    
    if (voc != nullptr)
        voc_shutdown (voc);
    
    voc = voc_init ((unsigned long) sampleRate, (unsigned int) time (nullptr));
    
    outputSmoothed.reset (sampleRate, 0.05);
    
    adsr.setSampleRate (sampleRate);
    
    noteSmoothed.reset (sampleRate, parameterValue (paramGlide));
    lastGlide = parameterValue (paramGlide);
}

void VocAudioProcessor::releaseResources()
{
}

void VocAudioProcessor::runUntil (int& done, juce::AudioSampleBuffer& buffer, int pos)
{
    int todo = std::min (pos, buffer.getNumSamples()) - done;
    
    if (todo > 0)
    {
        auto data = buffer.getWritePointer (0, done);
        for (int i = 0; i < todo; i++)
            data[i] = voc_f (voc, 0) * adsr.process();
        
        done += todo;
        noteSmoothed.skip (todo);
    }
}

void VocAudioProcessor::processBlock (juce::AudioSampleBuffer& buffer, juce::MidiBuffer& midi)
{
	buffer.clear();
	
    if (parameterValue (paramGlide) != lastGlide)
    {
        lastGlide = parameterValue (paramGlide);
        noteSmoothed.reset (sampleRate, parameterValue (paramGlide));
    }
    
    adsr.setAttack (parameterValue (paramAttack));
    adsr.setDecay (parameterValue (paramDecay));
    adsr.setSustainLevel (parameterValue (paramSustain));
    adsr.setRelease (parameterValue (paramRelease));

    voc_constriction_amount_set (voc, getParameter (paramConstrictionAmount)->getUserValue());
    voc_constriction_position_set (voc, getParameter (paramConstrictionPosition)->getUserValue());
    voc_tenseness_set (voc, getParameter (paramTenseness)->getUserValue());
    voc_smoothing_set (voc, getParameter (paramSmoothing)->getUserValue());
    
    outputSmoothed.setTargetValue (getParameter (paramOutput)->getUserValue());

    const int playingNote = noteQueue.size() > 0 ? noteQueue.getLast() : -1;
    if (playingNote != -1)
        voc_note_on (voc, juce::jlimit (0.0f, 127.0f, noteSmoothed.getCurrentValue() * 127.0f + bend), velocity);
    
    int done = 0;
    runUntil (done, buffer, 0);
    
    for (auto itr : midi)
    {
        auto msg = itr.getMessage();
        int pos = itr.samplePosition;

        runUntil (done, buffer, pos);
        
        if (msg.isNoteOn())
        {
            noteQueue.add (msg.getNoteNumber());
            velocity = msg.getVelocity();
        }
        else if (msg.isNoteOff())
        {
            noteQueue.removeFirstMatchingValue (msg.getNoteNumber());
            velocity = msg.getVelocity();
        }
        else if (msg.isAllNotesOff())
        {
            noteQueue.clear();
        }
        
        const int curNote = noteQueue.size() > 0 ? noteQueue.getLast() : -1;
        
        if (curNote != lastNote)
        {
            if (curNote == -1)
            {
                adsr.noteOff();
            }
            else
            {
                if (lastNote == -1)
                    noteSmoothed.setCurrentAndTargetValue (curNote / 127.0f);
                else
                    noteSmoothed.setTargetValue (curNote / 127.0f);
                
                voc_note_on (voc, juce::jlimit (0.0f, 127.0f, noteSmoothed.getCurrentValue() * 127.0f + bend), velocity);
                if (lastNote == -1)
                    adsr.noteOn();
            }
            
            lastNote = curNote;
        }
        
        if (msg.isPitchWheel())
        {
            bend = (msg.getPitchWheelValue() / float (0x3FFF)) * 4 - 2;
            if (curNote != -1)
                voc_note_on (voc, juce::jlimit (0.0f, 127.0f, noteSmoothed.getCurrentValue() * 127.0f + bend), velocity);
        }

        if (curNote == -1 && adsr.getOutput() == 0.0f)
            voc_note_off (voc, velocity);
    }

    int numSamples = buffer.getNumSamples();
    runUntil (done, buffer, numSamples);
    
    auto data = buffer.getWritePointer (0);
    for (int i = 0; i < numSamples; i++)
        data[i] *= outputSmoothed.getNextValue();
    
    if (fifo.getFreeSpace() >= numSamples)
        fifo.writeMono (data, numSamples);
}

//==============================================================================
bool VocAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* VocAudioProcessor::createEditor()
{
    return new VocAudioProcessorEditor (*this);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocAudioProcessor();
}
