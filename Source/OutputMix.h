#pragma once
#include <algorithm>
#include <cmath>
namespace pulse {
constexpr int auxMixCount=3;
constexpr int maxOutputChannels=4096;
constexpr int defaultOutputChannels=2+2*auxMixCount;
// Pair numbers refer to consecutive enabled callback channels, not physical jack indices.
inline bool outputPairAvailable(float* const* out,int channels,int pair) noexcept {
 return out && pair>=0 && pair<channels/2 && 2*pair+1<channels && out[2*pair] && out[2*pair+1];
}
inline float outputMixSample(float sample,float gain) noexcept {
 return std::isfinite(sample)&&std::isfinite(gain)?sample*gain:0.f;
}
inline float safeOutputSample(float sample) noexcept {
 return std::isfinite(sample)?std::clamp(sample,-1.f,1.f):0.f;
}
}
