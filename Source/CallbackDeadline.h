#pragma once
#include <atomic>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>

namespace pulse {
// Lock-free callback telemetry: 1000 units = 100% of the device buffer deadline.
// A deadline miss is a warning, not proof that a driver actually dropped audio.
struct CallbackDeadline {
 std::atomic<std::uint32_t> latestPermille{0}, peakPermille{0}, missedDeadlines{0};

 void reset() noexcept {
  latestPermille.store(0,std::memory_order_relaxed);
  peakPermille.store(0,std::memory_order_relaxed);
  missedDeadlines.store(0,std::memory_order_relaxed);
 }

 void record(double seconds,int frames,double sampleRate) noexcept {
  if(frames<=0||!std::isfinite(seconds)||seconds<0||!std::isfinite(sampleRate)||sampleRate<=0)return;
  const double budget=(double)frames/sampleRate;
  const auto load=static_cast<std::uint32_t>(std::clamp(seconds/budget*1000.0,0.0,100000.0));
  latestPermille.store(load,std::memory_order_relaxed);
  auto peak=peakPermille.load(std::memory_order_relaxed);
  while(peak<load&&!peakPermille.compare_exchange_weak(peak,load,std::memory_order_relaxed)){}
  if(seconds>budget)missedDeadlines.fetch_add(1,std::memory_order_relaxed);
 }

 struct Scope {
  CallbackDeadline& meter;
  int frames;
  double sampleRate;
  std::chrono::steady_clock::time_point started=std::chrono::steady_clock::now();
  Scope(CallbackDeadline& m,int n,double rate)noexcept:meter(m),frames(n),sampleRate(rate){}
  ~Scope()noexcept {
   meter.record(std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count(),frames,sampleRate);
  }
  Scope(const Scope&)=delete;
  Scope& operator=(const Scope&)=delete;
 };
};
}
