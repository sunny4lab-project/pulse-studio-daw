#pragma once
#include <cmath>
#include <algorithm>
namespace pulse {
inline float dbGain(float db) { return std::pow(10.f, db/20.f); }
struct StripDSP {
 float low=0, envelope=0, gain=0, lastPreGain=0;
 void reset(){low=envelope=gain=lastPreGain=0;}
 float process(float x,float target,float hp,float threshold,float ratio,float attack,float release,float smooth){
  low += hp*(x-low); if(hp>0) x-=low;
  float a=std::abs(x); envelope += (a>envelope?attack:release)*(a-envelope);
  float reduction=1; if(envelope>threshold && ratio>1) reduction=std::pow(threshold/envelope,1-1/ratio);
  lastPreGain=x*reduction;
  gain += smooth*(target-gain); return lastPreGain*gain;
 }
};
inline float coefficient(float hz,double rate){return 1.f-std::exp(-6.28318530718f*hz/static_cast<float>(rate));}
}
