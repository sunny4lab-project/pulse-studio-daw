#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <algorithm>
#include "Dsp.h"
#include "ToneDSP.h"
#include "StemSource.h"
#include "OutputMix.h"
#include "InsertRack.h"
#include "MultitrackRecorder.h"
#include "MasterDSP.h"
#include "SendEffects.h"
#include "MixerTopology.h"
#include <atomic>
#include <cmath>
using namespace juce;
constexpr int B=pulse::busCount;
static String busName(int b){switch(b){case 0:return "Drums";case 1:return "Instruments";case 2:return "Vocals";case 3:return "Other";default:return "Bus "+String(b+1);}}
struct Channel { std::atomic<float> gain{0.25f}, pan{0}, peak{0}, postPeak{0}, hp{0}, threshold{-18}, ratio{1}; std::atomic<int> bus{3}, source{-1}, rightSource{-1}; pulse::StripDSP dsp, rightDsp; pulse::ToneDSP toneLeft,toneRight; std::array<std::atomic<float>,3> eqFreq{{120.f,1000.f,6500.f}},eqGain{{0.f,0.f,0.f}},eqQ{{.8f,1.f,.8f}};std::atomic<float> gateThreshold{-80.f},deessThreshold{-80.f};std::atomic<bool> toneBypass{false};
 pulse::ToneSettings toneSettings() const {pulse::ToneSettings t;for(int i=0;i<3;++i){t.frequency[i]=eqFreq[i].load();t.gain[i]=eqGain[i].load();t.q[i]=eqQ[i].load();}t.gate=gateThreshold.load();t.deess=deessThreshold.load();t.bypass=toneBypass.load();return t;}
  std::atomic<bool> mute{true}, solo{false}; };
