#pragma once
#include <array>
#include <cmath>
#include <algorithm>
namespace pulse {
inline constexpr int eqBandCount=8;
struct ToneSettings {
 std::array<float,eqBandCount> frequency{120.f,1000.f,6500.f,60.f,250.f,500.f,2500.f,12000.f};
 std::array<float,eqBandCount> gain{};
 std::array<float,eqBandCount> q{0.8f,1.f,0.8f,1.f,1.f,1.f,1.f,1.f};
 float gate=-80.f, deess=-80.f;
 float lowPass=0.f,attackShape=0.f,saturation=0.f,limitDb=0.f;bool limit=false;
 bool bypass=false;
};
struct PeakFilter {
 float b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
 bool active=false;
 void reset(){z1=z2=0;}
 void configure(float hz,float db,float q,double rate){
  bool on=std::abs(db)>0.001f;
  if(!on){if(active)reset();active=false;return;}
  active=true;
  const double w=6.283185307179586*std::clamp((double)hz,20.0,rate*.45)/rate;
  const double a=std::pow(10.0,db/40.0),alpha=std::sin(w)/(2.0*std::clamp(q,.3f,8.f)),den=1+alpha/a;
  b0=(float)((1+alpha*a)/den);b1=(float)(-2*std::cos(w)/den);b2=(float)((1-alpha*a)/den);
  a1=b1;a2=(float)((1-alpha/a)/den);
 }
 float process(float x){if(!active)return x;float y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;if(!std::isfinite(y)){reset();return 0.f;}return y;}
};
struct ToneDSP {
 std::array<PeakFilter,eqBandCount> eq;
 float gateEnvelope=0,gateGain=1,low=0,essEnvelope=0,essGain=1;
 float gateThreshold=0,essThreshold=0,fast=0,slow=0,crossover=0;
 float lp=0,transFast=0,transSlow=0,lpCoeff=0,transA=0,transB=0,drive=1,ceiling=1;
 ToneSettings last;double lastRate=0;bool gateOn=false,essOn=false,bypassed=false;
 void reset(){for(auto& f:eq)f.reset();gateEnvelope=low=essEnvelope=lp=transFast=transSlow=0;gateGain=essGain=1;lastRate=0;}
 void prepare(const ToneSettings& s,double rate){
  rate=std::max(8000.0,rate);
  if(rate!=lastRate||s.frequency!=last.frequency||s.gain!=last.gain||s.q!=last.q){
   for(int i=0;i<eqBandCount;++i)eq[i].configure(s.frequency[i],s.gain[i],s.q[i],rate);
  }
  if(rate!=lastRate){fast=1.f-std::exp(-1.f/(.003f*(float)rate));slow=1.f-std::exp(-1.f/(.080f*(float)rate));crossover=1.f-std::exp(-6.2831853f*std::min(6000.f,(float)rate*.4f)/(float)rate);}
  bool newGate=s.gate>-79.99f,newEss=s.deess>-79.99f;
  if(newGate!=gateOn){gateEnvelope=0;gateGain=1;}
  if(newEss!=essOn){low=essEnvelope=0;essGain=1;}
  if(s.bypass!=bypassed){for(auto& f:eq)f.reset();gateEnvelope=low=essEnvelope=lp=transFast=transSlow=0;gateGain=essGain=1;}
  if(rate!=lastRate||s.lowPass!=last.lowPass){lpCoeff=1.f-std::exp(-6.2831853f*std::clamp(s.lowPass,20.f,(float)rate*.45f)/(float)rate);lp=0;}
  if(rate!=lastRate){transA=1.f-std::exp(-1.f/(.002f*(float)rate));transB=1.f-std::exp(-1.f/(.04f*(float)rate));}
  if(s.saturation!=last.saturation||rate!=lastRate)drive=1.f+5.f*std::clamp(s.saturation,0.f,1.f);
  if(s.limitDb!=last.limitDb||rate!=lastRate)ceiling=std::pow(10.f,std::clamp(s.limitDb,-18.f,0.f)/20.f);
  if(s.gate!=last.gate||rate!=lastRate)gateThreshold=std::pow(10.f,s.gate/20.f);
  if(s.deess!=last.deess||rate!=lastRate)essThreshold=std::pow(10.f,s.deess/20.f);
  gateOn=newGate;essOn=newEss;bypassed=s.bypass;last=s;lastRate=rate;
 }
 float process(float x){
  if(!std::isfinite(x))x=0;
  if(bypassed)return x;
  if(gateOn){float a=std::abs(x);gateEnvelope+=(a>gateEnvelope?fast:slow)*(a-gateEnvelope);
   float target=gateEnvelope>=gateThreshold?1.f:.0316228f;gateGain+=(target>gateGain?fast:slow)*(target-gateGain);x*=gateGain;}
  for(auto& f:eq)x=f.process(x);
  if(essOn){low+=crossover*(x-low);float high=x-low,a=std::abs(high);essEnvelope+=(a>essEnvelope?fast:slow)*(a-essEnvelope);
   float target=essEnvelope>essThreshold?std::max(.5f,essThreshold/essEnvelope):1.f;
   essGain+=(target<essGain?fast:slow)*(target-essGain);x=low+high*essGain;}
  if(last.lowPass>0){lp+=lpCoeff*(x-lp);x=lp;}
  if(std::abs(last.attackShape)>.001f){float a=std::abs(x);transFast+=transA*(a-transFast);transSlow+=transB*(a-transSlow);float contrast=std::clamp((transFast-transSlow)/(transSlow+.005f),0.f,1.f);x*=1.f+std::clamp(last.attackShape,-.8f,1.f)*contrast;}
  if(last.saturation>0){float y=x*drive;x=std::tanh(y)/drive;}
  if(last.limit)x=std::clamp(x,-ceiling,ceiling);
  return std::isfinite(x)?x:0.f;
 }
};
}
