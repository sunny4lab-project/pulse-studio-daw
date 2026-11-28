#pragma once
#include "TestDiagnostics.h"
#include "PluginProject.h"
#include "ProjectMedia.h"
namespace pulse {
inline bool testProjectRecall(){
 using namespace juce;
 Engine e;if(!e.addChannel())return pulse::testFailure(__FILE__,__LINE__);
 auto config=e.pluginPreparation();if(config.connected||config.sampleRate!=48000.0||config.blockSize!=256||e.devices.getCurrentAudioDevice()!=nullptr||e.enabled.load())return pulse::testFailure(__FILE__,__LINE__);
 PluginDescription instrument;instrument.isInstrument=true;
 if(e.changeInsertDescription(0,0,&instrument)!="Virtual instruments are not yet supported.")return pulse::testFailure(__FILE__,__LINE__);
 if(e.changeInsertDescription(0,0,nullptr).isNotEmpty()||e.devices.getCurrentAudioDevice()!=nullptr||e.enabled.load())return pulse::testFailure(__FILE__,__LINE__);
 XmlElement project("PulseProject");project.setAttribute("version",4);project.setAttribute("channelCount",1);e.rackAt(0).visibleSlots=12;e.masterInserts.visibleSlots=16;
 if(PluginProject::save(e,project).isNotEmpty()||!project.getChildByName("Plugins")||PluginProject::restore(e,project).isNotEmpty())return pulse::testFailure(__FILE__,__LINE__);
 if(e.rackAt(0).visibleSlots!=12||e.masterInserts.visibleSlots!=16)return pulse::testFailure(__FILE__,__LINE__);
 XmlElement missing("PulseProject");missing.setAttribute("version",3);
 if(PluginProject::restore(e,missing).isEmpty())return pulse::testFailure(__FILE__,__LINE__);
 auto* bad=project.getChildByName("Plugins")->createNewChildElement("Effect");bad->setAttribute("rack",9999);
 if(PluginProject::restore(e,project).isEmpty())return pulse::testFailure(__FILE__,__LINE__);
 project.getChildByName("Plugins")->deleteAllChildElements();
 auto folder=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("pulse-project-test","");
 if(!folder.createDirectory().wasOk())return pulse::testFailure(__FILE__,__LINE__);
 struct Cleanup{File dir;~Cleanup(){dir.deleteRecursively();}} cleanup{folder};
 auto file=folder.getChildFile("stem.wav");
 {
  auto stream=file.createOutputStream();if(!stream)return pulse::testFailure(__FILE__,__LINE__);
  WavAudioFormat wav;std::unique_ptr<AudioFormatWriter> writer(wav.createWriterFor(stream.get(),48000,1,24,{},0));if(!writer)return pulse::testFailure(__FILE__,__LINE__);stream.release();
  AudioBuffer<float> samples(1,256);samples.clear();samples.setSample(0,0,.5f);if(!writer->writeFromAudioSampleBuffer(samples,0,256))return pulse::testFailure(__FILE__,__LINE__);
 }
 project.setAttribute("mediaCount",1);project.setAttribute("mediaPath0",file.getFullPathName());project.setAttribute("mediaTrim0",.5);project.setAttribute("mediaMute0",true);
 ProjectMedia media;if(media.stage(e,project).isNotEmpty())return pulse::testFailure(__FILE__,__LINE__);media.apply(e);
 bool ok=e.stemFiles.size()==1&&e.stemFiles[0]==file&&e.stemTrims[0].load()==.5f&&e.stemMutes[0].load()&&!e.transport.isPlaying();
 XmlElement saved("PulseProject");ProjectMedia::save(e,saved);ok=ok&&saved.getIntAttribute("mediaCount")==1&&saved.getStringAttribute("mediaPath0")==file.getFullPathName()&&saved.getBoolAttribute("mediaMute0");
 project.setAttribute("mediaPath0",folder.getChildFile("missing.wav").getFullPathName());ProjectMedia badMedia;
 ok=ok&&badMedia.stage(e,project).isNotEmpty()&&e.stemFiles.size()==1&&e.stemFiles[0]==file;
 XmlElement empty("PulseProject");ProjectMedia clear;if(clear.stage(e,empty).isNotEmpty())return pulse::testFailure(__FILE__,__LINE__);clear.apply(e);
 return ok&&e.stemFiles.isEmpty()&&!e.reader;
}
}