class Engine : public AudioIODeviceCallback {
public:
 pulse::StableList<Channel> ch; std::atomic<float> master{0.5f}, peak{0}; std::atomic<bool> enabled{false};
 AudioDeviceManager devices; AudioFormatManager formats; AudioTransportSource transport; TimeSliceThread disk{"Recording disk"}, stemDisk{"Stem read ahead"};
 pulse::MultitrackRecorder multitrack{disk}; pulse::SendEffects sends; pulse::MasterLimiter limiter; pulse::LoudnessMeter loudness;
 std::atomic<bool> limiterEnabled{false},resetLoudness{false};std::atomic<float> limiterCeiling{-1.f},limiterRelease{100.f},limiterReduction{0.f},momentaryLufs{-100.f},shortLufs{-100.f},integratedLufs{-100.f},estimatedTruePeak{0.f};
 std::unique_ptr<pulse::StemSource> reader; std::array<std::atomic<float>,pulse::stemCount> stemTrims{}; std::array<std::atomic<bool>,pulse::stemCount> stemMutes{}; Array<File> stemFiles; std::atomic<float> rms{0}; std::atomic<unsigned> clippedBlocks{0}; std::unique_ptr<AudioFormatWriter::ThreadedWriter> writer; CriticalSection recordLock; std::atomic<unsigned> missedWrites{0}; std::atomic<double> rate{48000}; AudioBuffer<float> playback; std::array<AudioBuffer<float>,B> buses; std::array<std::atomic<float>,B> busGain{}, busPeak{};
 std::array<std::atomic<float>,B*pulse::auxMixCount> auxSend{}; std::array<std::atomic<float>,pulse::auxMixCount> auxGain{},auxPeak{}; std::array<std::atomic<bool>,pulse::auxMixCount> auxArmed{},auxAvailable{};
 void disarmAux() noexcept {for(auto& a:auxArmed)a=false;}
 float getAuxSend(int bus,int mix) const noexcept {return auxSend[bus*pulse::auxMixCount+mix].load();}
 AudioPluginFormatManager pluginFormats; pulse::StableList<pulse::InsertRack> channelInserts;std::array<pulse::InsertRack,B> busInserts;
 int channelCount()const{return ch.size();}
 int rackCount()const{return channelCount()+B;}
 pulse::InsertRack& rackAt(int i){return i<channelCount()?channelInserts[i]:busInserts[i-channelCount()];}
 const pulse::InsertRack& rackAt(int i)const{return i<channelCount()?channelInserts[i]:busInserts[i-channelCount()];}
 // Call only while idle, with UI references to removed channels released.
 bool addChannel(){if(!canEditInserts())return false;struct Pause{Engine& e;explicit Pause(Engine& x):e(x){if(e.started)e.devices.removeAudioCallback(&e);}~Pause(){if(e.started)e.devices.addAudioCallback(&e);}} pause(*this);try{auto& c=ch.append();try{channelInserts.append();}catch(...){ch.erase(ch.size()-1);throw;}c.source=-1;return true;}catch(const std::bad_alloc&){return false;}}
 bool removeChannel(int i){if(i<0||i>=channelCount()||!canEditInserts())return false;if(started)devices.removeAudioCallback(this);channelInserts.erase(i);ch.erase(i);if(started)devices.addAudioCallback(this);return true;}
 std::unique_ptr<AudioPluginInstance> masterPlugin; AudioBuffer<float> pluginBuffer; MidiBuffer pluginMidi; std::atomic<bool> pluginBypass{false}; String deviceError; bool started=false;
 Engine(){for(auto& v:auxSend)v=0.f;for(auto& v:auxGain)v=0.25f;for(auto& v:auxPeak)v=0.f;for(auto& a:auxArmed)a=false;for(auto& a:auxAvailable)a=false;for(auto& t:stemTrims)t=1.f;for(auto& m:stemMutes)m=false;juce::addDefaultFormatsToManager(pluginFormats);for(auto& g:busGain)g.store(1.f);for(auto& p:busPeak)p.store(0.f);formats.registerBasicFormats(); disk.startThread();stemDisk.startThread();}
 static File deviceSettingsFile(){return File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("PulseStudio").getChildFile("audio-device.xml");}
 String saveDeviceSettings(){
  if(!canEditInserts())return "Stop live audio, playback and recording before saving device settings.";
  auto* d=devices.getCurrentAudioDevice();if(!d||!d->isPlaying())return "No active audio device.";
  auto xml=devices.createStateXml();
  if(!xml)return "Select the driver and channel configuration in Device first.";
  auto file=deviceSettingsFile();
  if(!file.getParentDirectory().createDirectory().wasOk()||!xml->writeTo(file))return "Could not write audio-device settings.";
  return {};
 }
 void startAudio(){if(started)return;started=true;auto file=deviceSettingsFile();auto saved=file.existsAsFile()&&file.getSize()<1024*1024?XmlDocument::parse(file):nullptr;
  // Restore the last explicitly saved driver, sample rate, buffer and channel masks.
  // Never arm live monitoring or auxiliary outputs automatically.
  enabled=false;disarmAux();deviceError=devices.initialise(0,2,saved.get(),true);devices.addAudioCallback(this);
 }
 StringArray activeChannelNames(bool input){
  StringArray result;auto* d=devices.getCurrentAudioDevice();if(!d)return result;
  auto names=input?d->getInputChannelNames():d->getOutputChannelNames();
  auto mask=input?d->getActiveInputChannels():d->getActiveOutputChannels();
  for(int i=mask.findNextSetBit(0);i>=0;i=mask.findNextSetBit(i+1))
   result.add(String(i+1)+" | "+(i<names.size()?names[i]:String(input?"Input":"Output")));
  return result;
 }
 StringArray discoverAsioDrivers(){
  StringArray result;
  if(!canEditInserts())return result;
  for(auto* type:devices.getAvailableDeviceTypes())if(type->getTypeName().equalsIgnoreCase("ASIO")){
   type->scanForDevices();result.addArray(type->getDeviceNames(false));
  }
  return result;
 }

