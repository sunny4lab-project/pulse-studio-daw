#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
namespace pulse {
using Frame=juce::int64;
struct MidiNote {Frame start=0,length=24000;int pitch=60;float velocity=.8f;};
struct AutomationPoint {Frame frame=0;float value=1.f;};
inline bool validAutomation(const std::vector<AutomationPoint>& points,Frame limit,float low,float high){
 Frame last=-1;for(auto& p:points){if(p.frame<=last||p.frame>limit||!std::isfinite(p.value)||p.value<low||p.value>high)return false;last=p.frame;}return true;
}
inline float automationAt(const std::vector<AutomationPoint>& points,Frame frame,float fallback){
 if(points.empty())return fallback;
 auto it=std::upper_bound(points.begin(),points.end(),frame,[](Frame f,const AutomationPoint& p){return f<p.frame;});
 if(it==points.begin())return it->value;if(it==points.end())return points.back().value;
 auto& a=*(it-1);auto& b=*it;return a.value+(b.value-a.value)*(float)((double)(frame-a.frame)/(b.frame-a.frame));
}
inline void putAutomation(std::vector<AutomationPoint>& points,Frame frame,float value){
 auto it=std::lower_bound(points.begin(),points.end(),frame,[](const AutomationPoint& p,Frame f){return p.frame<f;});
 if(it!=points.end()&&it->frame==frame)it->value=value;else points.insert(it,{frame,value});
}
// Seekable additive instruments, no oscillator state or per-block allocation.
// Harmonics above Nyquist are omitted. Short attack/release stays inside each note.
inline float instrumentSample(int instrument,int pitch,Frame age,Frame length,float velocity,double rate){
 constexpr double tau=6.283185307179586;
 double frequency=440.0*std::exp2((pitch-69)/12.0),phase=tau*frequency*(double)age/rate;
 double env=std::min({1.0,(double)age/(rate*.005),(double)(length-1-age)/(rate*.015)});
 double v=std::sin(phase);
 if(instrument==1){if(frequency*2<rate*.45)v+=.32*std::sin(phase*2);if(frequency*3<rate*.45)v+=.14*std::sin(phase*3);v*=.68;env*=std::exp(-2.4*age/rate);}
 if(instrument==2){if(frequency*3<rate*.45)v+=.25*std::sin(phase*3);if(frequency*5<rate*.45)v+=.12*std::sin(phase*5);v*=.72;}
 if(instrument==3){if(frequency*2<rate*.45)v+=.20*std::sin(phase*2);v*=.83;env*=std::exp(-3.5*age/rate);}
 return (float)(v*std::max(0.0,env)*velocity*.22);
}
}
