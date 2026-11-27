#pragma once
#include <algorithm>
#include <cmath>

namespace pulse {
// Fixed-cost, deterministic fader advisor. 1=Balanced, 2=Praise,
// 3=Worship, 4=Sermon. Role IDs follow the mixer role selector.
struct AutoMixControl {
 static float targetDb(int role,int profile,bool speechActive) noexcept {
  constexpr float targets[4][7] = {
   {-24.f,-20.f,-18.f,-27.f,-24.f,-25.f,-28.f},
   {-24.f,-18.f,-18.f,-25.f,-22.f,-24.f,-25.f},
   {-25.f,-19.f,-18.f,-24.f,-27.f,-27.f,-26.f},
   {-27.f,-25.f,-17.f,-30.f,-32.f,-32.f,-31.f}
  };
  const int scene=std::clamp(profile,1,4)-1;
  const int channelRole=std::clamp(role,1,7)-1;
  float db=targets[scene][channelRole];
  // Speech side-chain: gentle priority, never a sudden hard mute.
  if(speechActive&&(role==4||role==5||role==6||role==7))db-=3.f;
  return db;
 }
 static float gainForDb(float db) noexcept {return std::pow(10.f,db/20.f);}
 static float propose(float anchor,float inputEnvelope,int role,int profile,bool speechActive) noexcept {
  if(!std::isfinite(anchor)||anchor<.001f||!std::isfinite(inputEnvelope)||inputEnvelope<=.0032f)return anchor;
  const float desired=gainForDb(targetDb(role,profile,speechActive))/inputEnvelope;
  const float low=anchor*gainForDb(-6.f),high=std::min(1.5f,anchor*gainForDb(3.f));
  return std::clamp(desired,low,std::max(low,high));
 }
 static float ride(float current,float target,bool reductionsOnly) noexcept {
  if(!std::isfinite(current)||!std::isfinite(target)||current<=0||target<=0)return current;
  const float error=20.f*std::log10(target/current);
  if(std::abs(error)<.4f||(reductionsOnly&&error>0))return current;
  // 15 Hz UI tick: at most 1.8 dB/s down and 0.6 dB/s up.
  const float step=std::clamp(error,-.12f,.04f);
  return std::clamp(current*gainForDb(step),0.f,1.5f);
 }
};
}
