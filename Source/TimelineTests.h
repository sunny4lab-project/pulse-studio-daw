#pragma once
#include "ProjectMedia.h"
#include "TimelineExport.h"
namespace pulse {
inline bool testTimeline(){
 using namespace juce;
 auto dir=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("pulse-timeline-test","");
 if(!dir.createDirectory().wasOk())return false;
 struct Cleanup{File f;~Cleanup(){f.deleteRecursively();}} cleanup{dir};
 auto a=dir.getChildFile("mono48.wav"),b=dir.getChildFile("stereo24.wav");
 auto write=[](const File& f,double rate,int channels,int size,float l,float r){
  auto stream=f.createOutputStream();if(!stream)return false;WavAudioFormat wav;
  std::unique_ptr<AudioFormatWriter> w(wav.createWriterFor(stream.get(),rate,(unsigned)channels,24,{},0));if(!w)return false;stream.release();
  AudioBuffer<float> data(channels,size);for(int i=0;i<size;++i){data.setSample(0,i,l);if(channels>1)data.setSample(1,i,r);}return w->writeFromAudioSampleBuffer(data,0,size);
 };
 if(!write(a,48000,1,128,.2f,0)||!write(b,24000,2,64,.1f,.3f))return false;
 Engine e;TimelineSession model;TimelineTrack t1,t2;t1.name="Vocal";t2.name="Keys";AudioClip c1,c2;c1.file=a;c1.start=16;c1.offset=8;c1.length=64;c1.fadeIn=8;c1.fadeOut=8;c2.file=b;c2.start=32;c2.length=64;t1.clips.push_back(c1);t2.clips.push_back(c2);model.tracks.push_back(t1);model.tracks.push_back(t2);
 String error;auto source=TimelineSource::create(e.formats,model,error);if(!source||source->getTotalLength()!=96)return false;
 AudioBuffer<float> data(2,96);AudioSourceChannelInfo info(&data,0,96);source->getNextAudioBlock(info);
 auto close=[](float a,float b){return std::abs(a-b)<.0001f;};
 if(!close(data.getSample(0,0),0)||!close(data.getSample(0,16),0)||!close(data.getSample(0,20),.1f)||!close(data.getSample(0,40),.3f)||!close(data.getSample(1,40),.5f)||!close(data.getSample(0,90),.1f))return false;
 auto faster=model;faster.tracks.resize(1);auto& fc=faster.tracks[0].clips[0];fc.start=0;fc.offset=80;fc.length=24;fc.playbackRate=2;fc.fadeIn=fc.fadeOut=0;auto fast=TimelineSource::create(e.formats,faster,error);if(!fast)return false;fast->getNextAudioBlock(info);if(fast->failed()||!close(data.getSample(0,0),.2f)||!close(data.getSample(0,23),.2f)||!close(data.getSample(0,24),0))return false;
 if(!model.split(0,0,48)||model.tracks[0].clips.size()!=2||model.tracks[0].clips[1].offset!=40||model.tracks[0].clips[1].start!=48)return false;
 auto split=TimelineSource::create(e.formats,model,error);if(!split)return false;split->getNextAudioBlock(info);if(!close(data.getSample(0,48),.3f))return false;
 model.tracks[1].solo=true;auto solo=TimelineSource::create(e.formats,model,error);if(!solo)return false;solo->getNextAudioBlock(info);if(!close(data.getSample(0,24),0)||!close(data.getSample(1,40),.3f))return false;
 model.tracks[1].solo=false;
 if(e.setTimeline(model,0).isNotEmpty()||!e.timeline)return false;
 XmlElement root("PulseProject");root.setAttribute("version",6);ProjectMedia::save(e,root);ProjectMedia media;
 if(media.stage(e,root).isNotEmpty())return false;media.apply(e);
 if(!e.timeline||e.timeline->session().tracks.size()!=2||e.timeline->session().tracks[0].clips.size()!=2)return false;
 auto invalid=model;invalid.tracks[0].clips[0].length=10000;if(TimelineSource::create(e.formats,invalid,error))return false;
 auto render=TimelineSource::create(e.formats,model,error);auto out=dir.getChildFile("render.wav");
 if(!render||!renderTimelineWav(*render,out,{},error)||!out.existsAsFile())return false;
 std::unique_ptr<AudioFormatReader> rd(e.formats.createReaderFor(out));if(!rd||rd->numChannels!=2||rd->lengthInSamples!=96||rd->sampleRate!=48000)return false;
 if(!rd->read(&data,0,96,0,true,true)||!close(data.getSample(1,40),.5f))return false;
 auto cancelled=dir.getChildFile("cancel.wav");if(renderTimelineWav(*render,cancelled,[](double){return false;},error)||cancelled.exists())return false;
 // Missing clip references must not replace the current valid session.
 auto* clip=root.getChildByName("Timeline")->getChildElement(0)->getChildElement(0);clip->setAttribute("path",dir.getChildFile("missing.wav").getFullPathName());
 ProjectMedia bad;return bad.stage(e,root).isNotEmpty()&&e.timeline&&e.timeline->session().tracks.size()==2;
}
}
