#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <cmath>
#include <algorithm>
namespace pulse {
using Frame=juce::int64;
inline constexpr double timelineRate=48000.0;
inline constexpr Frame timelineLimit=48LL*60*60*48000;
struct AudioClip {
 juce::File file;Frame start=0,offset=0,length=0,fadeIn=0,fadeOut=0;float gain=1.f;
};
struct TimelineTrack {
 juce::String name;float gain=1.f,pan=0.f;bool mute=false,solo=false;
 std::vector<AudioClip> clips;
};
struct TimelineSession {
 std::vector<TimelineTrack> tracks;
 Frame length()const{Frame end=0;for(auto& t:tracks)for(auto& c:t.clips)end=std::max(end,c.start+c.length);return end;}
 bool split(int track,int clip,Frame at){
  if(track<0||track>=(int)tracks.size())return false;auto& v=tracks[track].clips;
  if(clip<0||clip>=(int)v.size())return false;auto c=v[clip];Frame cut=at-c.start;if(cut<=0||cut>=c.length)return false;
  auto right=c;right.start=at;right.offset+=cut;right.length-=cut;right.fadeIn=0;right.fadeOut=std::min(right.fadeOut,right.length);
  c.length=cut;c.fadeOut=0;c.fadeIn=std::min(c.fadeIn,c.length);v[clip]=c;v.insert(v.begin()+clip+1,right);return true;
 }
 static Frame duration(const juce::AudioFormatReader& r){return (Frame)std::min((double)timelineLimit,std::floor((double)r.lengthInSamples*timelineRate/r.sampleRate));}
};
inline bool validClip(const AudioClip& c,Frame available){
 return c.start>=0&&c.start<=timelineLimit&&c.offset>=0&&c.offset<=available&&c.length>0&&c.length<=available-c.offset&&c.length<=timelineLimit-c.start&&c.fadeIn>=0&&c.fadeOut>=0&&c.fadeIn<=c.length&&c.fadeOut<=c.length&&std::isfinite(c.gain)&&c.gain>=0&&c.gain<=4;
}
// Immutable session snapshot, rendered by the transport read-ahead thread.
// The fixed 48 kHz project clock supports mixed file rates via linear interpolation.
class TimelineSource final : public juce::PositionableAudioSource {
 struct Item {const TimelineTrack* track;const AudioClip* clip;std::unique_ptr<juce::AudioFormatReader> reader;};
 TimelineSession model;std::vector<Item> items;
 juce::AudioBuffer<float> scratch{2,8194};std::atomic<Frame> position{0};Frame total=0;bool anySolo=false;
 std::atomic<bool> readError{false};
 explicit TimelineSource(TimelineSession s):model(std::move(s)){}
public:
 static std::unique_ptr<TimelineSource> create(juce::AudioFormatManager& formats,TimelineSession s,juce::String& error){
  auto src=std::unique_ptr<TimelineSource>(new TimelineSource(std::move(s)));
  for(auto& t:src->model.tracks){
   if(!std::isfinite(t.gain)||t.gain<0||t.gain>4||!std::isfinite(t.pan)||std::abs(t.pan)>1){error="Invalid track gain/pan.";return {};}
   src->anySolo|=t.solo;
   for(auto& c:t.clips){
    std::unique_ptr<juce::AudioFormatReader> r(formats.createReaderFor(c.file));
    if(!r||r->numChannels<1||r->numChannels>2||!std::isfinite(r->sampleRate)||r->sampleRate<8000||r->sampleRate>384000||r->lengthInSamples<=0||!validClip(c,TimelineSession::duration(*r))){error="Missing or invalid clip: "+c.file.getFileName();return {};}
    src->items.push_back({&t,&c,std::move(r)});
   }
  }
  src->total=src->model.length();error.clear();return src;
 }
 const TimelineSession& session()const{return model;}
 bool failed()const{return readError.load();}
 void prepareToPlay(int,double)override{}
 void releaseResources()override{}
 void setNextReadPosition(Frame p)override{position=std::max<Frame>(0,p);}
 Frame getNextReadPosition()const override{return position.load();}
 Frame getTotalLength()const override{return total;}
 bool isLooping()const override{return false;}
 void getNextAudioBlock(const juce::AudioSourceChannelInfo& info)override{
  if(!info.buffer||info.numSamples<=0)return;info.clearActiveBufferRegion();Frame pos=position.load();
  for(auto& item:items){
   auto& t=*item.track;auto& c=*item.clip;auto& r=*item.reader;
   if(t.mute||(anySolo&&!t.solo)||t.gain==0||c.gain==0)continue;
   Frame begin=std::max(pos,c.start),end=std::min(pos+info.numSamples,c.start+c.length);
   if(end<=begin)continue;
   double ratio=r.sampleRate/timelineRate;float g=t.gain*c.gain;
   float panL=std::min(1.f,1.f-t.pan),panR=std::min(1.f,1.f+t.pan);
   for(Frame p=begin;p<end;){
    int n=(int)std::min<Frame>(1024,end-p);
    double first=(double)(c.offset+p-c.start)*ratio;Frame base=(Frame)std::floor(first);
    int need=(int)std::floor(first-base+(n-1)*ratio)+2;
    scratch.clear();int available=(int)std::min<Frame>(need,std::max<Frame>(0,r.lengthInSamples-base));
    if(available<=0||!r.read(&scratch,0,available,base,true,true)){readError=true;p+=n;continue;}
    for(int i=0;i<n;++i){
     double x=first-base+i*ratio;int j=(int)x;float f=(float)(x-j);Frame local=p+i-c.start;
     float env=1.f;if(c.fadeIn>0)env=std::min(env,(float)local/(float)c.fadeIn);
     if(c.fadeOut>0)env=std::min(env,(float)(c.length-1-local)/(float)c.fadeOut);
     for(int k=0;k<std::min(2,info.buffer->getNumChannels());++k){auto* data=scratch.getReadPointer(r.numChannels==1?0:k);float v=data[j]+f*(data[j+1]-data[j]);if(std::isfinite(v))info.buffer->addSample(k,info.startSample+(int)(p-pos)+i,v*g*env*(k?panR:panL));}
    }p+=n;
   }
  }position=pos+info.numSamples;
 }
};
}
