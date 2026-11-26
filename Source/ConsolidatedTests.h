#pragma once
#include "AudioEngine.h"
namespace pulse {
// Synthetic stereo effect with known sample delay. No external plugin or hardware.
class TestDelayEffect final : public juce::AudioPluginInstance {
 StereoAlignDelay line;int delay;
public:
 explicit TestDelayEffect(int samples):AudioPluginInstance(BusesProperties().withInput("In",juce::AudioChannelSet::stereo()).withOutput("Out",juce::AudioChannelSet::stereo())),delay(samples){setLatencySamples(samples);}
 const juce::String getName()const override{return "Synthetic delay";}
 void prepareToPlay(double,int)override{line.prepare(delay);line.configure(delay);}
 void releaseResources()override{}
 void processBlock(juce::AudioBuffer<float>& block,juce::MidiBuffer&)override{line.process(block.getWritePointer(0),block.getWritePointer(1),block.getNumSamples());}
 double getTailLengthSeconds()const override{return 0;}
 bool acceptsMidi()const override{return false;}
 bool producesMidi()const override{return false;}
 bool hasEditor()const override{return false;}
 juce::AudioProcessorEditor* createEditor()override{return nullptr;}
 int getNumPrograms()override{return 1;}
 int getCurrentProgram()override{return 0;}
 void setCurrentProgram(int)override{}
 const juce::String getProgramName(int)override{return "Default";}
 void changeProgramName(int,const juce::String&)override{}
 void getStateInformation(juce::MemoryBlock&)override{}
 void setStateInformation(const void*,int)override{}
 void fillInPluginDescription(juce::PluginDescription& d)const override{d.name=getName();d.pluginFormatName="Synthetic";d.numInputChannels=2;d.numOutputChannels=2;}
};
inline bool testConsolidated(){
 // Distinct plugin identities sharing a single library file must survive cache serialization.
 juce::KnownPluginList catalog;
 juce::PluginDescription a;a.name="Shell A";a.manufacturerName="Synthetic";a.pluginFormatName="VST3";a.fileOrIdentifier="test-shell.vst3";a.uniqueId=11;a.deprecatedUid=11;a.numInputChannels=a.numOutputChannels=2;
 auto b=a;b.name="Shell B";b.uniqueId=12;b.deprecatedUid=12;
 if(!catalog.addType(a)||!catalog.addType(b)||catalog.getNumTypes()!=2)return false;
 juce::KnownPluginList restored;restored.recreateFromXml(*catalog.createXml());if(restored.getNumTypes()!=2)return false;
 Engine e;if(!e.addChannels({0,1,2}))return false;
 constexpr int block=256;
 e.playback.setSize(2,block);e.pluginBuffer.setSize(2,block);for(auto& bus:e.buses)bus.setSize(2,block);
 e.ch[0].bus=0;e.ch[1].bus=0;e.ch[2].bus=1;
 for(auto& c:e.ch){c.gain=1;c.mute=false;c.pan=0;}
 e.master=1;
 e.channelInserts[0].slots[0].plugin=std::make_unique<TestDelayEffect>(12);
 e.busInserts[0].slots[0].plugin=std::make_unique<TestDelayEffect>(8);
 e.channelInserts[0].prepare(48000,block);e.busInserts[0].prepare(48000,block);
 if(!e.setCompensated(true))return false;
 juce::AudioBuffer<float> input(3,block),output(2,block);input.clear();
 const float* ins[]={input.getReadPointer(0),input.getReadPointer(1),input.getReadPointer(2)};
 float* outs[]={output.getWritePointer(0),output.getWritePointer(1)};
 e.enabled=true;
 // Let existing channel gain smoothing settle before testing a common impulse.
 for(int i=0;i<32;++i)e.audioDeviceIOCallbackWithContext(ins,3,outs,2,block,{});
 for(int i=0;i<3;++i)input.setSample(i,0,.1f);
 e.audioDeviceIOCallbackWithContext(ins,3,outs,2,block,{});
 if(e.alignmentSamples.load()!=20||e.alignmentFault.load())return false;
 for(int k=0;k<2;++k)for(int i=0;i<block;++i){
  float target=i==20?.3f*std::sqrt(.5f):0.f;
  if(std::abs(output.getSample(k,i)-target)>.0005f)return false;
 }
 // Runtime excessive latency must fail closed rather than emit misaligned audio.
 e.channelInserts[0].slots[0].plugin->setLatencySamples(maxRackLatency+1);
 e.audioDeviceIOCallbackWithContext(ins,3,outs,2,block,{});
 return e.alignmentFault.load()&&!e.enabled.load()&&output.getMagnitude(0,block)==0.f;
}
}
