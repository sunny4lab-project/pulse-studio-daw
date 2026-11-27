#pragma once
#include "TimelineSource.h"
namespace pulse {
// Runs in a separate helper process, never on the device callback.
// Only a simple stereo instrument layout is supported; the plugin's default
// state is used. Audio is rendered to a take that can be edited like any WAV.
inline juce::String renderInstrumentRequest(const juce::File& request){
 using namespace juce;auto xml=request.getSize()<=16*1024*1024?XmlDocument::parse(request):nullptr;
 if(!xml||!xml->hasTagName("InstrumentBounce"))return "Invalid instrument request.";
 PluginDescription description;auto* desc=xml->getChildByName("PLUGIN");
 if(!desc||!description.loadFromXml(*desc)||!description.isInstrument)return "Invalid instrument description.";
 auto output=xml->getStringAttribute("output");if(!File::isAbsolutePath(output))return "Invalid output path.";File file(output);if(file.exists())return "Output already exists.";
 std::vector<std::pair<Frame,MidiMessage>> events;Frame end=0;
 for(auto* x:xml->getChildWithTagNameIterator("Note")){Frame start=x->getStringAttribute("start").getLargeIntValue(),length=x->getStringAttribute("length").getLargeIntValue();int pitch=x->getIntAttribute("pitch",-1);float velocity=(float)x->getDoubleAttribute("velocity",-1);
  if(start<0||start>172800000||length<=0||length>172800000-start||pitch<0||pitch>127||!std::isfinite(velocity)||velocity<0||velocity>1)return "Invalid note or bounce exceeds 60 minutes.";
  events.push_back({start,MidiMessage::noteOn(1,pitch,velocity)});events.push_back({start+length,MidiMessage::noteOff(1,pitch)});end=std::max(end,start+length);}
 if(events.empty())return "No notes to render.";
 std::stable_sort(events.begin(),events.end(),[](auto& a,auto& b){if(a.first!=b.first)return a.first<b.first;return a.second.isNoteOff()&&!b.second.isNoteOff();});
 AudioPluginFormatManager formats;juce::addDefaultFormatsToManager(formats);String error;constexpr int block=512;
 auto plugin=formats.createPluginInstance(description,timelineRate,block,error);if(!plugin)return error.isEmpty()?String("Instrument could not be opened."):error;
 if(plugin->getTotalNumInputChannels()!=0||plugin->getTotalNumOutputChannels()!=2||!plugin->acceptsMidi())return "Only MIDI instruments with zero audio inputs and stereo outputs are supported.";
 plugin->setNonRealtime(true);plugin->prepareToPlay(timelineRate,block);
 struct Release{AudioPluginInstance& p;~Release(){p.releaseResources();}} release{*plugin};
 TemporaryFile tmp(file);auto stream=tmp.getFile().createOutputStream();if(!stream)return "Cannot create audio output.";
 WavAudioFormat wav;std::unique_ptr<AudioFormatWriter> writer(wav.createWriterFor(stream.get(),timelineRate,2,24,{},0));if(!writer)return "Cannot create WAV writer.";stream.release();
 AudioBuffer<float> audio(2,block);MidiBuffer midi;midi.ensureSize(8192);size_t event=0;Frame total=end+96000;
 for(Frame pos=0;pos<total;pos+=block){audio.clear();midi.clear();while(event<events.size()&&events[event].first<pos+block){midi.addEvent(events[event].second,(int)(events[event].first-pos));++event;}
  plugin->processBlock(audio,midi);int count=(int)std::min<Frame>(block,total-pos);for(int k=0;k<2;++k)for(int i=0;i<count;++i){float v=audio.getSample(k,i);audio.setSample(k,i,std::isfinite(v)?std::clamp(v,-1.f,1.f):0.f);}
  if(!writer->writeFromAudioSampleBuffer(audio,0,count))return "Instrument output write failed.";
 }
 writer.reset();if(file.exists()||!tmp.overwriteTargetFileWithTemporary())return "Could not finalize output.";return {};
}
class InstrumentBounceTask final:public juce::ThreadWithProgressWindow {
 juce::File requestDir;juce::PluginDescription description;std::vector<MidiNote> notes;
public:
 juce::File output;juce::String error;bool completed=false,cancelled=false;
 InstrumentBounceTask(juce::PluginDescription d,std::vector<MidiNote> n,juce::File f):ThreadWithProgressWindow("Rendering instrument in isolated helper",true,true),description(std::move(d)),notes(std::move(n)),output(std::move(f)){}
 ~InstrumentBounceTask()override{signalThreadShouldExit();stopThread(-1);if(requestDir!=juce::File{})requestDir.deleteRecursively();}
 void threadComplete(bool cancel)override{cancelled=cancel;completed=true;}
 void run()override{
  using namespace juce;setProgress(-1);requestDir=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("pulse-instrument-render","");if(!requestDir.createDirectory().wasOk()){error="Cannot create helper request directory.";return;}
  auto req=requestDir.getChildFile("request.xml");XmlElement root("InstrumentBounce");root.setAttribute("output",output.getFullPathName());root.addChildElement(description.createXml().release());
  for(auto& n:notes){auto* x=root.createNewChildElement("Note");x->setAttribute("start",String(n.start));x->setAttribute("length",String(n.length));x->setAttribute("pitch",n.pitch);x->setAttribute("velocity",(double)n.velocity);}
  if(!root.writeTo(req)){error="Cannot write instrument request.";return;}
  ChildProcess child;StringArray arguments{File::getSpecialLocation(File::currentExecutableFile).getFullPathName(),"--render-instrument",req.getFullPathName()};
  if(!child.start(arguments,0)){error="Could not launch render helper.";return;}
  double begin=Time::getMillisecondCounterHiRes();
  while(!child.waitForProcessToFinish(100)){if(threadShouldExit()||Time::getMillisecondCounterHiRes()-begin>300000){child.kill();child.waitForProcessToFinish(2000);error="Instrument render cancelled or timed out.";return;}}
  if(child.getExitCode()!=0||!output.existsAsFile()){error=requestDir.getChildFile("result.txt").loadFileAsString();if(error.isEmpty())error="Instrument helper failed or crashed.";return;}
 }
};
}
