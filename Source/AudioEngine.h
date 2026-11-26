#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <algorithm>
#include "Dsp.h"
#include <atomic>
#include <cmath>
using namespace juce;
constexpr int N=32, B=32;
static String busName(int b){switch(b){case 0:return "Drums";case 1:return "Instruments";case 2:return "Vocals";case 3:return "Other";default:return "Bus "+String(b+1);}}
struct Channel { std::atomic<float> gain{0.25f}, pan{0}, peak{0}, postPeak{0}, hp{0}, threshold{-18}, ratio{1}; std::atomic<int> bus{3}, source{0}, rightSource{-1}; pulse::StripDSP dsp, rightDsp; std::atomic<bool> mute{true}, solo{false}; };
class Engine : public AudioIODeviceCallback {
public:
 std::array<Channel,N> ch; std::atomic<float> master{0.5f}, peak{0}; std::atomic<bool> enabled{false};
 AudioDeviceManager devices; AudioFormatManager formats; AudioTransportSource transport; TimeSliceThread disk{"Recording disk"};
 std::unique_ptr<AudioFormatReaderSource> reader; std::unique_ptr<AudioFormatWriter::ThreadedWriter> writer; CriticalSection recordLock; std::atomic<unsigned> missedWrites{0}; std::atomic<double> rate{48000}; AudioBuffer<float> playback; std::array<AudioBuffer<float>,B> buses; std::array<std::atomic<float>,B> busGain{}, busPeak{};
 AudioPluginFormatManager pluginFormats; std::unique_ptr<AudioPluginInstance> masterPlugin; AudioBuffer<float> pluginBuffer; MidiBuffer pluginMidi; std::atomic<bool> pluginBypass{false}; String deviceError; bool started=false;
 Engine(){pluginFormats.addDefaultFormats();for(auto& g:busGain)g.store(1.f);for(auto& p:busPeak)p.store(0.f);for(int i=0;i<N;++i)ch[i].source=i;formats.registerBasicFormats(); disk.startThread();}
 void startAudio(){if(started)return; started=true; deviceError=devices.initialise(0,2,nullptr,true); devices.addAudioCallback(this); }
 ~Engine() override {devices.removeAudioCallback(this); stopRecord(); transport.setSource(nullptr); disk.stopThread(2000);}
 void audioDeviceAboutToStart(AudioIODevice* d) override {rate=d->getCurrentSampleRate(); for(auto& c:ch){c.dsp.reset();c.rightDsp.reset();} playback.setSize(2,d->getCurrentBufferSizeSamples());pluginBuffer.setSize(2,d->getCurrentBufferSizeSamples());if(masterPlugin)masterPlugin->prepareToPlay(rate,d->getCurrentBufferSizeSamples()); for(auto& b:buses)b.setSize(2,d->getCurrentBufferSizeSamples());transport.prepareToPlay(d->getCurrentBufferSizeSamples(),rate);}
 void audioDeviceStopped() override {transport.releaseResources();if(masterPlugin)masterPlugin->releaseResources();}
 void audioDeviceIOCallbackWithContext(const float* const* in,int ni,float* const* out,int no,int n,const AudioIODeviceCallbackContext&) override {
  for(int c=0;c<no;++c) if(out[c]) FloatVectorOperations::clear(out[c],n);
  if(no<2 || !out[0] || !out[1] || n>playback.getNumSamples()) return;
  std::array<bool,B> used{}; bool solo=false; for(auto& c:ch) solo|=c.solo.load();
  for(int c=0;c<N;++c){auto& s=ch[c]; float p=0, post=0; int src=s.source.load(), group=std::clamp(s.bus.load(),0,B-1); if(src>=0 && src<ni && in[src]) {if(!used[group]){buses[group].clear(0,n);used[group]=true;}auto* bl=buses[group].getWritePointer(0);auto* br=buses[group].getWritePointer(1);auto* x=in[src];int rs=s.rightSource.load();const float* xr=rs>=0 && rs<ni?in[rs]:nullptr;bool stereo=rs>=0;for(int i=0;i<n;++i){p=std::max(p,std::abs(x[i]));if(xr)p=std::max(p,std::abs(xr[i]));} {float g=(enabled && !s.mute && (!solo || s.solo))?s.gain.load():0.f, a=s.pan; float l=std::sqrt((1-a)*0.5f),r=std::sqrt((1+a)*0.5f); float hp=pulse::coefficient(s.hp.load(),rate), th=pulse::dbGain(s.threshold), ratio=s.ratio, attack=pulse::coefficient(30,rate), release=pulse::coefficient(2,rate), smooth=pulse::coefficient(100,rate); for(int i=0;i<n;++i){float y=s.dsp.process(x[i],g,hp,th,ratio,attack,release,smooth);post=std::max(post,std::abs(y));if(stereo){float yr=s.rightDsp.process(xr?xr[i]:0.f,g,hp,th,ratio,attack,release,smooth);post=std::max(post,std::abs(yr));bl[i]+=y*std::min(1.f,1-a);br[i]+=yr*std::min(1.f,1+a);}else{bl[i]+=y*l;br[i]+=y*r;}}}} s.peak=p;s.postPeak=post;}
  for(int b=0;b<B;++b){if(!used[b]){busPeak[b]=0;continue;}float g=busGain[b].load(),p=0;for(int k=0;k<2;++k){auto* x=buses[b].getReadPointer(k);for(int i=0;i<n;++i){float y=x[i]*g;p=std::max(p,std::abs(y));out[k][i]+=y;}}busPeak[b]=p;}
  playback.clear(); AudioSourceChannelInfo info(&playback,0,n); transport.getNextAudioBlock(info);
  for(int c=0;c<2;++c)FloatVectorOperations::add(out[c],playback.getReadPointer(c),n);if(masterPlugin && !pluginBypass){float* ptrs[]={out[0],out[1]};AudioBuffer<float> block(ptrs,2,n);pluginMidi.clear();masterPlugin->processBlock(block,pluginMidi);}float p=0; for(int c=0;c<2;++c){ FloatVectorOperations::multiply(out[c],master.load(),n); for(int i=0;i<n;++i){p=std::max(p,std::abs(out[c][i])); out[c][i]=jlimit(-1.f,1.f,out[c][i]);}} peak=p;
  const ScopedTryLock lock(recordLock); if(lock.isLocked() && writer && !writer->write(out,n)) ++missedWrites;
 }
 String loadPlugin(const File& f){OwnedArray<PluginDescription> found;for(auto* format:pluginFormats.getFormats())if(format->fileMightContainThisPluginType(f.getFullPathName()))format->findAllTypesForFile(found,f.getFullPathName());if(found.isEmpty())return "No compatible plugin found";String error;auto* d=devices.getCurrentAudioDevice();if(!d)return "Open Audio setup first";auto instance=pluginFormats.createPluginInstance(*found[0],rate.load(),d->getCurrentBufferSizeSamples(),error);if(!instance)return error.isEmpty()?String("Plugin load failed"):error; if(instance->getTotalNumInputChannels()!=2 || instance->getTotalNumOutputChannels()!=2)return "Only stereo 2-in/2-out effects are supported";devices.removeAudioCallback(this);masterPlugin=std::move(instance);devices.addAudioCallback(this);return {};}
 bool load(File f){auto* r=formats.createReaderFor(f); if(!r)return false; transport.stop(); transport.setSource(nullptr); reader=std::make_unique<AudioFormatReaderSource>(r,true); transport.setSource(reader.get(),32768,&disk,r->sampleRate); return true;}
 bool record(File f){stopRecord(); auto stream=f.createOutputStream(); if(!stream)return false; WavAudioFormat wav; auto* w=wav.createWriterFor(stream.get(),rate,2,24,{},0); if(!w)return false; stream.release(); auto next=std::make_unique<AudioFormatWriter::ThreadedWriter>(w,disk,65536); const ScopedLock lock(recordLock); writer=std::move(next); return true;}
 void stopRecord(){std::unique_ptr<AudioFormatWriter::ThreadedWriter> old; {const ScopedLock lock(recordLock); old=std::move(writer);} }
};
