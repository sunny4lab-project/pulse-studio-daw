#pragma once
#include "StemSource.h"
namespace pulse {
inline bool testStemSource(){
 using namespace juce;
 auto root=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("pulse-stem-test","");
 if(!root.createDirectory().wasOk())return false;
 struct Cleanup{File f;~Cleanup(){f.deleteRecursively();}} cleanup{root};
 auto write=[&](const File& f,float left,float right,int samples,double rate,int channels){
  auto stream=f.createOutputStream();if(!stream)return false;WavAudioFormat fmt;
  std::unique_ptr<AudioFormatWriter> writer(fmt.createWriterFor(stream.get(),rate,(unsigned)channels,24,{},0));if(!writer)return false;stream.release();
  AudioBuffer<float> data(channels,samples);for(int i=0;i<samples;++i){data.setSample(0,i,left);if(channels>1)data.setSample(1,i,right);}
  return writer->writeFromAudioSampleBuffer(data,0,samples);
 };
 auto a=root.getChildFile("a.wav"),b=root.getChildFile("b.wav"),c=root.getChildFile("c.wav");
 if(!write(a,.2f,.4f,16,48000,2)||!write(b,.6f,0,8,48000,1)||!write(c,.1f,.1f,16,44100,2))return false;
 AudioFormatManager formats;formats.registerBasicFormats();
 std::array<std::atomic<float>,stemCount> trims{};std::array<std::atomic<bool>,stemCount> mutes{};
 for(auto& x:trims)x=1.f;for(auto& x:mutes)x=false;
 StemSource source(trims,mutes);
 if(!source.add(std::unique_ptr<AudioFormatReader>(formats.createReaderFor(a)))||!source.add(std::unique_ptr<AudioFormatReader>(formats.createReaderFor(b)))||source.add(std::unique_ptr<AudioFormatReader>(formats.createReaderFor(c))))return false;
 AudioBuffer<float> data(2,16);AudioSourceChannelInfo info(&data,0,16);source.prepareToPlay(16,48000);source.getNextAudioBlock(info);
 auto close=[](float x,float y){return std::abs(x-y)<.0001f;};
 if(source.getTotalLength()!=16||source.getNextReadPosition()!=16||!close(data.getSample(0,0),.4f)||!close(data.getSample(1,0),.5f)||!close(data.getSample(0,10),.1f)||!close(data.getSample(1,10),.2f))return false;
 source.setNextReadPosition(0);mutes[1]=true;trims[0]=.5f;source.getNextAudioBlock(info);
 return close(data.getSample(0,0),.05f)&&close(data.getSample(1,0),.1f);
}
}
