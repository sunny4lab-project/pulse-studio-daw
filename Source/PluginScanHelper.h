#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
namespace pulse {
// Each request is handled by a short-lived process, on that process's
// message thread (some VST3/AU implementations require it).
inline bool runPluginScanRequest(const juce::File& request){
 using namespace juce;
 if(!request.existsAsFile()||request.getSize()>65536)return false;
 auto xml=XmlDocument::parse(request);if(!xml||!xml->hasTagName("PluginScan"))return false;
 const auto action=xml->getStringAttribute("action"),format=xml->getStringAttribute("format"),path=xml->getStringAttribute("path");
 if(action!="discover"&&action!="scan")return false;
 AudioPluginFormatManager manager;juce::addDefaultFormatsToManager(manager);
 XmlElement result("PluginScanResult");bool matched=false;
 for(auto* f:manager.getFormats()){
  if(format.isNotEmpty()&&format!=f->getName())continue;
  if(action=="discover"){
   matched=true;
   for(auto& candidate:f->searchPathsForPlugins(f->getDefaultLocationsToSearch(),true,false)){
    auto* x=result.createNewChildElement("Candidate");x->setAttribute("format",f->getName());x->setAttribute("path",candidate);
   }
  }else if(path.isNotEmpty()&&f->fileMightContainThisPluginType(path)){
   matched=true;OwnedArray<PluginDescription> descriptions;f->findAllTypesForFile(descriptions,path);
   for(auto* d:descriptions)result.addChildElement(d->createXml().release());
  }
 }
 if(!matched)return false;
 auto dest=request.getSiblingFile("result.xml");TemporaryFile tmp(dest);
 return result.writeTo(tmp.getFile())&&tmp.getFile().getSize()<=32*1024*1024&&tmp.overwriteTargetFileWithTemporary();
}
}
