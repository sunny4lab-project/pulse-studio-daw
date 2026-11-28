#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <algorithm>
#include "Dsp.h"
#include "CallbackDeadline.h"
#include "ToneDSP.h"
#include "StemSource.h"
#include "TimelineSource.h"
#include "MixIntelligence.h"
#include "OutputMix.h"
#include "InsertRack.h"
#include "MultitrackRecorder.h"
#include "TrackRecorder.h"
#include "MasterDSP.h"
#include "SendEffects.h"
#include "MixerTopology.h"
#include "ChannelColours.h"
#include <atomic>
#include <cmath>
using namespace juce;
constexpr int B=pulse::busCount;
static String busName(int b){switch(b){case 0:return "Drums";case 1:return "Instruments";case 2:return "Vocals";case 3:return "Other";default:return "Bus "+String(b+1);}}
struct Channel {pulse::MixSpectrum spectrum;std::array<std::atomic<float>,3> bandEnergy{{0.f,0.f,0.f}};std::atomic<float> inputPower{0}; std::atomic<float> gain{0.25f}, pan{0}, peak{0}, postPeak{0}, hp{0}, threshold{-18}, ratio{1}; std::atomic<int> bus{3}, source{-1}, rightSource{-1}; pulse::StripDSP dsp, rightDsp; pulse::ToneDSP toneLeft,toneRight; bool processingActive=false;float broadcastSmooth=0.f;std::atomic<float> broadcastGain{0.f},broadcastPan{0.f},broadcastPeak{0.f};std::atomic<bool> broadcastMute{false},broadcastLock{false};std::atomic<int> mixRole{1};std::atomic<std::uint32_t> customColour{0xffb6c8d9u};std::atomic<bool> automaticColour{true}; std::array<std::atomic<float>,pulse::eqBandCount> eqFreq{{120.f,1000.f,6500.f,60.f,250.f,500.f,2500.f,12000.f}},eqGain{{0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f}},eqQ{{.8f,1.f,.8f,1.f,1.f,1.f,1.f,1.f}};std::atomic<float> trimDb{0},lowPass{0},attackShape{0},saturation{0},limitDb{0},compAttack{5.3f},compRelease{79.6f};std::atomic<bool> polarity{false},channelLimit{false},recordArm{false};std::atomic<int> dca{-1};std::atomic<float> gateThreshold{-80.f},deessThreshold{-80.f};std::atomic<bool> toneBypass{false};
 pulse::ToneSettings toneSettings() const {pulse::ToneSettings t;for(int i=0;i<pulse::eqBandCount;++i){t.frequency[i]=eqFreq[i].load();t.gain[i]=eqGain[i].load();t.q[i]=eqQ[i].load();}t.gate=gateThreshold.load();t.deess=deessThreshold.load();t.bypass=toneBypass.load();t.lowPass=lowPass.load();t.attackShape=attackShape.load();t.saturation=saturation.load();t.limit=channelLimit.load();t.limitDb=limitDb.load();return t;}
  std::atomic<bool> mute{true}, solo{false}; };
