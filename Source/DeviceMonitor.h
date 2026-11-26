#pragma once
#include "AudioEngine.h"
namespace pulse {
// Message-thread observer. Driver enumeration and opening devices never occur
// on the audio callback. OS notification callbacks only set an atomic flag.
class DeviceMonitor final : private juce::AudioIODeviceType::Listener {
 Engine& e;
 std::vector<juce::AudioIODeviceType*> watched;
 std::atomic<bool> dirty{true};
 juce::StringArray inventory;
 juce::String preferredType,notice{"Automatic device discovery is ready."};
 juce::AudioDeviceManager::AudioDeviceSetup preferred;
 bool havePreferred=false,pending=false,review=false,configured=false,observed=false;
 bool automatic=true;
 juce::uint32 lastScan=0,lastAttempt=0;
 unsigned lastStop=0;
 void audioDeviceListChanged() override {dirty.store(true);}
 void subscribe(){
  for(auto* t:e.devices.getAvailableDeviceTypes())if(std::find(watched.begin(),watched.end(),t)==watched.end()){t->addListener(this);watched.push_back(t);}
 }
 bool active()const{auto* d=e.devices.getCurrentAudioDevice();return d&&d->isPlaying()&&!e.deviceFault.load();}
 bool exactDevicePresent(){
  for(auto* t:watched)if(t->getTypeName()==preferredType){
   auto ins=t->getDeviceNames(true),outs=t->getDeviceNames(false);
   return (preferred.inputDeviceName.isEmpty()||ins.contains(preferred.inputDeviceName))
       && (preferred.outputDeviceName.isEmpty()||outs.contains(preferred.outputDeviceName))
       && (preferred.inputDeviceName.isNotEmpty()||preferred.outputDeviceName.isNotEmpty());
  }return false;
 }
 bool sameEndpoints()const{
  auto now=e.devices.getAudioDeviceSetup();
  return e.devices.getCurrentAudioDeviceType()==preferredType&&now.inputDeviceName==preferred.inputDeviceName&&now.outputDeviceName==preferred.outputDeviceName;
 }
 bool sameRouting()const{
  auto now=e.devices.getAudioDeviceSetup();
  return sameEndpoints()&&now.inputChannels==preferred.inputChannels&&now.outputChannels==preferred.outputChannels;
 }
 void remember(){
  preferredType=e.devices.getCurrentAudioDeviceType();preferred=e.devices.getAudioDeviceSetup();
  preferred.useDefaultInputChannels=false;preferred.useDefaultOutputChannels=false;havePreferred=true;
 }
 void disarm(){
  e.enabled=false;e.disarmAux();e.transport.stop();e.stopRecord();e.multitrack.stop();
 }
 void reconnect(){
  lastAttempt=juce::Time::getMillisecondCounter();
  if(!havePreferred||!exactDevicePresent()||!e.canEditInserts())return;
  juce::XmlElement xml("DEVICESETUP");
  xml.setAttribute("deviceType",preferredType);xml.setAttribute("audioInputDeviceName",preferred.inputDeviceName);xml.setAttribute("audioOutputDeviceName",preferred.outputDeviceName);
  xml.setAttribute("audioDeviceRate",preferred.sampleRate);xml.setAttribute("audioDeviceBufferSize",preferred.bufferSize);
  xml.setAttribute("audioDeviceInChans",preferred.inputChannels.toString(2));xml.setAttribute("audioDeviceOutChans",preferred.outputChannels.toString(2));
  disarm();e.devices.closeAudioDevice();
  auto error=e.devices.initialise(0,2,&xml,false);
  e.deviceError=error;
  if(error.isEmpty()&&active()&&sameRouting()){
   pending=false;review=false;remember();notice="Previous interface reconnected. Verify routing and manually enable LIVE.";
  }else{
   review=true;notice=error.isEmpty()?juce::String("Interface reopened with different channels. Configure and verify routing."):juce::String("Reconnect failed: ")+error;
   if(active())pending=false;
  }
  dirty=true;
 }
public:
 explicit DeviceMonitor(Engine& engine):e(engine){}
 ~DeviceMonitor() override {for(auto* t:watched)t->removeListener(this);}
 const juce::StringArray& devices()const{return inventory;}
 juce::String status()const{return notice;}
 bool requiresReview()const{return review;}
 bool autoEnabled()const{return automatic;}
 void setAutoEnabled(bool enabled){automatic=enabled;dirty=true;}
 bool scanNow(){
  if(!e.canEditInserts())return false;
  subscribe();dirty=false;inventory=e.discoverAudioDevices(true);lastScan=juce::Time::getMillisecondCounter();
  return true;
 }
 void tick(bool configOpen){
  const auto now=juce::Time::getMillisecondCounter();
  const bool ready=active();auto stopped=e.deviceStopSerial.load();
  bool lostEvent=e.deviceFaultNotice.exchange(false);
  if(observed&&stopped!=lastStop){disarm();if(!configOpen)notice="Audio engine stopped or restarted. Live monitoring remains off.";}
  observed=true;lastStop=stopped;
  if(configOpen){
   configured=true;pending=false;review=true;
  }else if(configured){
   configured=false;pending=false;review=!ready;havePreferred=false;
   if(ready){remember();notice="Device configured. Verify routing before enabling LIVE.";}else notice="No active device. Configure an interface.";
   dirty=true;
  }else if(pending&&ready&&sameRouting()&&!lostEvent){pending=false;review=false;remember();notice="Previous interface available again. LIVE remains off.";
  }else if(havePreferred&&(lostEvent||!ready||!sameRouting())){
   if(!pending){disarm();pending=true;review=true;dirty=true;notice="Previous interface lost or routing changed. Live monitoring disabled.";}
   if(ready&&sameRouting()&&!lostEvent){pending=false;review=false;remember();notice="Previous interface available again. LIVE remains off.";}
  }else if(ready&&!havePreferred){remember();review=false;notice="Audio device active. Verify routing before enabling LIVE.";}
  if(lostEvent){disarm();review=true;if(havePreferred)pending=true;}
  // Never probe vendor drivers while processing a live mix or recording.
  if(!configOpen&&e.canEditInserts()&&(watched.empty()||dirty.load()||now-lastScan>=10000)){
   scanNow();
  }
  if(!configOpen&&automatic&&pending&&e.started&&e.canEditInserts()&&(lastAttempt==0||now-lastAttempt>=10000))reconnect();
 }
};
}
