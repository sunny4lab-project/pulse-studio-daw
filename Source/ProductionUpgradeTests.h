#pragma once
#include "AudioEngine.h"
#include "ProjectRecovery.h"
#include "ProjectMedia.h"
namespace pulse {
inline bool testProductionUpgrade(){
 using namespace juce;
 if(std::abs(maskingCut(.01f,.001f)+3.f)>.001f||maskingCut(.01f,0)!=0)return false;
 if(masterStep(-22,-18,-1,-6,-6)!=0||masterStep(-22,-18,-10,-6,-6)<=0||masterStep(-12,-18,-10,-6,-6)>=0||masterStep(-22,-18,-10,-3,-6)>.0001f)return false;
 MixSpectrum m;std::array<float,1024> x{};for(int i=0;i<(int)x.size();++i)x[i]=.1f*std::sin(6.2831853f*100.f*i/48000.f);
 for(int i=0;i<40;++i)m.analyze(x.data(),nullptr,(int)x.size(),48000);if(m.energy[0]<=m.energy[2]||m.rms<=0)return false;
 Engine e;TimelineSession session;TimelineTrack t;t.bus=9;t.notes.push_back({0,24000,69,.8f});session.tracks.push_back(t);
 String error;auto routed=TimelineSource::create(e.formats,session,error);if(!routed||routed->busMask()!=(1u<<9))return false;routed->enableBusOutputs();
 AudioBuffer<float> data(22,512);routed->setNextReadPosition(3000);routed->getNextAudioBlock({&data,0,512});
 if(data.getMagnitude(20,0,512)<.001f||data.getMagnitude(0,0,512)!=0||data.getMagnitude(18,0,512)!=0)return false;
 auto stereo=TimelineSource::create(e.formats,session,error);if(!stereo)return false;AudioBuffer<float> render(2,512);stereo->setNextReadPosition(3000);stereo->getNextAudioBlock({&render,0,512});if(render.getMagnitude(0,512)<.001f)return false;
 if(e.setTimeline(session,0).isNotEmpty()||e.arrangementMask.load()!=(1u<<9))return false;
 XmlElement root("PulseProject");root.setAttribute("version",8);ProjectMedia::save(e,root);ProjectMedia media;if(media.stage(e,root).isNotEmpty())return false;media.apply(e);if(e.arrangementMask.load()!=(1u<<9))return false;
 auto dir=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("pulse-recovery-test","");if(!dir.createDirectory().wasOk())return false;
 struct Cleanup{File f;~Cleanup(){f.deleteRecursively();}} cleanup{dir};auto f=dir.getChildFile("session.pulse");
 if(writeProjectSafely(root,f,true).isNotEmpty())return false;root.setAttribute("version",9);if(writeProjectSafely(root,f,true).isNotEmpty())return false;
 auto previous=XmlDocument::parse(f.getSiblingFile("session.pulse.bak"));auto current=XmlDocument::parse(f);return previous&&current&&previous->getIntAttribute("version")==8&&current->getIntAttribute("version")==9;
}
}
