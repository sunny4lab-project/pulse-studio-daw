#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace pulse {
// Low-cost three-band energy and crest analysis. No neural model, no allocation.
struct MixSpectrum {
 float low=0,mid=0;std::array<float,3> energy{};float rms=0,peak=0;
 void reset(){low=mid=rms=peak=0;energy.fill(0);}
 void analyze(const float* left,const float* right,int n,double rate){
  if(!left||n<=0||rate<8000){reset();return;}
  float a=1.f-std::exp(-6.2831853f*250.f*4.f/(float)rate),b=1.f-std::exp(-6.2831853f*2000.f*4.f/(float)rate);
  std::array<double,3> sum{};double squares=0;float p=0;int count=0;
  for(int i=0;i<n;i+=4){float x=left[i];if(right)x=(x+right[i])*.5f;if(!std::isfinite(x))x=0;low+=a*(x-low);mid+=b*(x-mid);float bands[]={low,mid-low,x-mid};for(int k=0;k<3;++k)sum[k]+=bands[k]*bands[k];squares+=x*x;p=std::max(p,std::abs(x));++count;}
  float smoothing=1.f-std::exp(-(float)n/((float)rate*.35f));
  for(int k=0;k<3;++k)energy[k]+=smoothing*((float)(sum[k]/count)-energy[k]);rms+=smoothing*((float)(squares/count)-rms);peak=std::max(p,peak*(1.f-smoothing));
 }
};
inline float energyDb(float power){return 10.f*std::log10(std::max(1e-12f,power));}
inline float maskingCut(float competing,float vocal){if(vocal<1e-6f||competing<1e-6f)return 0;return -std::clamp((energyDb(competing)-energyDb(vocal)+3.f)*.25f,0.f,3.f);}
inline float masterStep(float shortLufs,float target,float peakDb,float currentDb,float anchorDb){
 if(!std::isfinite(shortLufs)||shortLufs<-60||!std::isfinite(peakDb)||!std::isfinite(currentDb)||!std::isfinite(anchorDb))return 0;
 float error=target-shortLufs;if(std::abs(error)<.8f) return 0;
 float step=std::clamp(error*.06f,-.12f,.06f);if(step>0)step=std::min(step,std::max(0.f,-2.f-peakDb));
 return std::clamp(currentDb+step,std::max(-60.f,anchorDb-3.f),std::min(0.f,anchorDb+3.f))-currentDb;
}
}
