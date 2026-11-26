#pragma once
#include "AudioEngine.h"

namespace pulse {
// Version 2 projects carry plugin descriptions and opaque state for all inserts
// and the master. A failed restore leaves the running rack untouched.
class PluginProject {
 static constexpr int maxStateBytes=16*1024*1024;
 static constexpr int maxTotalBytes=32*1024*1024;
 static constexpr int masterRack=N+B;
 static constexpr int slotCount=(N+B)*insertsPerRack+1;
 struct Pending {std::unique_ptr<juce::AudioPluginInstance> plugin;bool bypass=false;};
 using Slots=std::array<Pending,slotCount>;
 static int index(int rack,int slot){return rack==masterRack?(N+B)*insertsPerRack:rack*insertsPerRack+slot;}
 struct Detached {
  Engine& engine;bool attached;
  explicit Detached(Engine& e):engine(e),attached(e.started){if(attached)engine.devices.removeAudioCallback(&engine);}
  ~Detached(){if(attached)engine.devices.addAudioCallback(&engine);}
 };
public:
 static bool hasPlugins(const Engine& e){
  if(e.masterPlugin)return true;
  for(const auto& rack:e.inserts)for(const auto& slot:rack.slots)if(slot.plugin)return true;
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
   for(int r=0;r<N+B;++r)for(int s=0;s<insertsPerRack;++s){auto& x=e.inserts[r].slots[s];if(!saveOne(r,s,x.plugin.get(),x.bypass.load()))return "Plugin state exceeds project size limit.";}
   if(!saveOne(masterRack,0,e.masterPlugin.get(),e.pluginBypass.load()))return "Master plugin state exceeds project size limit.";
  }
  root.addChildElement(new juce::XmlElement(plugins));return {};
 }
 static juce::String restore(Engine& e,const juce::XmlElement& root){
  if(!e.canEditInserts())return "Stop live audio, playback and recording before opening a project.";
  auto* list=root.getChildByName("Plugins");
  if(!list&&root.getIntAttribute("version")==2)return "Missing plugin rack section.";
  if(list&&list->getNumChildElements()>slotCount)return "Too many plugin slots.";
  auto* d=e.devices.getCurrentAudioDevice();
  if(list&&list->getNumChildElements()>0&&(!d||!d->isPlaying()))return "Open Audio Device setup before restoring a project with plugins.";
  Slots pending;std::array<bool,slotCount> seen{};size_t total=0;
  if(list)for(auto* x:list->getChildIterator()){
   int rack=x->getIntAttribute("rack",-1),slot=x->getIntAttribute("slot",-1);
   if(!x->hasTagName("Effect")||rack<0||rack>masterRack||slot<0||slot>=insertsPerRack||(rack==masterRack&&slot!=0)||x->getNumChildElements()!=1)return "Invalid plugin slot in project.";
   int i=index(rack,slot);if(seen[i])return "Duplicate plugin slot.";seen[i]=true;
   juce::PluginDescription desc;if(!desc.loadFromXml(*x->getChildElement(0))||desc.isInstrument)return "Invalid or unsupported effect description.";
   juce::MemoryBlock state;auto encoded=x->getStringAttribute("state");
   if(encoded.length()>24*1024*1024||!state.fromBase64Encoding(encoded)||state.getSize()>maxStateBytes||total+state.getSize()>maxTotalBytes)return "Invalid or oversized plugin state.";
   total+=state.getSize();juce::String error;auto p=e.pluginFormats.createPluginInstance(desc,d->getCurrentSampleRate(),d->getCurrentBufferSizeSamples(),error);
   if(!p)return "Cannot restore "+desc.name+": "+error;
   if(p->getTotalNumInputChannels()!=2||p->getTotalNumOutputChannels()!=2)return desc.name+" is not a stereo effect.";
   p->prepareToPlay(d->getCurrentSampleRate(),d->getCurrentBufferSizeSamples());p->setStateInformation(state.getData(),(int)state.getSize());
   int latency=p->getLatencySamples();p->releaseResources();
   if(rack!=masterRack&&latency!=0)return desc.name+" reports latency; channel/bus compensation is not enabled.";
   pending[i].plugin=std::move(p);pending[i].bypass=x->getBoolAttribute("bypass",false);
  }
  if(!e.canEditInserts())return "Audio state changed; project was not restored.";
  {
   Detached detached(e);
   for(int r=0;r<N+B;++r)for(int s=0;s<insertsPerRack;++s){auto& src=pending[index(r,s)];auto& dst=e.inserts[r].slots[s];dst.plugin=std::move(src.plugin);dst.bypass=src.bypass;}
   auto& src=pending[index(masterRack,0)];e.masterPlugin=std::move(src.plugin);e.pluginBypass=src.bypass;
  }
  return {};
 }
};
}
