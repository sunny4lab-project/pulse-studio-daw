#pragma once
#include "AudioEngine.h"
namespace pulse {
// Project files reference original media paths, never duplicate large WAV files.
// Open and validate every reader before changing the current session.
class ProjectMedia {
 juce::Array<juce::File> files;
 std::unique_ptr<StemSource> source;
 std::array<float,stemCount> trims{};
 std::array<bool,stemCount> mutes{};
 double cursor=0;
public:
 static void save(Engine& e,juce::XmlElement& root){
  root.setAttribute("mediaCount",e.stemFiles.size());
  root.setAttribute("mediaCursor",e.transport.getCurrentPosition());
  for(int i=0;i<e.stemFiles.size();++i){
   root.setAttribute("mediaPath"+juce::String(i),e.stemFiles[i].getFullPathName());
   root.setAttribute("mediaTrim"+juce::String(i),(double)e.stemTrims[i].load());
   root.setAttribute("mediaMute"+juce::String(i),e.stemMutes[i].load());
  }
 }
 juce::String stage(Engine& e,const juce::XmlElement& root){
  int n=root.getIntAttribute("mediaCount",0);
  if(n<0||n>stemCount)return "Invalid stem count.";
  files.clear();source.reset();trims.fill(1.f);mutes.fill(false);
  cursor=root.getDoubleAttribute("mediaCursor",0);if(!std::isfinite(cursor)||cursor<0)cursor=0;
  if(!n)return {};
  source=std::make_unique<StemSource>(e.stemTrims,e.stemMutes);
  for(int i=0;i<n;++i){
   auto path=root.getStringAttribute("mediaPath"+juce::String(i));
   if(!juce::File::isAbsolutePath(path))return "Invalid stem file path.";
   juce::File file(path);
   if(!file.existsAsFile()||!source->add(std::unique_ptr<juce::AudioFormatReader>(e.formats.createReaderFor(file))))return "Missing or incompatible stem: "+file.getFileName();
   files.add(file);double g=root.getDoubleAttribute("mediaTrim"+juce::String(i),1);
   trims[i]=std::isfinite(g)?(float)juce::jlimit(0.0,1.0,g):1.f;
   mutes[i]=root.getBoolAttribute("mediaMute"+juce::String(i),false);
  }
  return {};
 }
 void apply(Engine& e){
  e.transport.stop();e.transport.setSource(nullptr);
  for(int i=0;i<stemCount;++i){e.stemTrims[i]=trims[i];e.stemMutes[i]=mutes[i];}
  e.reader=std::move(source);e.stemFiles=files;
  if(e.reader){e.transport.setSource(e.reader.get(),16384,&e.stemDisk,e.reader->getRate());e.transport.setPosition(juce::jlimit(0.0,e.transport.getLengthInSeconds(),cursor));}
 }
};
}
