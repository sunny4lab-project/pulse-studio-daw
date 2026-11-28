#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
namespace pulse {
// One synchronized interleaved 24-bit WAV, one channel per active device input.
// Disk writer owns a FIFO; callback never opens files or waits for disk I/O.
class MultitrackRecorder {
 juce::TimeSliceThread& disk;
 juce::CriticalSection mutex;
 std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
 std::atomic<bool> writing{false};
 int channels=0;
 juce::AudioBuffer<float> zeros;
public:
 std::atomic<unsigned> missedBlocks{0};
 explicit MultitrackRecorder(juce::TimeSliceThread& d):disk(d){}
 ~MultitrackRecorder(){stop();}
 bool active() const noexcept{return writing.load();}
 void invalidate() noexcept {writing=false;}
 void stop(){writing=false;std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> old;{const juce::ScopedLock lock(mutex);old=std::move(writer);channels=0;}}
 bool start(const juce::File& file,double rate,int count,int blockSize){
  if(count<1||count>32||blockSize<1||rate<=0||file.exists())return false;
  stop();auto stream=file.createOutputStream();if(!stream)return false;juce::WavAudioFormat format;
  auto* wav=format.createWriterFor(stream.get(),rate,(unsigned)count,24,{},0);
  if(!wav)return false;stream.release();
  auto next=std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(wav,disk,32768);
  const juce::ScopedLock lock(mutex);zeros.setSize(1,blockSize);zeros.clear();channels=count;writer=std::move(next);missedBlocks=0;writing=true;return true;
 }
 void push(const float* const* input,int count,int samples) noexcept {
  if(!writing.load()||samples<=0)return;
  const juce::ScopedTryLock lock(mutex);if(!lock.isLocked()){++missedBlocks;return;}
  if(!writing.load()||!writer)return;
  if(samples>zeros.getNumSamples()||count!=channels){writing=false;++missedBlocks;return;}
  std::array<const float*,33> ptr{};for(int i=0;i<channels;++i)ptr[i]=(input&&input[i])?input[i]:zeros.getReadPointer(0);
  if(!writer->write(ptr.data(),samples))++missedBlocks;
 }
};
}
