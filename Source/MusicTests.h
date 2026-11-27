#pragma once
#include "ProjectMedia.h"
#include "TimelineExport.h"
namespace pulse {
inline bool testMusic(){
 using namespace juce;
 Engine e;TimelineSession session;TimelineTrack t;t.name="MIDI keys";t.instrument=1;t.notes={{0,24000,69,.8f},{12000,24000,72,.5f}};
 putAutomation(t.gainAutomation,0,0);putAutomation(t.gainAutomation,12000,1);putAutomation(t.gainAutomation,24000,.5f);putAutomation(t.gainAutomation,12000,.8f);
 putAutomation(t.panAutomation,0,-1);putAutomation(t.panAutomation,24000,1);
 if(t.gainAutomation.size()!=3||std::abs(automationAt(t.gainAutomation,6000,1)-.4f)>.0001f||std::abs(automationAt(t.panAutomation,12000,0))>.0001f)return false;
 session.tracks.push_back(t);String error;auto s=TimelineSource::create(e.formats,session,error);if(!s||s->getTotalLength()!=36000)return false;
 AudioBuffer<float> a(2,512),b(2,512);s->setNextReadPosition(6000);s->getNextAudioBlock({&a,0,512});if(a.getMagnitude(0,512)<.001f||a.getMagnitude(0,512)<=a.getMagnitude(1,512))return false;
 s->setNextReadPosition(6000);s->getNextAudioBlock({&b,0,512});for(int k=0;k<2;++k)for(int i=0;i<512;++i)if(a.getSample(k,i)!=b.getSample(k,i))return false;
 s->setNextReadPosition(30000);s->getNextAudioBlock({&b,0,512});if(b.getMagnitude(1,512)<.001f||b.getMagnitude(0,512)>.00001f)return false;
 auto bad=session;bad.tracks[0].notes[0].pitch=128;if(TimelineSource::create(e.formats,bad,error))return false;
 bad=session;bad.tracks[0].gainAutomation.push_back({0,1});if(TimelineSource::create(e.formats,bad,error))return false;
 if(e.setTimeline(session,0).isNotEmpty())return false;
 XmlElement xml("PulseProject");xml.setAttribute("version",7);ProjectMedia::save(e,xml);ProjectMedia media;if(media.stage(e,xml).isNotEmpty())return false;media.apply(e);
 if(!e.timeline)return false;auto& restored=e.timeline->session().tracks[0];if(restored.notes.size()!=2||restored.instrument!=1||restored.gainAutomation.size()!=3||restored.panAutomation.size()!=2||restored.notes[1].pitch!=72)return false;
 auto dir=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("pulse-midi-render","");if(!dir.createDirectory().wasOk())return false;
 struct Cleanup{File file;~Cleanup(){file.deleteRecursively();}} cleanup{dir};
 auto out=dir.getChildFile("midi.wav");auto render=TimelineSource::create(e.formats,session,error);if(!render||!renderTimelineWav(*render,out,{},error))return false;
 std::unique_ptr<AudioFormatReader> reader(e.formats.createReaderFor(out));return reader&&reader->lengthInSamples==36000&&reader->read(&b,0,512,6000,true,true)&&b.getMagnitude(0,512)>.001f;
}
}