class Engine : public AudioIODeviceCallback {
public:
 pulse::StableList<Channel> ch; std::atomic<float> master{0.5f}, peak{0}; std::atomic<bool> enabled{false};
 pulse::CallbackDeadline callbackDeadline;
 std::atomic<bool> broadcastDirectEnabled{false};
 std::atomic<float> broadcastLeftPeak{0},broadcastRightPeak{0},broadcastShortLufs{-100},broadcastIntegratedLufs{-100},broadcastTruePeakEstimate{0};
 pulse::LoudnessMeter broadcastLoudness; bool broadcastMeterActive=false;
 AudioDeviceManager devices; AudioFormatManager formats; AudioTransportSource transport; TimeSliceThread disk{"Recording disk"}, stemDisk{"Stem read ahead"};
 pulse::MultitrackRecorder multitrack{disk};pulse::TrackRecorder trackRecorder{disk};static constexpr int dcaCount=8;std::array<std::atomic<float>,dcaCount> dcaGain{};std::array<std::atomic<bool>,dcaCount> dcaMute{}; pulse::SendEffects sends; pulse::MasterLimiter limiter,broadcastLimiter; pulse::LoudnessMeter loudness;
 std::atomic<bool> limiterEnabled{false},resetLoudness{false};std::atomic<float> limiterCeiling{-1.f},limiterRelease{100.f},limiterReduction{0.f},momentaryLufs{-100.f},shortLufs{-100.f},integratedLufs{-100.f},estimatedTruePeak{0.f};
 std::unique_ptr<pulse::StemSource> reader;std::unique_ptr<pulse::TimelineSource> timeline,clipLoop;double loopReturnCursor=0;std::atomic<unsigned> arrangementMask{0};std::atomic<bool> analyzeMix{false};std::array<pulse::StereoAlignDelay,B> arrangementAlign;std::array<int,B> arrangementDelay{},arrangementLast{}; std::array<std::atomic<float>,pulse::stemCount> stemTrims{}; std::array<std::atomic<bool>,pulse::stemCount> stemMutes{}; Array<File> stemFiles; std::atomic<float> rms{0}; std::atomic<unsigned> clippedBlocks{0}; std::unique_ptr<AudioFormatWriter::ThreadedWriter> writer; CriticalSection recordLock; std::atomic<unsigned> missedWrites{0}; std::atomic<double> rate{48000}; AudioBuffer<float> playback; std::array<AudioBuffer<float>,B> buses; std::array<std::atomic<float>,B> busGain{}, busPeak{};
 std::array<std::atomic<float>,B*pulse::auxMixCount> auxSend{}; std::array<std::atomic<float>,pulse::auxMixCount> auxGain{},auxPeak{}; std::array<std::atomic<bool>,pulse::auxMixCount> auxArmed{},auxAvailable{};
 void disarmAux() noexcept {for(auto& a:auxArmed)a=false;}
 float getAuxSend(int bus,int mix) const noexcept {return auxSend[bus*pulse::auxMixCount+mix].load();}
 AudioPluginFormatManager pluginFormats; pulse::StableList<pulse::InsertRack> channelInserts;std::array<pulse::InsertRack,B> busInserts;pulse::InsertRack masterInserts;
 int channelCount()const{return ch.size();}
 int rackCount()const{return channelCount()+B;}
 pulse::InsertRack& rackAt(int i){return i==rackCount()?masterInserts:i<channelCount()?channelInserts[i]:busInserts[i-channelCount()];}
 const pulse::InsertRack& rackAt(int i)const{return i==rackCount()?masterInserts:i<channelCount()?channelInserts[i]:busInserts[i-channelCount()];}
 // Call only while idle, with UI references to removed channels released.
 bool addChannels(const std::vector<int>& sources){
  if(sources.empty()||!canEditInserts())return false;
  struct Pause{Engine& e;explicit Pause(Engine& x):e(x){if(e.started)e.devices.removeAudioCallback(&e);}~Pause(){if(e.started)e.devices.addAudioCallback(&e);}} pause(*this);
  const int old=channelCount();
  try{
   for(int source:sources){auto& c=ch.append();try{channelInserts.append();}catch(...){ch.erase(ch.size()-1);throw;}c.source=source;}
   return true;
  }catch(const std::bad_alloc&){while(ch.size()>old){channelInserts.erase(ch.size()-1);ch.erase(ch.size()-1);}return false;}
 }
 bool addChannel(){return addChannels(std::vector<int>{-1});}
 bool removeChannel(int i){if(i<0||i>=channelCount()||!canEditInserts())return false;if(started)devices.removeAudioCallback(this);channelInserts.erase(i);ch.erase(i);if(started)devices.addAudioCallback(this);return true;}
 std::atomic<bool> compensated{false},alignmentFault{false};std::atomic<int> alignmentSamples{0};
 std::atomic<int> masterPair{0};std::array<std::atomic<int>,pulse::auxMixCount> auxPair{{1,2,3}};
 std::vector<int> channelDelay,channelGroup;std::array<int,B> busDelay{};pulse::StereoAlignDelay playbackAlignment;int playbackDelay=-1;
 bool setOutputPair(int mix,int pair){
  if(!canEditInserts()||mix<0||mix>pulse::auxMixCount||pair<-1||pair>=pulse::maxOutputChannels/2||(mix==0&&pair<0))return false;
  auto* d=devices.getCurrentAudioDevice();if(pair>=0&&(!d||pair>=d->getActiveOutputChannels().countNumberOfSetBits()/2))return false;
  if(pair>=0){if(mix!=0&&masterPair.load()==pair)return false;for(int a=0;a<pulse::auxMixCount;++a)if(mix!=a+1&&auxPair[a].load()==pair)return false;}
  disarmAux();if(mix==0)masterPair=pair;else auxPair[mix-1]=pair;return true;
 }
 void prepareAlignment(){
  bool on=compensated.load();channelDelay.assign(on?channelCount():0,0);channelGroup.assign(on?channelCount():0,0);
  for(auto& rack:channelInserts)rack.prepareAlignment(on);for(auto& rack:busInserts)rack.prepareAlignment(on);
  for(auto& a:arrangementAlign)a.prepare(on?pulse::maxAlignmentLatency:0);arrangementLast.fill(-1);playbackAlignment.prepare(on?pulse::maxAlignmentLatency:0);playbackDelay=-1;alignmentSamples=0;alignmentFault=false;
 }
 bool setCompensated(bool on){
  if(!canEditInserts())return false;if(on==compensated.load())return true;
  if(started)devices.removeAudioCallback(this);compensated=on;prepareAlignment();if(started)devices.addAudioCallback(this);return true;
 }
 bool calculateAlignment(int ni,const float* const* in,unsigned routed=0){
  if(channelDelay.size()!=(size_t)channelCount()||channelGroup.size()!=channelDelay.size())return false;
  std::array<int,B> maxInput{},busLat{};std::array<bool,B> used{};int total=0;
  for(int c=0;c<channelCount();++c){int group=std::clamp(ch[c].bus.load(),0,B-1);channelGroup[c]=group;int src=ch[c].source.load();channelDelay[c]=-1;if(!in||src<0||src>=ni||!in[src])continue;
   int latency=channelInserts[c].latency(true);if(latency<0)return false;channelDelay[c]=latency;used[group]=true;maxInput[group]=std::max(maxInput[group],latency);
  }
  for(int b=0;b<B;++b){if(routed&(1u<<b))used[b]=true;arrangementDelay[b]=maxInput[b];busDelay[b]=0;if(!used[b])continue;busLat[b]=busInserts[b].latency(true);if(busLat[b]<0)return false;total=std::max(total,maxInput[b]+busLat[b]);}
  for(int c=0;c<channelCount();++c)if(channelDelay[c]>=0)channelDelay[c]=maxInput[channelGroup[c]]-channelDelay[c];
  for(int b=0;b<B;++b)if(used[b])busDelay[b]=total-maxInput[b]-busLat[b];
  alignmentSamples=total;return total<=pulse::maxAlignmentLatency;
 }
 std::unique_ptr<AudioPluginInstance>& masterPlugin=masterInserts.slots[0].plugin; AudioBuffer<float> pluginBuffer; MidiBuffer pluginMidi; std::atomic<bool>& pluginBypass=masterInserts.slots[0].bypass; String deviceError; bool started=false;
 std::atomic<bool> deviceFault{false},deviceFaultNotice{false};std::atomic<unsigned> deviceStopSerial{0};
 Engine(){for(auto& v:dcaGain)v=1.f;for(auto& v:dcaMute)v=false;for(auto& v:auxSend)v=0.f;for(auto& v:auxGain)v=0.25f;for(auto& v:auxPeak)v=0.f;for(auto& a:auxArmed)a=false;for(auto& a:auxAvailable)a=false;for(auto& t:stemTrims)t=1.f;for(auto& m:stemMutes)m=false;juce::addDefaultFormatsToManager(pluginFormats);for(auto& g:busGain)g.store(1.f);for(auto& p:busPeak)p.store(0.f);formats.registerBasicFormats(); disk.startThread();stemDisk.startThread();}
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
  enabled=false;disarmAux();if(saved)deviceError=devices.initialise(2,2,saved.get(),false);devices.addAudioCallback(this);
 }
 // Open exactly the selected endpoints without first opening a default device.
 String openSelectedDevice(const String& type,const String& input,const String& output,double sampleRate,int bufferSize,int inputCount,int outputCount){
  if(!canEditInserts())return "Stop live monitoring, playback and recording first.";
  if(output.isEmpty()||outputCount<2||outputCount>pulse::maxOutputChannels||inputCount<0||inputCount>4096||sampleRate<0||bufferSize<0)return "Select an output device and valid channel counts.";
  bool found=false;
  for(auto* t:devices.getAvailableDeviceTypes())if(t->getTypeName()==type){
   if(!t->getDeviceNames(false).contains(output)||(input.isNotEmpty()&&!t->getDeviceNames(true).contains(input)))return "The selected device is no longer listed. Refresh devices.";
   found=true;break;
  }
  if(!found)return "Selected audio backend is unavailable.";
  XmlElement xml("DEVICESETUP");xml.setAttribute("deviceType",type);xml.setAttribute("audioInputDeviceName",input);xml.setAttribute("audioOutputDeviceName",output);
  if(sampleRate>0)xml.setAttribute("audioDeviceRate",sampleRate);
  if(bufferSize>0)xml.setAttribute("audioDeviceBufferSize",bufferSize);
  BigInteger ins,outs;if(input.isNotEmpty()&&inputCount>0)ins.setRange(0,inputCount,true);outs.setRange(0,outputCount,true);
  xml.setAttribute("audioDeviceInChans",ins.toString(2));xml.setAttribute("audioDeviceOutChans",outs.toString(2));
  enabled=false;disarmAux();devices.closeAudioDevice();deviceFault=false;deviceFaultNotice=false;
  auto error=devices.initialise(2,2,&xml,false);
  auto* d=devices.getCurrentAudioDevice();
  if(error.isEmpty()&&(!d||!d->isOpen()||!d->isPlaying()))error="The driver did not enter a running state.";
  if(error.isEmpty()&&d->getActiveOutputChannels().countNumberOfSetBits()<2)error="The device did not enable a stereo output pair.";
  if(error.isNotEmpty()){devices.closeAudioDevice();deviceError=type+" | "+output+" | "+String(sampleRate,0)+" Hz | "+String(bufferSize)+" samples | requested "+String(inputCount)+" in / "+String(outputCount)+" out: "+error;return deviceError;}
  deviceError.clear();return {};
 }
 StringArray activeChannelNames(bool input){
  StringArray result;auto* d=devices.getCurrentAudioDevice();if(!d)return result;
  auto names=input?d->getInputChannelNames():d->getOutputChannelNames();
  auto mask=input?d->getActiveInputChannels():d->getActiveOutputChannels();
  for(int i=mask.findNextSetBit(0);i>=0;i=mask.findNextSetBit(i+1))
   result.add(String(i+1)+" | "+(i<names.size()?names[i]:String(input?"Input":"Output")));
  return result;
 }
 // Read-only inventory of the backends registered by JUCE. Scanning is
 // explicit and idle-only, because some vendor drivers can block on enumeration.
 StringArray discoverAudioDevices(bool rescan=true){
  StringArray result;
  if(rescan&&!canEditInserts())return result;
  for(auto* type:devices.getAvailableDeviceTypes()){
   if(rescan)type->scanForDevices();
   result.add("["+type->getTypeName()+"]");
   auto ins=type->getDeviceNames(true),outs=type->getDeviceNames(false);
   if(ins.isEmpty()&&outs.isEmpty()){result.add("  No devices reported");continue;}
   if(!type->hasSeparateInputsAndOutputs()){
    StringArray combined=outs;combined.addArray(ins);combined.removeDuplicates(false);
    for(auto& name:combined)result.add("  I/O: "+name);
   }else{
    for(auto& name:ins)result.add("  Input: "+name);
    for(auto& name:outs)result.add("  Output: "+name);
   }
  }
  return result;
 }
 StringArray registeredAudioBackends(){
  StringArray names;for(auto* type:devices.getAvailableDeviceTypes())names.add(type->getTypeName());return names;
 }
 ~Engine() override {devices.removeAudioCallback(this); trackRecorder.stop();multitrack.stop();stopRecord(); transport.setSource(nullptr); stemDisk.stopThread(2000);disk.stopThread(2000);}
 void audioDeviceAboutToStart(AudioIODevice* d) override {callbackDeadline.reset();enabled=false;deviceFault=false;disarmAux();for(auto& a:auxAvailable)a=false;rate=d->getCurrentSampleRate();multitrack.invalidate();trackRecorder.invalidate();sends.prepare(rate.load(),d->getCurrentBufferSizeSamples());limiter.reset();broadcastLimiter.reset();broadcastLoudness.reset(rate.load());broadcastMeterActive=false;broadcastLeftPeak=0;broadcastRightPeak=0;broadcastShortLufs=-100;broadcastIntegratedLufs=-100;broadcastTruePeakEstimate=0;loudness.reset(rate.load());for(auto& c:ch){c.broadcastSmooth=0.f;c.broadcastPeak=0.f;c.dsp.reset();c.rightDsp.reset();c.toneLeft.reset();c.toneRight.reset();c.spectrum.reset();for(auto& a:c.bandEnergy)a=0;c.inputPower=0;} playback.setSize(2*(B+1),d->getCurrentBufferSizeSamples());pluginBuffer.setSize(2,d->getCurrentBufferSizeSamples());masterInserts.prepare(rate,d->getCurrentBufferSizeSamples());for(auto& rack:channelInserts)rack.prepare(rate,d->getCurrentBufferSizeSamples());for(auto& rack:busInserts)rack.prepare(rate,d->getCurrentBufferSizeSamples());pluginMidi.ensureSize(2048); for(auto& b:buses)b.setSize(2,d->getCurrentBufferSizeSamples());transport.prepareToPlay(d->getCurrentBufferSizeSamples(),rate);prepareAlignment();}
 void audioDeviceError(const String&) override {trackRecorder.invalidate();multitrack.invalidate();deviceFault=true;deviceFaultNotice=true;enabled=false;disarmAux();peak=0;rms=0;}
 void audioDeviceStopped() override {++deviceStopSerial;enabled=false;multitrack.invalidate();trackRecorder.invalidate();limiterReduction=0;momentaryLufs=-100;shortLufs=-100;integratedLufs=-100;estimatedTruePeak=0;disarmAux();for(auto& a:auxAvailable)a=false;for(auto& p:auxPeak)p=0;peak=0;rms=0;transport.releaseResources();masterInserts.release();for(auto& rack:channelInserts)rack.release();for(auto& rack:busInserts)rack.release();}
 // Bypass costly per-sample DSP and plugin processing for silent strips, while
 // preserving their pre-fader hardware input meter and optional spectrum data.
 // This runs exclusively on the audio callback thread and never allocates.
 bool skipSilentChannel(Channel& s,int index,const float* x,const float* xr,int n,bool solo,bool pdc,bool broadcastActive){
  const int dca=s.dca.load();
  const float dcaLevel=dca>=0&&dca<dcaCount?(dcaMute[dca].load()?0.f:dcaGain[dca].load()):1.f;
  bool mainAudible=enabled.load()&&!s.mute.load()&&(!solo||s.solo.load())&&s.gain.load()*dcaLevel>0.f;
  bool broadcastAudible=broadcastActive&&!s.broadcastMute.load()&&s.broadcastGain.load()>0.f;
  if(x&&(mainAudible||broadcastAudible)){s.processingActive=true;return false;}
  if(s.processingActive){s.dsp.reset();s.rightDsp.reset();s.toneLeft.reset();s.toneRight.reset();s.processingActive=false;}
  s.broadcastSmooth=0.f;s.broadcastPeak=0.f;
  if(pdc)channelInserts[index].clearAlignment();
  if(x&&analyzeMix.load()){s.spectrum.analyze(x,xr,n,rate.load());for(int j=0;j<3;++j)s.bandEnergy[j]=s.spectrum.energy[j];s.inputPower=s.spectrum.rms;}
  float inputPeak=0;if(x)for(int i=0;i<n;++i){inputPeak=std::max(inputPeak,std::abs(x[i]));if(xr)inputPeak=std::max(inputPeak,std::abs(xr[i]));}
  s.peak=inputPeak;s.postPeak=0;return true;
 }
 void audioDeviceIOCallbackWithContext(const float* const* in,int ni,float* const* out,int no,int n,const AudioIODeviceCallbackContext&) override {
  ScopedNoDenormals noDenormals;
  pulse::CallbackDeadline::Scope callbackTiming(callbackDeadline,n,rate.load());
  if(n<=0)return;if(deviceFault.load()){if(out)for(int c=0;c<no;++c)if(out[c])FloatVectorOperations::clear(out[c],n);return;}multitrack.push(in,ni,n);trackRecorder.push(in,ni,n);if(!out)return;
  for(int c=0;c<no;++c) if(out[c]) FloatVectorOperations::clear(out[c],n);
  int mPair=masterPair.load();std::array<int,pulse::auxMixCount> pair{};std::array<bool,pulse::auxMixCount> auxActive{};
  for(int a=0;a<pulse::auxMixCount;++a){pair[a]=auxPair[a].load();bool available=pair[a]!=mPair&&pulse::outputPairAvailable(out,no,pair[a]);for(int j=0;j<a;++j)if(pair[j]==pair[a])available=false;auxAvailable[a]=available;auxActive[a]=available&&auxArmed[a].load()&&enabled.load();auxPeak[a]=0;}
  bool broadcastActive=auxActive[0]&&broadcastDirectEnabled.load();
  if(!broadcastActive){broadcastLeftPeak=0;broadcastRightPeak=0;broadcastShortLufs=-100;broadcastIntegratedLufs=-100;broadcastTruePeakEstimate=0;broadcastMeterActive=false;}
  float* broadcastL=broadcastActive?out[2*pair[0]]:nullptr;
  float* broadcastR=broadcastActive?out[2*pair[0]+1]:nullptr;
  if(!pulse::outputPairAvailable(out,no,mPair)||n<=0||n>playback.getNumSamples()) {peak=0;rms=0;return;}
  float* mixOut[]={out[2*mPair],out[2*mPair+1]};
  unsigned routed=playback.getNumChannels()>=2*(B+1)?arrangementMask.load():0;bool pdc=compensated.load();if(pdc&&!calculateAlignment(ni,in,routed)){alignmentFault=true;alignmentSamples=0;enabled=false;disarmAux();peak=0;rms=0;return;}alignmentFault=false;
  sends.begin(n);std::array<bool,B> used{}; bool solo=false; for(auto& c:ch) solo|=c.solo.load();
  for(int c=0;c<channelCount();++c){auto& s=ch[c]; float p=0, post=0; int src=s.source.load(), group=pdc?channelGroup[c]:std::clamp(s.bus.load(),0,B-1); if(in && src>=0 && src<ni && in[src]) {int rs=s.rightSource.load();const float* xr=rs>=0&&rs<ni?in[rs]:nullptr;if(skipSilentChannel(s,c,in[src],xr,n,solo,pdc,broadcastActive))continue;if(!used[group]){buses[group].clear(0,n);used[group]=true;}auto* bl=buses[group].getWritePointer(0);auto* br=buses[group].getWritePointer(1);auto* x=in[src];bool stereo=rs>=0;if(analyzeMix.load()){s.spectrum.analyze(x,xr,n,rate.load());for(int j=0;j<3;++j)s.bandEnergy[j]=s.spectrum.energy[j];s.inputPower=s.spectrum.rms;}for(int i=0;i<n;++i){p=std::max(p,std::abs(x[i]));if(xr)p=std::max(p,std::abs(xr[i]));} {int dca=s.dca.load();float dg=dca>=0&&dca<dcaCount?(dcaMute[dca].load()?0.f:dcaGain[dca].load()):1.f;float g=(enabled && !s.mute && (!solo || s.solo))?s.gain.load()*dg:0.f, a=s.pan;float trim=pulse::dbGain(std::clamp(s.trimDb.load(),-24.f,24.f))*(s.polarity.load()?-1.f:1.f); float l=std::sqrt((1-a)*0.5f),r=std::sqrt((1+a)*0.5f); float hp=pulse::coefficient(s.hp.load(),rate), th=pulse::dbGain(s.threshold), ratio=s.ratio, attack=1.f-std::exp(-1000.f/(std::clamp(s.compAttack.load(),.1f,100.f)*(float)rate.load())), release=1.f-std::exp(-1000.f/(std::clamp(s.compRelease.load(),10.f,1000.f)*(float)rate.load())), smooth=pulse::coefficient(100,rate); bool insertActive=(g>0.f&&channelInserts[c].active(pdc))||(pdc&&channelDelay[c]>0);if(!insertActive&&pdc)channelInserts[c].clearAlignment();auto* il=insertActive?pluginBuffer.getWritePointer(0):nullptr;auto* ir=insertActive?pluginBuffer.getWritePointer(1):nullptr;float broadcastTarget=broadcastActive&&!s.broadcastMute.load()?std::clamp(s.broadcastGain.load(),0.f,1.5f):0.f;float broadcastPan=std::clamp(s.broadcastPan.load(),-1.f,1.f);float broadcastP=0.f;if(!broadcastActive)s.broadcastSmooth=0.f;auto tone=s.toneSettings();s.toneLeft.prepare(tone,rate.load());if(stereo)s.toneRight.prepare(tone,rate.load());for(int i=0;i<n;++i){float y=s.dsp.process(s.toneLeft.process(x[i]*trim),g,hp,th,ratio,attack,release,smooth);if(broadcastActive)s.broadcastSmooth+=smooth*(broadcastTarget-s.broadcastSmooth);post=std::max(post,std::abs(y));if(stereo){float yr=s.rightDsp.process(s.toneRight.process(xr?xr[i]*trim:0.f),g,hp,th,ratio,attack,release,smooth);post=std::max(post,std::abs(yr));if(broadcastActive){float lb=s.dsp.lastPreGain*s.broadcastSmooth*std::min(1.f,1.f-broadcastPan),rb=s.rightDsp.lastPreGain*s.broadcastSmooth*std::min(1.f,1.f+broadcastPan);broadcastL[i]+=lb;broadcastR[i]+=rb;broadcastP=std::max(broadcastP,std::max(std::abs(lb),std::abs(rb)));}float left=y*std::min(1.f,1-a),right=yr*std::min(1.f,1+a);if(insertActive){il[i]=left;ir[i]=right;}else{bl[i]+=left;br[i]+=right;}}else{if(broadcastActive){float v=s.dsp.lastPreGain*s.broadcastSmooth;float lb=v*std::sqrt((1.f-broadcastPan)*.5f),rb=v*std::sqrt((1.f+broadcastPan)*.5f);broadcastL[i]+=lb;broadcastR[i]+=rb;broadcastP=std::max(broadcastP,std::max(std::abs(lb),std::abs(rb)));}if(insertActive){il[i]=y*l;ir[i]=y*r;}else{bl[i]+=y*l;br[i]+=y*r;}}}if(insertActive){if(g>0.f)channelInserts[c].process(il,ir,n,pluginMidi,pdc);if(pdc){if(g>0.f)channelInserts[c].align(il,ir,n,std::max(0,channelDelay[c]));else{channelInserts[c].clearAlignment();FloatVectorOperations::clear(il,n);FloatVectorOperations::clear(ir,n);}}post=0;for(int i=0;i<n;++i){post=std::max(post,std::max(std::abs(il[i]),std::abs(ir[i])));bl[i]+=il[i];br[i]+=ir[i];}}s.broadcastPeak=broadcastP;}}else {skipSilentChannel(s,c,nullptr,nullptr,n,solo,pdc,broadcastActive);} s.peak=p;s.postPeak=post;}
  playback.clear();AudioSourceChannelInfo info(&playback,0,n);transport.getNextAudioBlock(info);
  for(int b=0;b<B;++b)if(routed&(1u<<b)){auto* l=playback.getWritePointer(2*(b+1));auto* r=playback.getWritePointer(2*(b+1)+1);if(pdc){int delay=arrangementDelay[b];if(arrangementLast[b]!=delay){arrangementAlign[b].configure(delay);arrangementLast[b]=delay;}arrangementAlign[b].process(l,r,n);}if(!used[b]){buses[b].clear(0,n);used[b]=true;}buses[b].addFrom(0,0,l,n);buses[b].addFrom(1,0,r,n);}
  for(int b=0;b<B;++b){if(!used[b]){busPeak[b]=0;if(pdc)busInserts[b].clearAlignment();continue;}bool busActive=enabled.load()||(routed&(1u<<b));if(busActive)busInserts[b].process(buses[b].getWritePointer(0),buses[b].getWritePointer(1),n,pluginMidi,pdc);if(pdc){if(busActive)busInserts[b].align(buses[b].getWritePointer(0),buses[b].getWritePointer(1),n,busDelay[b]);else{busInserts[b].clearAlignment();buses[b].clear(0,n);}}float g=busGain[b].load(),p=0;std::array<float,pulse::auxMixCount> send{};for(int a=0;a<pulse::auxMixCount;++a)if(auxActive[a])send[a]=getAuxSend(b,a);for(int k=0;k<2;++k){auto* x=buses[b].getReadPointer(k);for(int i=0;i<n;++i){float y=pulse::outputMixSample(x[i],g);p=std::max(p,std::abs(y));mixOut[k][i]+=y;}sends.send(b,k,x,g,n);for(int a=0;a<pulse::auxMixCount;++a)if(send[a]>0.f&&!(a==0&&broadcastActive))FloatVectorOperations::addWithMultiply(out[2*pair[a]+k],x,g*send[a],n);}busPeak[b]=p;}
  if(broadcastActive)broadcastLimiter.process(broadcastL,broadcastR,n,rate.load(),-1.f,100.f,true);
  for(int a=0;a<pulse::auxMixCount;++a)if(auxActive[a]){float p=0,g=auxGain[a].load();for(int k=0;k<2;++k){auto* x=out[2*pair[a]+k];for(int i=0;i<n;++i){float y=pulse::outputMixSample(x[i],g);p=std::max(p,std::abs(y));x[i]=pulse::safeOutputSample(y);}}auxPeak[a]=p;}
  if(broadcastActive){if(!broadcastMeterActive){broadcastLoudness.reset(rate.load());broadcastMeterActive=true;}broadcastLeftPeak=FloatVectorOperations::findMaximum(broadcastL,n);broadcastRightPeak=FloatVectorOperations::findMaximum(broadcastR,n);broadcastLeftPeak=std::max(broadcastLeftPeak.load(),-FloatVectorOperations::findMinimum(broadcastL,n));broadcastRightPeak=std::max(broadcastRightPeak.load(),-FloatVectorOperations::findMinimum(broadcastR,n));broadcastLoudness.process(broadcastL,broadcastR,n);broadcastShortLufs=broadcastLoudness.getShortTerm();broadcastIntegratedLufs=broadcastLoudness.getIntegrated();broadcastTruePeakEstimate=broadcastLoudness.getHeldPeak();}
  sends.render(mixOut[0],mixOut[1],n);
  if(pdc){int delay=alignmentSamples.load();if(delay!=playbackDelay){playbackAlignment.configure(delay);playbackDelay=delay;}playbackAlignment.process(playback.getWritePointer(0),playback.getWritePointer(1),n);}
  for(int c=0;c<2;++c)FloatVectorOperations::add(mixOut[c],playback.getReadPointer(c),n);masterInserts.process(mixOut[0],mixOut[1],n,pluginMidi,true);float p=0;double squares=0;float masterGain=master.load();for(int c=0;c<2;++c)FloatVectorOperations::multiply(mixOut[c],masterGain,n);
  limiter.process(mixOut[0],mixOut[1],n,rate.load(),limiterCeiling.load(),limiterRelease.load(),limiterEnabled.load());limiterReduction=limiter.reduction;
  for(int c=0;c<2;++c){for(int i=0;i<n;++i){p=std::max(p,std::abs(mixOut[c][i]));mixOut[c][i]=pulse::safeOutputSample(mixOut[c][i]);squares+=mixOut[c][i]*mixOut[c][i];}}peak=p;
  if(resetLoudness.exchange(false))loudness.reset(rate.load());loudness.process(mixOut[0],mixOut[1],n);momentaryLufs=loudness.getMomentary();shortLufs=loudness.getShortTerm();integratedLufs=loudness.getIntegrated();estimatedTruePeak=loudness.getHeldPeak();rms=n>0?(float)std::sqrt(squares/(2.0*n)):0.f;if(p>1.f)++clippedBlocks;
  const ScopedTryLock lock(recordLock); if(lock.isLocked() && writer && !writer->write(mixOut,n)) ++missedWrites;
 }
 bool canEditInserts(){const ScopedLock lock(recordLock);return !enabled.load()&&!transport.isPlaying()&&!writer&&!multitrack.active()&&!trackRecorder.active();}
 // Offline session setup does not open or arm any physical device. Plugins
 // are re-prepared at the actual device rate/block in audioDeviceAboutToStart.
 struct PluginPreparation {double sampleRate=48000.0;int blockSize=256;bool connected=false;};
 PluginPreparation pluginPreparation()const{
  if(auto* d=devices.getCurrentAudioDevice();d&&d->isPlaying()&&d->getCurrentSampleRate()>0&&d->getCurrentBufferSizeSamples()>0)
   return {d->getCurrentSampleRate(),d->getCurrentBufferSizeSamples(),true};
  return {};
 }
 String changeInsertDescription(int rack,int slot,const PluginDescription* desc){
  if(rack<0||rack>rackCount()||slot<0||slot>=pulse::insertsPerRack)return "Invalid insert slot";
  if(!canEditInserts())return "Stop playback and recording and disable live inputs first.";
  const auto prep=pluginPreparation();
  std::unique_ptr<AudioPluginInstance> instance;
  if(desc){
   if(desc->isInstrument)return "Virtual instruments are not yet supported.";
   String error;instance=pluginFormats.createPluginInstance(*desc,prep.sampleRate,prep.blockSize,error);
   if(!instance)return error.isEmpty()?String("Plugin load failed"):error;
   if(instance->getTotalNumInputChannels()!=2||instance->getTotalNumOutputChannels()!=2)return "Only stereo 2-in/2-out effects are supported";
   instance->prepareToPlay(prep.sampleRate,prep.blockSize);int latency=instance->getLatencySamples();instance->releaseResources();
   if(rack!=rackCount()){int other=0;auto& rackRef=rackAt(rack);for(int i=0;i<pulse::insertsPerRack;++i)if(i!=slot&&rackRef.slots[i].plugin){int l=rackRef.slots[i].plugin->getLatencySamples();if(l<0||l>pulse::maxRackLatency-other)return "Existing rack latency exceeds budget.";other+=l;}
   if(latency<0||latency>pulse::maxRackLatency-other)return "Effect chain latency exceeds the 4096-sample per-rack budget.";}
  }
  if(!canEditInserts())return "Transport or live state changed. Stop audio first.";
  if(started)devices.removeAudioCallback(this);auto& target=rackAt(rack).slots[slot];target.plugin=std::move(instance);target.bypass=false;rackAt(rack).visibleSlots=std::max(rackAt(rack).visibleSlots,slot+1);if(started)devices.addAudioCallback(this);return {};
 }
 String changeInsert(int rack,int slot,const File& file){
  if(file==File{})return changeInsertDescription(rack,slot,nullptr);
  if(!canEditInserts())return "Stop audio before scanning effects.";
  OwnedArray<PluginDescription> found;for(auto* format:pluginFormats.getFormats())if(format->fileMightContainThisPluginType(file.getFullPathName()))format->findAllTypesForFile(found,file.getFullPathName());
  if(found.isEmpty())return "No compatible plugin found";if(found.size()!=1)return "This file exposes multiple plugins. Choose the individual effect in the plugin browser.";
  return changeInsertDescription(rack,slot,found[0]);
 }
 String loadPluginDescription(const PluginDescription& desc){return changeInsertDescription(rackCount(),0,&desc);}
 String loadPlugin(const File& file){
  if(!canEditInserts())return "Stop audio before scanning effects.";
  OwnedArray<PluginDescription> found;for(auto* format:pluginFormats.getFormats())if(format->fileMightContainThisPluginType(file.getFullPathName()))format->findAllTypesForFile(found,file.getFullPathName());
  if(found.isEmpty())return "No compatible plugin found";if(found.size()!=1)return "This file exposes multiple plugins. Choose the individual effect in the plugin browser.";
  return loadPluginDescription(*found[0]);
 }
 pulse::TimelineSession timelineSession(){if(timeline)return timeline->session();pulse::TimelineSession session;for(int i=0;i<stemFiles.size();++i){std::unique_ptr<AudioFormatReader> r(formats.createReaderFor(stemFiles[i]));if(!r)continue;pulse::TimelineTrack t;t.name=stemFiles[i].getFileNameWithoutExtension();t.gain=stemTrims[i].load()/std::max(1,stemFiles.size());t.mute=stemMutes[i].load();pulse::AudioClip clip;clip.file=stemFiles[i];clip.length=pulse::TimelineSession::duration(*r);t.clips.push_back(clip);session.tracks.push_back(std::move(t));}return session;}
 String startClipLoop(int track,int clip){if(!canEditInserts()||clipLoop||!timeline)return "Stop audio and commit an arrangement first.";auto& model=timeline->session();if(track<0||track>=(int)model.tracks.size()||clip<0||clip>=(int)model.tracks[track].clips.size())return "Select a clip.";pulse::TimelineSession preview;pulse::TimelineTrack t=model.tracks[track];auto c=t.clips[clip];t.clips.clear();t.notes.clear();t.gainAutomation.clear();t.panAutomation.clear();t.mute=false;t.solo=false;c.start=0;t.clips.push_back(c);preview.tracks.push_back(std::move(t));String error;auto source=pulse::TimelineSource::create(formats,std::move(preview),error);if(!source)return error;source->enableBusOutputs();source->enableLoop();loopReturnCursor=transport.getCurrentPosition();transport.stop();transport.setSource(nullptr);clipLoop=std::move(source);transport.setSource(clipLoop.get(),16384,&stemDisk,pulse::timelineRate,clipLoop->busMask()?2*(B+1):2);arrangementMask=clipLoop->busMask();transport.start();return {};}
 void stopClipLoop(){if(!clipLoop)return;transport.stop();transport.setSource(nullptr);clipLoop.reset();arrangementMask=0;if(timeline&&timeline->getTotalLength()>0){transport.setSource(timeline.get(),16384,&stemDisk,pulse::timelineRate,timeline->busMask()?2*(B+1):2);arrangementMask=timeline->busMask();transport.setPosition(jlimit(0.0,transport.getLengthInSeconds(),loopReturnCursor));}}
 void applyTimeline(std::unique_ptr<pulse::TimelineSource> next,double cursor){transport.stop();transport.setSource(nullptr);clipLoop.reset();reader.reset();stemFiles.clear();timeline=std::move(next);arrangementMask=0;if(timeline&&timeline->getTotalLength()>0){timeline->enableBusOutputs();transport.setSource(timeline.get(),16384,&stemDisk,pulse::timelineRate,timeline->busMask()?2*(B+1):2);arrangementMask=timeline->busMask();transport.setPosition(jlimit(0.0,transport.getLengthInSeconds(),cursor));}}
 String setTimeline(pulse::TimelineSession model,double cursor){if(!canEditInserts())return "Stop playback, live audio and recording before editing clips.";String error;auto next=pulse::TimelineSource::create(formats,std::move(model),error);if(!next)return error;applyTimeline(std::move(next),cursor);return {};}
 bool replaceStems(const Array<File>& files,bool reset){if(files.isEmpty()||files.size()>pulse::stemCount)return false;auto next=std::make_unique<pulse::StemSource>(stemTrims,stemMutes);for(const auto& f:files)if(!next->add(std::unique_ptr<AudioFormatReader>(formats.createReaderFor(f))))return false;transport.stop();transport.setSource(nullptr);clipLoop.reset();timeline.reset();arrangementMask=0;if(reset){for(auto& t:stemTrims)t=1.f;for(auto& m:stemMutes)m=false;}reader=std::move(next);stemFiles=files;transport.setSource(reader.get(),16384,&stemDisk,reader->getRate());transport.setPosition(0);return true;}
 bool load(File f){Array<File> files;files.add(f);return replaceStems(files,true);}
 bool addStem(File f){if(stemFiles.size()>=pulse::stemCount)return false;auto files=stemFiles;files.add(f);int index=files.size()-1;stemTrims[index]=1.f;stemMutes[index]=false;return replaceStems(files,false);}
 bool removeStem(){if(stemFiles.size()<2)return false;auto files=stemFiles;files.removeLast();return replaceStems(files,false);}
 bool recordInputs(const File& f){if(trackRecorder.active())return false;auto* d=devices.getCurrentAudioDevice();if(!d||!d->isPlaying())return false;return multitrack.start(f,d->getCurrentSampleRate(),d->getActiveInputChannels().countNumberOfSetBits(),d->getCurrentBufferSizeSamples());}
 bool record(File f){stopRecord(); auto stream=f.createOutputStream(); if(!stream)return false; WavAudioFormat wav; auto* w=wav.createWriterFor(stream.get(),rate,2,24,{},0); if(!w)return false; stream.release(); auto next=std::make_unique<AudioFormatWriter::ThreadedWriter>(w,disk,65536); const ScopedLock lock(recordLock); writer=std::move(next); return true;}
 void stopRecord(){std::unique_ptr<AudioFormatWriter::ThreadedWriter> old; {const ScopedLock lock(recordLock); old=std::move(writer);} }
};
