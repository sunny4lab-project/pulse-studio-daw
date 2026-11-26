#pragma once
#include "AudioEngine.h"
class MonitorPanel : public Component {
 Engine& e;
public:
 int mixMode=1;
 std::array<String,4> names{"Drums","Instruments","Vocals","Other"};
 explicit MonitorPanel(Engine& engine):e(engine){}
 void paint(Graphics& g) override {
  auto r=getLocalBounds().reduced(3);
  auto card=[&](int h,const String& title){auto q=r.removeFromTop(h).reduced(0,3);g.setColour(Colour(0xff0c1623));g.fillRoundedRectangle(q.toFloat(),6);g.setColour(Colour(0xff26394a));g.drawRoundedRectangle(q.toFloat(),6,1);auto inner=q.reduced(8);g.setFont(Font(12.f,Font::bold));g.setColour(Colour(0xffb6c9d9));g.drawText(title,inner.removeFromTop(20),Justification::centredLeft);return inner;};
  auto meter=[&](Rectangle<int> bar,float p){float v=jlimit(0.f,1.f,(Decibels::gainToDecibels(p,-60.f)+60.f)/60.f);g.setColour(Colour(0xff243445));g.fillRoundedRectangle(bar.toFloat(),3);if(v>0){auto fill=bar.withWidth((int)(bar.getWidth()*v));g.setGradientFill(ColourGradient(Colour(0xff16c886),bar.getX(),0.f,Colour(0xff29d6db),bar.getRight(),0.f,false));g.fillRoundedRectangle(fill.toFloat(),3);}};
  auto q=card(145,"MIX BUSES");g.setFont(11.f);
  for(int i=0;i<4;++i){auto row=q.removeFromTop(21);g.setColour(Colour(0xffd3dfe8));g.drawFittedText(names[i],row.removeFromLeft(std::min(74,row.getWidth()/2)),Justification::centredLeft,1);meter(row.reduced(3,6),e.busPeak[i].load());}
  q=card(104,"AI ENGINEER");g.setFont(11.f);g.setColour(Colour(0xff31cfd0));g.drawFittedText(mixMode==1?"Manual control":mixMode==2?"Assisted level matching":"Automatic level matching",q.removeFromTop(20),Justification::centredLeft,1);g.setColour(Colour(0xffa9bac8));g.drawFittedText(e.enabled.load()?"Live inputs active":"Live inputs disabled",q.removeFromTop(18),Justification::centredLeft,1);g.drawFittedText("Feedback analysis: not available",q.removeFromTop(18),Justification::centredLeft,1);
  q=card(116,"OUTPUT MONITOR");float db=Decibels::gainToDecibels(e.peak.load(),-100.f);g.setColour(db>=0?Colours::orange:Colour(0xff31d5d5));g.setFont(Font(26.f,Font::bold));g.drawText(String(db,1)+" dBFS",q.removeFromTop(36),Justification::centredLeft);meter(q.removeFromTop(12),e.peak.load());g.setFont(10.f);g.setColour(Colour(0xff9eb4c5));g.drawText("Sample peak | LUFS pending",q.removeFromTop(18),Justification::centredLeft);
  q=card(124,"SYSTEM STATUS");g.setFont(11.f);g.setColour(Colour(0xffd3dfe8));auto line=[&](const String& s){g.drawFittedText(s,q.removeFromTop(18),Justification::centredLeft,1);};auto* d=e.devices.getCurrentAudioDevice();line(d?"Audio device connected":"Audio device disconnected");line("CPU  "+String(e.devices.getCpuUsage()*100,1)+"%");line(d?String(d->getCurrentSampleRate()/1000,1)+" kHz / "+String(d->getCurrentBufferSizeSamples())+" samples":"Sample rate / buffer: --");line("Recording write errors: "+String((int)e.missedWrites.load()));
 }
};
