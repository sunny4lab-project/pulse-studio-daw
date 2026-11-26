#pragma once
#include "PluginProject.h"
#include "ProjectMedia.h"
namespace pulse {
inline bool testProjectRecall(){
 using namespace juce;
 Engine e;
 XmlElement project("PulseProject");project.setAttribute("version",2);
 if(PluginProject::save(e,project).isNotEmpty()||!project.getChildByName("Plugins")||PluginProject::restore(e,project).isNotEmpty())return false;
 XmlElement missing("PulseProject");missing.setAttribute("version",2);
 if(PluginProject::restore(e,missing).isEmpty())return false;
 auto* bad=project.getChildByName("Plugins")->createNewChildElement("Effect");bad->setAttribute("rack",9999);
 if(PluginProject::restore(e,project).isEmpty())return false;
 project.getChildByName("Plugins")->deleteAllChildElements();
 auto folder=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("pulse-project-test","");
 if(!folder.createDirectory().wasOk())return false;
 struct Cleanup{File dir;~Cleanup(){dir.deleteRecursively();}} cleanup{folder};
 auto file=folder.getChildFile("stem.wav");
 {
  auto stream=file.createOutputStream();if(!stream)return false;
  WavAudioFormat wav;std::unique_ptr<AudioFormatWriter> writer(wav.createWriterFor(stream.get(),48000,1,24,{},0));if(!writer)return false;stream.release();
  AudioBuffer<float> samples(1,256);samples.clear();samples.setSample(0,0,.5f);if(!writer->writeFromAudioSampleBuffer(samples,0,256))return false;
 }
 project.setAttribute("mediaCount",1);project.setAttribute("mediaPath0",file.getFullPathName());project.setAttribute("mediaTrim0",.5);project.setAttribute("mediaMute0",true);
 ProjectMedia media;if(media.stage(e,project).isNotEmpty())return false;media.apply(e);
 bool ok=e.stemFiles.size()==1&&e.stemFiles[0]==file&&e.stemTrims[0].load()==.5f&&e.stemMutes[0].load()&&!e.transport.isPlaying();
 XmlElement saved("PulseProject");ProjectMedia::save(e,saved);ok=ok&&saved.getIntAttribute("mediaCount")==1&&saved.getStringAttribute("mediaPath0")==file.getFullPathName()&&saved.getBoolAttribute("mediaMute0");
 project.setAttribute("mediaPath0",folder.getChildFile("missing.wav").getFullPathName());ProjectMedia badMedia;
 ok=ok&&badMedia.stage(e,project).isNotEmpty()&&e.stemFiles.size()==1&&e.stemFiles[0]==file;
 XmlElement empty("PulseProject");ProjectMedia clear;if(clear.stage(e,empty).isNotEmpty())return false;clear.apply(e);
 return ok&&e.stemFiles.isEmpty()&&!e.reader;
}
}
