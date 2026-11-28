#pragma once
#include "TimelineSource.h"
namespace pulse {
// Offline arrangement-only render; no live inputs, master plugins or limiter.
inline bool renderTimelineWav(TimelineSource& source,const juce::File& file,const std::function<bool(double)>& progress,juce::String& error){
 auto total=source.getTotalLength();if(total<=0||total>700000000){error="Empty arrangement or WAV exceeds the 4 GB RIFF size limit.";return false;}
 if(file.exists()){error="Select a new output filename.";return false;}
 juce::TemporaryFile temp(file);auto stream=temp.getFile().createOutputStream();if(!stream){error="Cannot open output file.";return false;}
 juce::WavAudioFormat wav;std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.get(),timelineRate,2,24,{},0));
 if(!writer){error="Cannot create WAV writer.";return false;}stream.release();
 juce::AudioBuffer<float> buffer(2,4096);source.setNextReadPosition(0);
 for(Frame pos=0;pos<total;){
  if(progress&&!progress((double)pos/(double)total)){error="Export cancelled.";return false;}
  int n=(int)std::min<Frame>(4096,total-pos);source.getNextAudioBlock({&buffer,0,n});
  if(source.failed()){error="Source read failed during export.";return false;}
  for(int k=0;k<2;++k)for(int i=0;i<n;++i){float x=buffer.getSample(k,i);buffer.setSample(k,i,std::isfinite(x)?std::clamp(x,-1.f,1.f):0.f);}
  if(!writer->writeFromAudioSampleBuffer(buffer,0,n)){error="WAV write failed.";return false;}pos+=n;
 }
 writer.reset();if(progress&&!progress(1.0)){error="Export cancelled.";return false;}
 if(file.exists()||!temp.overwriteTargetFileWithTemporary()){error="Cannot finalize new WAV file.";return false;}error.clear();return true;
}
class TimelineExportTask final : public juce::ThreadWithProgressWindow {
 std::unique_ptr<TimelineSource> source;juce::File file;
public:
 juce::String error;bool completed=false,cancelled=false;
 TimelineExportTask(std::unique_ptr<TimelineSource> s,juce::File f):ThreadWithProgressWindow("Rendering arrangement WAV",true,true),source(std::move(s)),file(std::move(f)){}
 ~TimelineExportTask()override{signalThreadShouldExit();stopThread(-1);}
 void threadComplete(bool userPressedCancel)override{cancelled=userPressedCancel;completed=true;}
 void run()override{renderTimelineWav(*source,file,[this](double p){setProgress(p);return !threadShouldExit();},error);}
};
}