 ~Engine() override {devices.removeAudioCallback(this); multitrack.stop();stopRecord(); transport.setSource(nullptr); stemDisk.stopThread(2000);disk.stopThread(2000);}
 void audioDeviceAboutToStart(AudioIODevice* d) override {disarmAux();for(auto& a:auxAvailable)a=false;rate=d->getCurrentSampleRate();multitrack.invalidate();sends.prepare(rate.load(),d->getCurrentBufferSizeSamples());limiter.reset();loudness.reset(rate.load());for(auto& c:ch){c.dsp.reset();c.rightDsp.reset();c.toneLeft.reset();c.toneRight.reset();} playback.setSize(2,d->getCurrentBufferSizeSamples());pluginBuffer.setSize(2,d->getCurrentBufferSizeSamples());if(masterPlugin)masterPlugin->prepareToPlay(rate,d->getCurrentBufferSizeSamples());for(auto& rack:channelInserts)rack.prepare(rate,d->getCurrentBufferSizeSamples());for(auto& rack:busInserts)rack.prepare(rate,d->getCurrentBufferSizeSamples());pluginMidi.ensureSize(2048); for(auto& b:buses)b.setSize(2,d->getCurrentBufferSizeSamples());transport.prepareToPlay(d->getCurrentBufferSizeSamples(),rate);}
 void audioDeviceStopped() override {enabled=false;multitrack.invalidate();limiterReduction=0;momentaryLufs=-100;shortLufs=-100;integratedLufs=-100;estimatedTruePeak=0;disarmAux();for(auto& a:auxAvailable)a=false;for(auto& p:auxPeak)p=0;peak=0;rms=0;transport.releaseResources();if(masterPlugin)masterPlugin->releaseResources();for(auto& rack:channelInserts)rack.release();for(auto& rack:busInserts)rack.release();}
 void audioDeviceIOCallbackWithContext(const float* const* in,int ni,float* const* out,int no,int n,const AudioIODeviceCallbackContext&) override {
  ScopedNoDenormals noDenormals;
  if(n<=0)return;multitrack.push(in,ni,n);if(!out)return;
  for(int c=0;c<no;++c) if(out[c]) FloatVectorOperations::clear(out[c],n);
  std::array<bool,pulse::auxMixCount> auxActive{};for(int a=0;a<pulse::auxMixCount;++a){bool available=pulse::outputPairAvailable(out,no,a+1);auxAvailable[a]=available;auxActive[a]=available&&auxArmed[a].load()&&enabled.load();auxPeak[a]=0;}
  if(!pulse::outputPairAvailable(out,no,0)||n<=0||n>playback.getNumSamples()) {peak=0;rms=0;return;}
  sends.begin(n);std::array<bool,B> used{}; bool solo=false; for(auto& c:ch) solo|=c.solo.load();
  for(int c=0;c<channelCount();++c){auto& s=ch[c]; float p=0, post=0; int src=s.source.load(), group=std::clamp(s.bus.load(),0,B-1); if(in && src>=0 && src<ni && in[src]) {if(!used[group]){buses[group].clear(0,n);used[group]=true;}auto* bl=buses[group].getWritePointer(0);auto* br=buses[group].getWritePointer(1);auto* x=in[src];int rs=s.rightSource.load();const float* xr=rs>=0 && rs<ni?in[rs]:nullptr;bool stereo=rs>=0;for(int i=0;i<n;++i){p=std::max(p,std::abs(x[i]));if(xr)p=std::max(p,std::abs(xr[i]));} {float g=(enabled && !s.mute && (!solo || s.solo))?s.gain.load():0.f, a=s.pan; float l=std::sqrt((1-a)*0.5f),r=std::sqrt((1+a)*0.5f); float hp=pulse::coefficient(s.hp.load(),rate), th=pulse::dbGain(s.threshold), ratio=s.ratio, attack=pulse::coefficient(30,rate), release=pulse::coefficient(2,rate), smooth=pulse::coefficient(100,rate); bool insertActive=g>0.f&&channelInserts[c].active()&&pluginBuffer.getNumSamples()>=n;auto* il=insertActive?pluginBuffer.getWritePointer(0):nullptr;auto* ir=insertActive?pluginBuffer.getWritePointer(1):nullptr;auto tone=s.toneSettings();s.toneLeft.prepare(tone,rate.load());if(stereo)s.toneRight.prepare(tone,rate.load());for(int i=0;i<n;++i){float y=s.dsp.process(s.toneLeft.process(x[i]),g,hp,th,ratio,attack,release,smooth);post=std::max(post,std::abs(y));if(stereo){float yr=s.rightDsp.process(s.toneRight.process(xr?xr[i]:0.f),g,hp,th,ratio,attack,release,smooth);post=std::max(post,std::abs(yr));float left=y*std::min(1.f,1-a),right=yr*std::min(1.f,1+a);if(insertActive){il[i]=left;ir[i]=right;}else{bl[i]+=left;br[i]+=right;}}else{if(insertActive){il[i]=y*l;ir[i]=y*r;}else{bl[i]+=y*l;br[i]+=y*r;}}}if(insertActive){channelInserts[c].process(il,ir,n,pluginMidi);post=0;for(int i=0;i<n;++i){post=std::max(post,std::max(std::abs(il[i]),std::abs(ir[i])));bl[i]+=il[i];br[i]+=ir[i];}}}} s.peak=p;s.postPeak=post;}
  for(int b=0;b<B;++b){if(!used[b]){busPeak[b]=0;continue;}if(enabled.load())busInserts[b].process(buses[b].getWritePointer(0),buses[b].getWritePointer(1),n,pluginMidi);float g=busGain[b].load(),p=0;std::array<float,pulse::auxMixCount> send{};for(int a=0;a<pulse::auxMixCount;++a)if(auxActive[a])send[a]=getAuxSend(b,a);for(int k=0;k<2;++k){auto* x=buses[b].getReadPointer(k);for(int i=0;i<n;++i){float y=pulse::outputMixSample(x[i],g);p=std::max(p,std::abs(y));out[k][i]+=y;}sends.send(b,k,x,g,n);for(int a=0;a<pulse::auxMixCount;++a)if(send[a]>0.f)FloatVectorOperations::addWithMultiply(out[2*(a+1)+k],x,g*send[a],n);}busPeak[b]=p;}
  for(int a=0;a<pulse::auxMixCount;++a)if(auxActive[a]){float p=0,g=auxGain[a].load();for(int k=0;k<2;++k){auto* x=out[2*(a+1)+k];for(int i=0;i<n;++i){float y=pulse::outputMixSample(x[i],g);p=std::max(p,std::abs(y));x[i]=pulse::safeOutputSample(y);}}auxPeak[a]=p;}
  sends.render(out[0],out[1],n);
  playback.clear(); AudioSourceChannelInfo info(&playback,0,n); transport.getNextAudioBlock(info);
  for(int c=0;c<2;++c)FloatVectorOperations::add(out[c],playback.getReadPointer(c),n);if(masterPlugin && !pluginBypass){float* ptrs[]={out[0],out[1]};AudioBuffer<float> block(ptrs,2,n);pluginMidi.clear();masterPlugin->processBlock(block,pluginMidi);}float p=0;double squares=0;float masterGain=master.load();for(int c=0;c<2;++c)FloatVectorOperations::multiply(out[c],masterGain,n);
  limiter.process(out[0],out[1],n,rate.load(),limiterCeiling.load(),limiterRelease.load(),limiterEnabled.load());limiterReduction=limiter.reduction;
  for(int c=0;c<2;++c){for(int i=0;i<n;++i){p=std::max(p,std::abs(out[c][i]));out[c][i]=pulse::safeOutputSample(out[c][i]);squares+=out[c][i]*out[c][i];}}peak=p;
  if(resetLoudness.exchange(false))loudness.reset(rate.load());loudness.process(out[0],out[1],n);momentaryLufs=loudness.getMomentary();shortLufs=loudness.getShortTerm();integratedLufs=loudness.getIntegrated();estimatedTruePeak=loudness.getHeldPeak();rms=n>0?(float)std::sqrt(squares/(2.0*n)):0.f;if(p>1.f)++clippedBlocks;
  const ScopedTryLock lock(recordLock); if(lock.isLocked() && writer && !writer->write(out,n)) ++missedWrites;
 }
 bool canEditInserts(){const ScopedLock lock(recordLock);return !enabled.load()&&!transport.isPlaying()&&!writer&&!multitrack.active();}
 String changeInsert(int rack,int slot,const File& file){
  if(rack<0||rack>=rackCount()||slot<0||slot>=pulse::insertsPerRack)return "Invalid insert slot";
  if(!canEditInserts())return "Stop playback and recording and disable live inputs first.";
  auto* d=devices.getCurrentAudioDevice();if(!d||!d->isPlaying())return "Open Audio setup first";
  std::unique_ptr<AudioPluginInstance> instance;
  if(file!=File{}){
   OwnedArray<PluginDescription> found;for(auto* format:pluginFormats.getFormats())if(format->fileMightContainThisPluginType(file.getFullPathName()))format->findAllTypesForFile(found,file.getFullPathName());
   if(found.isEmpty())return "No compatible plugin found";
   String error;instance=pluginFormats.createPluginInstance(*found[0],rate.load(),d->getCurrentBufferSizeSamples(),error);
   if(!instance)return error.isEmpty()?String("Plugin load failed"):error;
   if(found[0]->isInstrument||instance->getTotalNumInputChannels()!=2||instance->getTotalNumOutputChannels()!=2)return "Only stereo 2-in/2-out effects are supported";
   instance->prepareToPlay(rate.load(),d->getCurrentBufferSizeSamples());int latency=instance->getLatencySamples();instance->releaseResources();
   if(latency!=0)return "Only zero-reported-latency effects are supported until delay compensation is implemented.";
  }
  if(!canEditInserts())return "Transport or live state changed. Stop audio first.";
  devices.removeAudioCallback(this);auto& target=rackAt(rack).slots[slot];target.plugin=std::move(instance);target.bypass=false;devices.addAudioCallback(this);return {};
 }
 String loadPlugin(const File& f){if(!canEditInserts())return "Stop playback and recording and disable live inputs first.";OwnedArray<PluginDescription> found;for(auto* format:pluginFormats.getFormats())if(format->fileMightContainThisPluginType(f.getFullPathName()))format->findAllTypesForFile(found,f.getFullPathName());if(found.isEmpty())return "No compatible plugin found";String error;auto* d=devices.getCurrentAudioDevice();if(!d)return "Open Audio setup first";auto instance=pluginFormats.createPluginInstance(*found[0],rate.load(),d->getCurrentBufferSizeSamples(),error);if(!instance)return error.isEmpty()?String("Plugin load failed"):error; if(instance->getTotalNumInputChannels()!=2 || instance->getTotalNumOutputChannels()!=2)return "Only stereo 2-in/2-out effects are supported";devices.removeAudioCallback(this);masterPlugin=std::move(instance);devices.addAudioCallback(this);return {};}
 bool replaceStems(const Array<File>& files,bool reset){if(files.isEmpty()||files.size()>pulse::stemCount)return false;auto next=std::make_unique<pulse::StemSource>(stemTrims,stemMutes);for(const auto& f:files)if(!next->add(std::unique_ptr<AudioFormatReader>(formats.createReaderFor(f))))return false;transport.stop();transport.setSource(nullptr);if(reset){for(auto& t:stemTrims)t=1.f;for(auto& m:stemMutes)m=false;}reader=std::move(next);stemFiles=files;transport.setSource(reader.get(),16384,&stemDisk,reader->getRate());transport.setPosition(0);return true;}
 bool load(File f){Array<File> files;files.add(f);return replaceStems(files,true);}
 bool addStem(File f){if(stemFiles.size()>=pulse::stemCount)return false;auto files=stemFiles;files.add(f);int index=files.size()-1;stemTrims[index]=1.f;stemMutes[index]=false;return replaceStems(files,false);}
 bool removeStem(){if(stemFiles.size()<2)return false;auto files=stemFiles;files.removeLast();return replaceStems(files,false);}
 bool recordInputs(const File& f){auto* d=devices.getCurrentAudioDevice();if(!d||!d->isPlaying())return false;return multitrack.start(f,d->getCurrentSampleRate(),d->getActiveInputChannels().countNumberOfSetBits(),d->getCurrentBufferSizeSamples());}
 bool record(File f){stopRecord(); auto stream=f.createOutputStream(); if(!stream)return false; WavAudioFormat wav; auto* w=wav.createWriterFor(stream.get(),rate,2,24,{},0); if(!w)return false; stream.release(); auto next=std::make_unique<AudioFormatWriter::ThreadedWriter>(w,disk,65536); const ScopedLock lock(recordLock); writer=std::move(next); return true;}
 void stopRecord(){std::unique_ptr<AudioFormatWriter::ThreadedWriter> old; {const ScopedLock lock(recordLock); old=std::move(writer);} }
};
