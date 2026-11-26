#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
namespace pulse {
class SendEffects {
 juce::AudioBuffer<float> verbInput,delayInput,ring;
 juce::Reverb verb;
 int cursor=0;bool renderActive=false;
 double rate=48000;
public:
 std::array<std::atomic<float>,32> reverbSend{},delaySend{};
 std::atomic<float> reverbReturn{.25f},delayReturn{.25f},delayMs{280.f},feedback{.25f},room{.45f};
 std::atomic<bool> enabled{false};
 SendEffects(){for(auto& s:reverbSend)s=0;for(auto& s:delaySend)s=0;}
 void prepare(double sr,int block){rate=sr;verbInput.setSize(2,block);delayInput.setSize(2,block);ring.setSize(2,(int)std::ceil(sr*1.1)+1);verb.setSampleRate(sr);reset();}
 void reset(){verb.reset();ring.clear();cursor=0;}
 void begin(int n){bool next=enabled.load()&&n<=verbInput.getNumSamples()&&ring.getNumSamples()>1;if(next!=renderActive)reset();renderActive=next;if(renderActive){verbInput.clear(0,n);delayInput.clear(0,n);}}
 void send(int bus,int channel,const float* data,float gain,int n){if(!renderActive)return;
  float v=reverbSend[bus].load()*gain,d=delaySend[bus].load()*gain;
  if(v>0)verbInput.addFrom(channel,0,data,n,v);
  if(d>0)delayInput.addFrom(channel,0,data,n,d);
 }
 void render(float* l,float* r,int n){
  if(!renderActive)return;
  juce::Reverb::Parameters p;p.roomSize=room.load();p.damping=.55f;p.wetLevel=1;p.dryLevel=0;p.width=1;p.freezeMode=0;verb.setParameters(p);
  verb.processStereo(verbInput.getWritePointer(0),verbInput.getWritePointer(1),n);
  int size=ring.getNumSamples(),distance=juce::jlimit(1,size-1,(int)(delayMs.load()*.001*rate));
  float f=juce::jlimit(0.f,.65f,feedback.load()),vg=reverbReturn.load(),dg=delayReturn.load();
  float* out[]={l,r};for(int i=0;i<n;++i){int read=(cursor+size-distance)%size;for(int c=0;c<2;++c){float wet=ring.getSample(c,read);float input=delayInput.getSample(c,i);ring.setSample(c,cursor,juce::jlimit(-4.f,4.f,input+wet*f));out[c][i]+=verbInput.getSample(c,i)*vg+wet*dg;}cursor=(cursor+1)%size;}
 }
};
}
