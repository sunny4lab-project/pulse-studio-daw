#pragma once
#include "AudioEngine.h"
class ArrangementView : public Component {
 Engine& engine;
 AudioThumbnailCache cache{4};
 AudioThumbnail waveform;
 String fileName;
public:
 explicit ArrangementView(Engine& e):engine(e),waveform(512,e.formats,cache){}
 void openFile(const File& f){fileName=f.getFileName();waveform.setSource(new FileInputSource(f));repaint();}
 Rectangle<int> track() const {return getLocalBounds().reduced(8).withTrimmedTop(28).withTrimmedLeft(120);}
 void paint(Graphics& g) override {
  g.fillAll(Colour(0xff19222c));g.setColour(Colour(0xffb7c9d7));g.setFont(14.f);
  g.drawText("ARRANGEMENT  /  Audio playback",8,2,getWidth()-16,24,Justification::centredLeft);
  auto r=track();if(r.isEmpty())return;g.setColour(Colour(0xff101820));g.fillRoundedRectangle(r.toFloat(),4);
  g.setColour(Colour(0xffb7c9d7));g.drawFittedText(fileName.isEmpty()?"Audio track":fileName,8,r.getY(),112,r.getHeight(),Justification::centredLeft,2);
  double length=waveform.getTotalLength();
  if(length<=0){g.drawText("Open audio to display its waveform",r,Justification::centred);return;}
  g.setColour(Colour(0xff276a78));g.fillRoundedRectangle(r.toFloat().reduced(1),4);
  g.setColour(Colour(0xff91e0df));waveform.drawChannels(g,r.reduced(2),0,length,1.f);
  float progress=(float)jlimit(0.0,1.0,engine.transport.getCurrentPosition()/length);
  g.setColour(Colours::white);g.drawVerticalLine(r.getX()+(int)(r.getWidth()*progress), (float)r.getY(),(float)r.getBottom());
 }
 void mouseDown(const MouseEvent& e) override {auto r=track();if(r.contains(e.getPosition()) && waveform.getTotalLength()>0)engine.transport.setPosition(jlimit(0.0,1.0,(double)(e.x-r.getX())/std::max(1,r.getWidth()))*waveform.getTotalLength());repaint();}
};
