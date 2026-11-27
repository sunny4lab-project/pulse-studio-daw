#pragma once
#include "AudioEngine.h"
namespace pulse {
// Project files reference original media paths, never duplicate large WAV files.
// Open and validate every reader before changing the current session.
class ProjectMedia {
 juce::Array<juce::File> files;
 std::unique_ptr<StemSource> source;std::unique_ptr<TimelineSource> timeline;
 std::array<float,stemCount> trims{};
 std::array<bool,stemCount> mutes{};
 double cursor=0;
public:
 static void save(Engine& e,juce::XmlElement& root){
  root.setAttribute("mediaCount",e.stemFiles.size());
  root.setAttribute("mediaCursor",e.transport.getCurrentPosition());
  if(e.timeline){auto* list=root.createNewChildElement("Timeline");for(auto& t:e.timeline->session().tracks){auto* x=list->createNewChildElement("Track");x->setAttribute("name",t.name);x->setAttribute("gain",(double)t.gain);x->setAttribute("pan",(double)t.pan);x->setAttribute("mute",t.mute);x->setAttribute("solo",t.solo);for(auto& c:t.clips){auto* y=x->createNewChildElement("Clip");y->setAttribute("path",c.file.getFullPathName());y->setAttribute("start",juce::String(c.start));y->setAttribute("offset",juce::String(c.offset));y->setAttribute("length",juce::String(c.length));y->setAttribute("fadeIn",juce::String(c.fadeIn));y->setAttribute("fadeOut",juce::String(c.fadeOut));y->setAttribute("gain",(double)c.gain);}}return;}
  for(int i=0;i<e.stemFiles.size();++i){
   root.setAttribute("mediaPath"+juce::String(i),e.stemFiles[i].getFullPathName());
   root.setAttribute("mediaTrim"+juce::String(i),(double)e.stemTrims[i].load());
   root.setAttribute("mediaMute"+juce::String(i),e.stemMutes[i].load());
  }
 }
 juce::String stage(Engine& e,const juce::XmlElement& root){
  int n=root.getIntAttribute("mediaCount",0);
  if(n<0||n>stemCount)return "Invalid stem count.";
  files.clear();source.reset();timeline.reset();trims.fill(1.f);mutes.fill(false);
  cursor=root.getDoubleAttribute("mediaCursor",0);if(!std::isfinite(cursor)||cursor<0)cursor=0;
  if(auto* list=root.getChildByName("Timeline")){
   if(n||root.getIntAttribute("version")<6)return "Invalid mixed timeline and legacy media section.";
   TimelineSession model;for(auto* x:list->getChildIterator()){if(!x->hasTagName("Track"))return "Invalid timeline track.";TimelineTrack t;t.name=x->getStringAttribute("name");t.gain=(float)x->getDoubleAttribute("gain",1);t.pan=(float)x->getDoubleAttribute("pan",0);t.mute=x->getBoolAttribute("mute",false);t.solo=x->getBoolAttribute("solo",false);
    for(auto* y:x->getChildIterator()){auto path=y->getStringAttribute("path");if(!y->hasTagName("Clip")||!juce::File::isAbsolutePath(path))return "Invalid timeline clip path.";AudioClip c;c.file=juce::File(path);c.start=y->getStringAttribute("start").getLargeIntValue();c.offset=y->getStringAttribute("offset").getLargeIntValue();c.length=y->getStringAttribute("length").getLargeIntValue();c.fadeIn=y->getStringAttribute("fadeIn").getLargeIntValue();c.fadeOut=y->getStringAttribute("fadeOut").getLargeIntValue();c.gain=(float)y->getDoubleAttribute("gain",1);t.clips.push_back(std::move(c));}model.tracks.push_back(std::move(t));}
   juce::String error;timeline=TimelineSource::create(e.formats,std::move(model),error);return timeline?juce::String():error;
  }
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
  if(timeline){e.applyTimeline(std::move(timeline),cursor);return;}
  e.transport.stop();e.transport.setSource(nullptr);e.timeline.reset();
  for(int i=0;i<stemCount;++i){e.stemTrims[i]=trims[i];e.stemMutes[i]=mutes[i];}
  e.reader=std::move(source);e.stemFiles=files;
  if(e.reader){e.transport.setSource(e.reader.get(),16384,&e.stemDisk,e.reader->getRate());e.transport.setPosition(juce::jlimit(0.0,e.transport.getLengthInSeconds(),cursor));}
 }
};
}
