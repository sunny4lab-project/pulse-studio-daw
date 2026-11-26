#pragma once
#include "AudioEngine.h"

namespace pulse {
// Version 2/3 projects carry plugin descriptions and opaque state for all inserts
// and the master. A failed restore leaves the running rack untouched.
class PluginProject {
 static constexpr int maxStateBytes=16*1024*1024;
 static constexpr int maxTotalBytes=32*1024*1024;
 
 struct Pending {std::unique_ptr<juce::AudioPluginInstance> plugin;bool bypass=false;};
 static size_t index(int rack,int slot){return (size_t)rack*insertsPerRack+slot;}
 struct Detached {
  Engine& engine;bool attached;
  explicit Detached(Engine& e):engine(e),attached(e.started){if(attached)engine.devices.removeAudioCallback(&engine);}
  ~Detached(){if(attached)engine.devices.addAudioCallback(&engine);}
 };
public:
 static bool hasPlugins(const Engine& e){
  for(int r=0;r<=e.rackCount();++r)for(const auto& slot:e.rackAt(r).slots)if(slot.plugin)return true;
  return false;
 }
 static juce::String save(Engine& e,juce::XmlElement& root){
  if(!e.canEditInserts())return "Stop live audio, playback and recording before saving a project.";
  juce::XmlElement plugins("Plugins");size_t total=0;
  {
   Detached detached(e);
   auto saveOne=[&](int rack,int slot,juce::AudioPluginInstance* p,bool bypass)->bool{
    if(!p)return true;
    juce::MemoryBlock state;p->getStateInformation(state);
    if(state.getSize()>maxStateBytes||total+state.getSize()>maxTotalBytes)return false;
    total+=state.getSize();auto* item=plugins.createNewChildElement("Effect");
    item->setAttribute("rack",rack);item->setAttribute("slot",slot);item->setAttribute("bypass",bypass);
    item->setAttribute("state",state.toBase64Encoding());
    auto desc=p->getPluginDescription().createXml();item->addChildElement(desc.release());return true;
   };
   for(int r=0;r<=e.rackCount();++r){plugins.setAttribute("slots"+juce::String(r),e.rackAt(r).visibleSlots);for(int s=0;s<insertsPerRack;++s){auto& x=e.rackAt(r).slots[s];if(!saveOne(r,s,x.plugin.get(),x.bypass.load()))return "Plugin state exceeds project size limit.";}}
  }
  root.addChildElement(new juce::XmlElement(plugins));return {};
 }
 static juce::String restore(Engine& e,const juce::XmlElement& root,int targetCount=-1,std::function<void()> beforeSwap={}){
  if(!e.canEditInserts())return "Stop live audio, playback and recording before opening a project.";
  auto* list=root.getChildByName("Plugins");
  if(!list&&root.getIntAttribute("version")>=2)return "Missing plugin rack section.";
  if(targetCount<0)targetCount=e.channelCount();int masterRack=targetCount+B;int oldMaster=targetCount+(root.getIntAttribute("version")<3?legacyBusCount:B);size_t slotCount=index(masterRack,insertsPerRack-1)+1;
  if(list&&(size_t)list->getNumChildElements()>slotCount)return "Too many plugin slots for this mixer.";
  auto* d=e.devices.getCurrentAudioDevice();
  if(list&&list->getNumChildElements()>0&&(!d||!d->isPlaying()))return "Open Audio Device setup before restoring a project with plugins.";
  std::vector<Pending> pending(slotCount);std::vector<bool> seen(slotCount,false);std::vector<int> rackLatency((size_t)masterRack,0),visible((size_t)masterRack+1,initialInsertSlots);size_t total=0;
  if(list&&root.getIntAttribute("version")>=4)for(int r=0;r<=masterRack;++r)visible[r]=juce::jlimit(initialInsertSlots,insertsPerRack,list->getIntAttribute("slots"+juce::String(r),initialInsertSlots));
  if(list)for(auto* x:list->getChildIterator()){
   int rack=x->getIntAttribute("rack",-1),slot=x->getIntAttribute("slot",-1);
   if(rack==oldMaster)rack=masterRack;else if(rack>=masterRack)return "Project has effects on buses 11–32. Move or remove those effects in the older version first.";
   if(!x->hasTagName("Effect")||rack<0||rack>masterRack||slot<0||slot>=insertsPerRack||(rack==masterRack&&root.getIntAttribute("version")<4&&slot!=0)||x->getNumChildElements()!=1)return "Invalid plugin slot in project.";
   auto i=index(rack,slot);if(seen[i])return "Duplicate plugin slot.";seen[i]=true;
   juce::PluginDescription desc;if(!desc.loadFromXml(*x->getChildElement(0))||desc.isInstrument)return "Invalid or unsupported effect description.";
   juce::MemoryBlock state;auto encoded=x->getStringAttribute("state");
   if(encoded.length()>24*1024*1024||!state.fromBase64Encoding(encoded)||state.getSize()>maxStateBytes||total+state.getSize()>maxTotalBytes)return "Invalid or oversized plugin state.";
   total+=state.getSize();juce::String error;auto p=e.pluginFormats.createPluginInstance(desc,d->getCurrentSampleRate(),d->getCurrentBufferSizeSamples(),error);
   if(!p)return "Cannot restore "+desc.name+": "+error;
   if(p->getTotalNumInputChannels()!=2||p->getTotalNumOutputChannels()!=2)return desc.name+" is not a stereo effect.";
   p->prepareToPlay(d->getCurrentSampleRate(),d->getCurrentBufferSizeSamples());p->setStateInformation(state.getData(),(int)state.getSize());
   int latency=p->getLatencySamples();p->releaseResources();
   if(rack!=masterRack){if(latency<0||latency>maxRackLatency-rackLatency[rack])return desc.name+" exceeds the per-rack latency budget.";rackLatency[rack]+=latency;}visible[rack]=std::max(visible[rack],slot+1);
   pending[i].plugin=std::move(p);pending[i].bypass=x->getBoolAttribute("bypass",false);
  }
  pulse::StableList<Channel> newChannels;pulse::StableList<InsertRack> newRacks;newChannels.reserve(targetCount);newRacks.reserve(targetCount);for(int i=0;i<targetCount;++i){newChannels.append().source=-1;newRacks.append();}
  if(!e.canEditInserts())return "Audio state changed; project was not restored.";
  {
   Detached detached(e);
   if(beforeSwap)beforeSwap();e.ch.swap(newChannels);e.channelInserts.swap(newRacks);
   for(int r=0;r<=e.rackCount();++r){e.rackAt(r).visibleSlots=visible[r];for(int s=0;s<insertsPerRack;++s){auto& src=pending[index(r,s)];auto& dst=e.rackAt(r).slots[s];dst.plugin=std::move(src.plugin);dst.bypass=src.bypass;}}
  }
  return {};
 }
};
}
