#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <vector>
namespace pulse {
// Raw input recording, independent of mixer mute, fader and insert processing.
// All armed tracks start in the same device callback. A dropout stops the group,
// rather than silently allowing the recorded tracks to drift out of alignment.
class TrackRecorder {
public:
 struct Request {juce::String name;int left=-1,right=-1;};
private:
 struct Track {int left=-1,right=-1;juce::File file;std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;};
 juce::TimeSliceThread& disk;juce::CriticalSection mutex;
 std::vector<Track> tracks;std::atomic<bool> running{false};juce::AudioBuffer<float> zeros;
public:
 std::atomic<unsigned> dropped{0};std::atomic<int> count{0};std::atomic<juce::int64> frames{0};
 explicit TrackRecorder(juce::TimeSliceThread& thread):disk(thread){}
 ~TrackRecorder(){stop();}
 bool active()const noexcept{return running.load();}
 void invalidate()noexcept{running=false;}
 void stop(){running=false;std::vector<Track> old;{const juce::ScopedLock lock(mutex);old.swap(tracks);} }
 juce::String start(const juce::File& folder,const std::vector<Request>& requests,double rate,int inputs,int block){
  if(active())return "Recording is already active.";
  if(requests.empty()||block<=0||rate<=0||inputs<1)return "Arm at least one track with an active input.";
  for(const auto& r:requests)if(r.left<0||r.left>=inputs||r.right>=inputs||r.right<-1)return "An armed track has an inactive input.";
  stop();if(!folder.createDirectory().wasOk())return "Cannot create recording folder.";
  // Never overwrite files. Remove only files created by this failed attempt.
  std::vector<Track> next;std::vector<juce::File> created;
  auto fail=[&](const juce::String& error){next.clear();for(auto& f:created)f.deleteFile();return error;};
  juce::WavAudioFormat wav;
  for(size_t i=0;i<requests.size();++i){
   auto& r=requests[i];auto name=juce::File::createLegalFileName(r.name).substring(0,80);if(name.isEmpty())name="Audio";
   auto file=folder.getNonexistentChildFile(juce::String((int)i+1)+"-"+name,".wav");
   auto stream=file.createOutputStream();if(!stream)return fail("Could not open "+file.getFileName());
   created.push_back(file);auto* raw=wav.createWriterFor(stream.get(),rate,r.right>=0?2u:1u,24,{},0);
   if(!raw){stream.reset();return fail("Could not create WAV writer.");}stream.release();
   Track t;t.left=r.left;t.right=r.right;t.file=file;
   t.writer=std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(raw,disk,std::max(32768,block*4));next.push_back(std::move(t));
  }
  const juce::ScopedLock lock(mutex);zeros.setSize(1,block);zeros.clear();tracks.swap(next);count=(int)tracks.size();frames=0;dropped=0;running=true;return {};
 }
 void push(const float* const* in,int inputs,int n)noexcept{
  if(!running.load()||n<=0)return;
  const juce::ScopedTryLock lock(mutex);if(!lock.isLocked()){++dropped;running=false;return;}
  if(!running.load())return;
  if(n>zeros.getNumSamples()){++dropped;running=false;return;}
  for(auto& t:tracks)if(!in||t.left>=inputs||t.right>=inputs||!in[t.left]||(t.right>=0&&!in[t.right])){++dropped;running=false;return;}
  for(auto& t:tracks){const float* p[]={in[t.left],t.right>=0?in[t.right]:nullptr,nullptr};if(!t.writer->write(p,n)){++dropped;running=false;return;}}
  frames.fetch_add(n,std::memory_order_relaxed);
 }
};
}
