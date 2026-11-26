#pragma once
#include "AudioEngine.h"
class MonitorPanel : public Component {
 Engine& e;
public:
 explicit MonitorPanel(Engine& engine):e(engine){}
 void paint(Graphics& g) override {
  auto r=getLocalBounds().reduced(6);
  auto card=[&](int h,const String& title){auto q=r.removeFromTop(h).reduced(0,3);g.setColour(Colour(0xff1a242e));g.fillRoundedRectangle(q.toFloat(),6);g.setColour(Colour(0xff344554));g.drawRoundedRectangle(q.toFloat(),6,1);auto inner=q.reduced(10);g.setFont(Font(12.f,Font::bold));g.setColour(Colour(0xff9db2c4));g.drawText(title,inner.removeFromTop(20),Justification::centredLeft);return inner;};
  auto q=card(136,"STEREO OUTPUT");
  float db=Decibels::gainToDecibels(e.peak.load(),-100.f);
  g.setColour(Colour(0xff45d9ca));g.setFont(28.f);g.drawText(String(db,1)+" dBFS",q.removeFromTop(40),Justification::centredLeft);
  auto bar=q.removeFromTop(12);g.setColour(Colour(0xff0c1218));g.fillRoundedRectangle(bar.toFloat(),3);g.setColour(db>=0?Colours::orange:Colour(0xff35c99b));g.fillRoundedRectangle(bar.withWidth((int)(bar.getWidth()*jlimit(0.f,1.f,(db+60)/60))).toFloat(),3);
  g.setFont(11.f);g.setColour(Colour(0xff9db2c4));g.drawText("Sample peak  /  -60 to 0 dBFS",q.removeFromTop(20),Justification::centredLeft);
  q=card(150,"AUDIO ENGINE");g.setFont(13.f);g.setColour(Colour(0xffd3dfe8));
  auto line=[&](const String& s){g.drawFittedText(s,q.removeFromTop(22),Justification::centredLeft,1);};
  auto* d=e.devices.getCurrentAudioDevice();line(d?"Device connected":"No active audio device");line("CPU   "+String(e.devices.getCpuUsage()*100,1)+"%");
  line(d?String(d->getCurrentSampleRate()/1000,1)+" kHz  /  "+String(d->getCurrentBufferSizeSamples())+" samples":"Rate / buffer: unavailable");
  line("Write failures   "+String((int)e.missedWrites.load()));
  q=card(105,"LIVE SAFETY");g.setFont(13.f);g.setColour(e.enabled.load()?Colour(0xff35c99b):Colour(0xffe4ba65));line(e.enabled.load()?"Live inputs enabled":"Live inputs disabled");g.setColour(Colour(0xffd3dfe8));line("Master FX: "+String(e.masterPlugin?(e.pluginBypass.load()?"bypassed":"active"):"not loaded"));
 }
};
