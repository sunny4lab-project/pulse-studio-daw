#pragma once
#include "TestDiagnostics.h"
#include "AudioEngine.h"
namespace pulse {
inline bool testTrackUpgrade(){
 using namespace juce;
 // Verify input trim, polarity and DCA scaling through the real engine.
 Engine e;if(!e.addChannels({0,1}))return pulse::testFailure(__FILE__,__LINE__);
 constexpr int block=256;e.playback.setSize(2,block);e.pluginBuffer.setSize(2,block);
 for(auto& b:e.buses)b.setSize(2,block);
 for(auto& c:e.ch){c.gain=1;c.mute=false;c.pan=0;}
 e.ch[0].dca=0;e.ch[1].dca=1;e.dcaGain[0]=.5f;e.dcaMute[1]=true;e.ch[0].trimDb=6.0206f;e.ch[0].polarity=true;e.master=1;e.enabled=true;
 AudioBuffer<float> input(2,block),output(2,block);
 for(int i=0;i<block;++i){input.setSample(0,i,.2f);input.setSample(1,i,.6f);}
 const float* in[]={input.getReadPointer(0),input.getReadPointer(1)};
 float* out[]={output.getWritePointer(0),output.getWritePointer(1)};
 for(int i=0;i<32;++i)e.audioDeviceIOCallbackWithContext(in,2,out,2,block,{});
 if(std::abs(output.getSample(0,block-1)+.2f*std::sqrt(.5f))>.001f)return pulse::testFailure(__FILE__,__LINE__);
 e.dcaMute[0]=true;for(int i=0;i<16;++i)e.audioDeviceIOCallbackWithContext(in,2,out,2,block,{});
 if(output.getMagnitude(0,block)>.001f)return pulse::testFailure(__FILE__,__LINE__);
 e.enabled=false;
 // Eighth band must actually change the response; optional ceiling must constrain samples.
 ToneSettings tone;ToneDSP dsp;tone.frequency[7]=1000;tone.gain[7]=6;dsp.prepare(tone,48000);
 double sum=0;for(int i=0;i<48000;++i){float v=dsp.process(.1f*std::sin(6.28318530718*1000*i/48000.0));if(i>=24000)sum+=v*v;}
 if(std::sqrt(sum/24000)<.135)return pulse::testFailure(__FILE__,__LINE__);
 tone.gain[7]=0;tone.limit=true;tone.limitDb=-6;dsp.prepare(tone,48000);if(std::abs(dsp.process(1.f))>.502f)return pulse::testFailure(__FILE__,__LINE__);
 auto folder=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("pulse-armed-test","");
 struct Cleanup{File f;~Cleanup(){f.deleteRecursively();}} cleanup{folder};
 TimeSliceThread disk("armed test");disk.startThread();TrackRecorder recorder(disk);
 if(recorder.start(folder,{{"Bad",5,-1}},48000,2,block).isEmpty())return pulse::testFailure(__FILE__,__LINE__);
 if(recorder.start(folder,{{"Vocal",0,-1},{"Keys",1,0}},48000,2,block).isNotEmpty())return pulse::testFailure(__FILE__,__LINE__);
 recorder.push(in,2,block);recorder.push(in,2,block);recorder.stop();
 if(recorder.dropped.load()!=0||recorder.frames.load()!=2*block)return pulse::testFailure(__FILE__,__LINE__);
 auto files=folder.findChildFiles(File::findFiles,false,"*.wav");if(files.size()!=2)return pulse::testFailure(__FILE__,__LINE__);
 WavAudioFormat wav;
 for(auto& f:files){std::unique_ptr<AudioFormatReader> reader(wav.createReaderFor(f.createInputStream().release(),true));bool stereo=f.getFileName().startsWith("2-");if(!reader||reader->numChannels!=(stereo?2u:1u)||reader->lengthInSamples!=2*block)return pulse::testFailure(__FILE__,__LINE__);
  AudioBuffer<float> read(stereo?2:1,2*block);if(!reader->read(&read,0,2*block,0,true,true))return pulse::testFailure(__FILE__,__LINE__);
  if(std::abs(read.getSample(0,block)-(stereo?.6f:.2f))>.001f||(stereo&&std::abs(read.getSample(1,block)-.2f)>.001f))return pulse::testFailure(__FILE__,__LINE__);
 }
 if(recorder.start(folder,{{"Drop",0,-1}},48000,2,block).isNotEmpty())return pulse::testFailure(__FILE__,__LINE__);
 recorder.push(nullptr,2,block);bool ok=!recorder.active()&&recorder.dropped.load()==1;recorder.stop();disk.stopThread(2000);return ok;
}
}
