#pragma once
#include <algorithm>
#include <cmath>
namespace pulse {
constexpr int auxMixCount=3;
constexpr int maxOutputChannels=2+2*auxMixCount;
// Master is pair zero. Auxiliary buses use callback slots 2/3, 4/5, 6/7.
inline bool outputPairAvailable(float* const* out,int channels,int pair) noexcept {
 return out && pair>=0 && pair<=auxMixCount && 2*pair+1<channels && out[2*pair] && out[2*pair+1];
}
inline float outputMixSample(float sample,float gain) noexcept {
 return std::isfinite(sample)&&std::isfinite(gain)?sample*gain:0.f;
}
inline float safeOutputSample(float sample) noexcept {
 return std::isfinite(sample)?std::clamp(sample,-1.f,1.f):0.f;
}
}
