#pragma once
#include <array>
#include <algorithm>
#include <cmath>
namespace pulse {
// Stereo-linked, zero-lookahead sample-peak limiter. Not a true-peak limiter.
struct MasterLimiter {
 float gain=1.f,reduction=0.f;
 void reset(){gain=1;reduction=0;}
 void process(float* l,float* r,int n,double rate,float ceilingDb,float releaseMs,bool enabled){
  if(!enabled){reset();return;}
  const float ceiling=std::pow(10.f,ceilingDb/20.f), release=1.f-std::exp(-1000.f/(std::max(10.f,releaseMs)*(float)std::max(8000.0,rate)));
  float lowest=1.f;
  for(int i=0;i<n;++i){float a=std::isfinite(l[i])?l[i]:0.f,b=std::isfinite(r[i])?r[i]:0.f;
   float peak=std::max(std::abs(a),std::abs(b)), target=peak>ceiling?ceiling/peak:1.f;
   gain=target<gain?target:std::min(target,gain+release*(target-gain));
   l[i]=a*gain;r[i]=b*gain;lowest=std::min(lowest,gain);
  }
  reduction=-20.f*std::log10(std::max(lowest,1.e-9f));
 }
};
// K weighting coefficients calculated from the BS.1770 filter design.
struct MeterBiquad {
 double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
 double process(double x){double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return y;}
 void shelf(double rate){double k=std::tan(3.141592653589793*1681.974450955533/rate),q=.7071752369554196,vh=std::pow(10.,3.999843853973347/20.),vb=std::pow(vh,.4996667741545416),a=1+k/q+k*k;b0=(vh+vb*k/q+k*k)/a;b1=2*(k*k-vh)/a;b2=(vh-vb*k/q+k*k)/a;a1=2*(k*k-1)/a;a2=(1-k/q+k*k)/a;z1=z2=0;}
 void highpass(double rate){double k=std::tan(3.141592653589793*38.13547087602444/rate),q=.5003270373238773,a=1+k/q+k*k;b0=1;b1=-2;b2=1;a1=2*(k*k-1)/a;a2=(1-k/q+k*k)/a;z1=z2=0;}
};
class LoudnessMeter {
 static constexpr int bins=801;
 std::array<double,bins> power{};std::array<unsigned long long,bins> counts{};
 std::array<MeterBiquad,2> shelf,hp;
 std::array<double,30> chunks{};int cursor=0,filled=0,samples=0,hop=4800;
 double energy=0;
 float momentary=-100,shortTerm=-100,integrated=-100,peakEstimate=0,heldPeak=0;
 // Four-phase cubic interpolation estimate. Not IEC/ITU conformance tested.
 std::array<std::array<float,4>,2> prev{};
 static float lufs(double mean){return mean>1e-12?(float)(-.691+10*std::log10(mean)):-100.f;}
 void step(){
  chunks[cursor]=energy/hop;cursor=(cursor+1)%30;filled=std::min(30,filled+1);energy=0;samples=0;
  double m=0,s=0;for(int j=0;j<filled;++j){double v=chunks[(cursor+29-j)%30];s+=v;if(j<4)m+=v;}
  if(filled>=30)shortTerm=lufs(s/30.);
  if(filled<4)return;
  double block=m/4.;momentary=lufs(block);
  if(momentary>=-70){int index=std::clamp((int)std::floor((momentary+70)*10),0,bins-1);power[index]+=block;++counts[index];}
  double total=0;unsigned long long num=0;for(int i=0;i<bins;++i){total+=power[i];num+=counts[i];}
  if(!num){integrated=-100;return;}
  float gate=std::max(-70.f,lufs(total/num)-10.f);int first=std::clamp((int)std::ceil((gate+70)*10),0,bins-1);
  total=0;num=0;for(int i=first;i<bins;++i){total+=power[i];num+=counts[i];}
  integrated=num?lufs(total/num):-100.f;
 }
public:
 void reset(double rate){rate=std::max(8000.,rate);for(int c=0;c<2;++c){shelf[c].shelf(rate);hp[c].highpass(rate);}power.fill(0);counts.fill(0);chunks.fill(0);prev={};cursor=filled=samples=0;hop=std::max(1,(int)std::round(rate*.1));energy=0;momentary=shortTerm=integrated=-100;peakEstimate=heldPeak=0;}
 void process(const float* l,const float* r,int n){
  peakEstimate=0;
  for(int i=0;i<n;++i){for(int c=0;c<2;++c){float x=c?r[i]:l[i];if(!std::isfinite(x))x=0;double y=hp[c].process(shelf[c].process(x));energy+=y*y;
   auto& p=prev[c];p[0]=p[1];p[1]=p[2];p[2]=p[3];p[3]=x;
   peakEstimate=std::max(peakEstimate,std::abs(x));for(float t:{.25f,.5f,.75f}){float v=.5f*((2*p[1])+(-p[0]+p[2])*t+(2*p[0]-5*p[1]+4*p[2]-p[3])*t*t+(-p[0]+3*p[1]-3*p[2]+p[3])*t*t*t);peakEstimate=std::max(peakEstimate,std::abs(v));}}
   if(++samples>=hop)step();
  }
  heldPeak=std::max(heldPeak,peakEstimate);
 }
 float getMomentary()const{return momentary;}float getShortTerm()const{return shortTerm;}float getIntegrated()const{return integrated;}float getPeakEstimate()const{return peakEstimate;}float getHeldPeak()const{return heldPeak;}
};
}
