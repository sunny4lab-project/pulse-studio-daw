#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
namespace pulse {
constexpr int insertsPerRack=2;
struct InsertSlot {
 std::unique_ptr<juce::AudioPluginInstance> plugin;
 std::atomic<bool> bypass{false};
};
// Instances are replaced only with the device callback detached.
// No scanning, editor creation or plugin destruction happens in the audio callback.
struct InsertRack {
 std::array<InsertSlot,insertsPerRack> slots;
 bool active() const noexcept {
  for(const auto& s:slots)if(s.plugin&&!s.bypass.load()&&s.plugin->getLatencySamples()==0)return true;
  return false;
 }
 void prepare(double rate,int size){for(auto& s:slots)if(s.plugin)s.plugin->prepareToPlay(rate,size);}
 void release(){for(auto& s:slots)if(s.plugin)s.plugin->releaseResources();}
 void process(float* left,float* right,int n,juce::MidiBuffer& midi){
  if(!active())return;
  float* pointers[]={left,right};
  juce::AudioBuffer<float> block(pointers,2,n);
  for(auto& s:slots){
   if(!s.plugin||s.bypass.load()||s.plugin->getLatencySamples()!=0)continue;
   midi.clear();s.plugin->processBlock(block,midi);
   for(int c=0;c<2;++c){auto* p=block.getWritePointer(c);for(int i=0;i<n;++i)if(!std::isfinite(p[i]))p[i]=0.f;}
  }
 }
};
}
