#pragma once
#include <algorithm>
#include <array>
#include <vector>
namespace pulse {
// Standalone fixed-topology alignment planner. Not wired into the live graph.
// The longest path is the reference. Shorter channels align at their bus,
// then shorter buses and the direct playback path align at the master.
template<int Channels,int Buses> struct DelayPlan {
 std::array<int,Channels> channelDelay{};
 std::array<int,Buses> busDelay{};
 int playbackDelay=0,total=0;
 bool valid=false;
 static DelayPlan calculate(const std::array<int,Channels>& channelLatency,const std::array<int,Channels>& destination,const std::array<int,Buses>& busLatency,int playbackLatency,int cap){
  DelayPlan p;std::array<int,Buses> maxInput{};std::array<bool,Buses> used{};
  if(cap<0||playbackLatency<0||playbackLatency>cap)return p;
  for(int c=0;c<Channels;++c){int b=destination[c],l=channelLatency[c];if(b<0||b>=Buses||l<0||l>cap)return p;used[b]=true;maxInput[b]=std::max(maxInput[b],l);}
  p.total=playbackLatency;
  for(int b=0;b<Buses;++b){if(busLatency[b]<0||busLatency[b]>cap-maxInput[b])return p;if(used[b])p.total=std::max(p.total,maxInput[b]+busLatency[b]);}
  for(int c=0;c<Channels;++c)p.channelDelay[c]=maxInput[destination[c]]-channelLatency[c];
  for(int b=0;b<Buses;++b)if(used[b])p.busDelay[b]=p.total-maxInput[b]-busLatency[b];
  p.playbackDelay=p.total-playbackLatency;p.valid=true;return p;
 }
};
class StereoAlignDelay {
 std::array<std::vector<float>,2> ring;int cursor=0,delay=0;
public:
 void prepare(int cap){for(auto& x:ring)x.assign(std::max(0,cap)+1,0);cursor=delay=0;}
 bool configure(int samples){if(samples<0||samples>=(int)ring[0].size())return false;delay=samples;for(auto& x:ring)std::fill(x.begin(),x.end(),0.f);cursor=0;return true;}
 void process(float* l,float* r,int count) noexcept {
  if(!delay||ring[0].empty())return;int size=(int)ring[0].size();for(int i=0;i<count;++i){int read=(cursor+size-delay)%size;float a=l[i],b=r[i];l[i]=ring[0][read];r[i]=ring[1][read];ring[0][cursor]=a;ring[1][cursor]=b;cursor=(cursor+1)%size;}
 }
};
}
