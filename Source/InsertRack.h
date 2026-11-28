#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include <cmath>
#include "DelayAlignment.h"
namespace pulse {
constexpr int insertsPerRack=16,initialInsertSlots=2;
constexpr int maxRackLatency=4096,maxAlignmentLatency=8192;
struct InsertSlot {
 std::unique_ptr<juce::AudioPluginInstance> plugin;
 std::atomic<bool> bypass{false};
};
struct InsertRack {
 std::array<InsertSlot,insertsPerRack> slots;
 int visibleSlots=initialInsertSlots; // message-thread UI only
 StereoAlignDelay alignment;
 int lastDelay=-1;
 int latency(bool compensated) const noexcept {
  if(!compensated)return 0;
  int sum=0;
  for(const auto& s:slots)if(s.plugin&&!s.bypass.load()){
   int l=s.plugin->getLatencySamples();if(l<0||l>maxRackLatency-sum)return -1;sum+=l;
  }return sum;
 }
 bool active(bool compensated=false) const noexcept {
  for(const auto& s:slots)if(s.plugin&&!s.bypass.load()&&(compensated||s.plugin->getLatencySamples()==0))return true;
  return false;
 }
 void prepare(double rate,int size){for(auto& s:slots)if(s.plugin)s.plugin->prepareToPlay(rate,size);}
 void prepareAlignment(bool enabled){alignment.prepare(enabled?maxAlignmentLatency:0);lastDelay=-1;}
 void release(){for(auto& s:slots)if(s.plugin)s.plugin->releaseResources();}
 void align(float* left,float* right,int n,int delay){
  if(lastDelay!=delay){alignment.configure(delay);lastDelay=delay;}
  if(delay>0)alignment.process(left,right,n);
 }
 void clearAlignment(){if(lastDelay!=0){alignment.configure(0);lastDelay=0;}}
 void process(float* left,float* right,int n,juce::MidiBuffer& midi,bool compensated=false){
  if(!active(compensated))return;
  float* pointers[]={left,right};juce::AudioBuffer<float> block(pointers,2,n);
  for(auto& s:slots){
   if(!s.plugin||s.bypass.load()||(!compensated&&s.plugin->getLatencySamples()!=0))continue;
   midi.clear();s.plugin->processBlock(block,midi);
   for(int c=0;c<2;++c){auto* p=block.getWritePointer(c);for(int i=0;i<n;++i)if(!std::isfinite(p[i]))p[i]=0.f;}
  }
 }
};
}
