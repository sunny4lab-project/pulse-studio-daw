#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
namespace pulse {
constexpr int stemCount=4;
// All readers run on the transport read-ahead thread, not the device callback.
// A single source position keeps same-rate files aligned, including seeks.
class StemSource final : public juce::PositionableAudioSource {
 std::array<std::unique_ptr<juce::AudioFormatReader>,stemCount> readers;
 std::array<std::atomic<float>,stemCount>& trims;
 std::array<std::atomic<bool>,stemCount>& mutes;
 juce::AudioBuffer<float> scratch{2,4096};
 std::atomic<juce::int64> position{0};
 juce::int64 length=0;
 double sourceRate=0;
 int count=0;
public:
 StemSource(std::array<std::atomic<float>,stemCount>& t,std::array<std::atomic<bool>,stemCount>& m):trims(t),mutes(m){}
 bool add(std::unique_ptr<juce::AudioFormatReader> r){
  if(!r||count>=stemCount||r->lengthInSamples<=0||r->sampleRate<=0||r->numChannels<1||r->numChannels>2)return false;
  if(count&&std::abs(r->sampleRate-sourceRate)>0.01)return false;
  sourceRate=r->sampleRate;length=std::max(length,r->lengthInSamples);readers[count++]=std::move(r);return true;
 }
 double getRate() const{return sourceRate;}
 void prepareToPlay(int,double) override {}
 void releaseResources() override {}
 void setNextReadPosition(juce::int64 p) override {position.store(std::max<juce::int64>(0,p));}
 juce::int64 getNextReadPosition() const override {return position.load();}
 juce::int64 getTotalLength() const override {return length;}
 bool isLooping() const override {return false;}
 void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override {
  if(!info.buffer)return;info.clearActiveBufferRegion();auto pos=position.load();
  for(int offset=0;offset<info.numSamples;){
   int size=std::min(scratch.getNumSamples(),info.numSamples-offset);
   for(int t=0;t<count;++t){
    auto& reader=*readers[t];auto remaining=reader.lengthInSamples-pos-offset;
    if(mutes[t].load()||remaining<=0)continue;
    int n=(int)std::min<juce::int64>(size,remaining);scratch.clear();
    if(!reader.read(&scratch,0,n,pos+offset,true,true))continue;
    float g=trims[t].load()/(float)count;
    for(int k=0;k<std::min(2,info.buffer->getNumChannels());++k)
     info.buffer->addFrom(k,info.startSample+offset,scratch,reader.numChannels==1?0:k,0,n,g);
   }
   offset+=size;
  }
  position.store(pos+info.numSamples);
 }
};
}
