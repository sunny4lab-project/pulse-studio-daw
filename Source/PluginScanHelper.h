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
inline bool testPluginScanHelper(){
 using namespace juce;
 auto dir=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("pulse-scan-test","");
 if(!dir.createDirectory().wasOk())return false;
 struct Cleanup{File folder;~Cleanup(){folder.deleteRecursively();}} cleanup{dir};
 auto req=dir.getChildFile("request.xml");auto result=dir.getChildFile("result.xml");
 if(runPluginScanRequest(req))return false;
 auto invoke=[&](const String& format,bool expectSuccess){
  XmlElement xml("PluginScan");xml.setAttribute("action","discover");xml.setAttribute("format",format);
  result.deleteFile();if(!xml.writeTo(req))return false;
  ChildProcess child;if(!child.start(StringArray{File::getSpecialLocation(File::currentExecutableFile).getFullPathName(),"--scan-plugin-helper",req.getFullPathName()},0))return false;
  if(!child.waitForProcessToFinish(30000)){child.kill();return false;}
  if(!expectSuccess)return child.getExitCode()!=0&&!result.exists();
  if(child.getExitCode()!=0)return false;
  auto out=XmlDocument::parse(result);return out&&out->hasTagName("PluginScanResult");
 };
 return invoke("NoSuchPluginFormat",false)&&invoke("VST3",true);
}
}
