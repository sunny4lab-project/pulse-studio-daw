#pragma once
#include "AudioEngine.h"
class ArrangementView : public Component {
 Engine& engine;
 AudioThumbnailCache cache{4};
 AudioThumbnail waveform;
 String fileName;
 double span() const {return std::max(10.0,waveform.getTotalLength());}
public:
 explicit ArrangementView(Engine& e):engine(e),waveform(512,e.formats,cache){}
 void openFile(const File& f){fileName=f.getFileName();waveform.setSource(new FileInputSource(f));repaint();}
 Rectangle<int> track() const {return getLocalBounds().reduced(5).withTrimmedTop(44).withTrimmedLeft(100);}
 void paint(Graphics& g) override {
  g.fillAll(Colour(0xff131e28));auto r=track();if(r.isEmpty())return;
  g.setColour(Colour(0xff243440));g.fillRect(0,0,getWidth(),24);g.setFont(Font(12.f,Font::bold));g.setColour(Colour(0xffd1dce5));
  g.drawText("TIMELINE",8,0,100,24,Justification::centredLeft);g.setFont(11.f);g.drawText("Audio preview  /  seconds",110,0,getWidth()-115,24,Justification::centredLeft);
  g.setColour(Colour(0xff0f1720));g.fillRect(r);
  double length=waveform.getTotalLength(), duration=span();
  int ticks=std::max(2,r.getWidth()/65);
  for(int i=0;i<=ticks;++i){int x=r.getX()+r.getWidth()*i/ticks;g.setColour(Colour(0xff30404f));g.drawVerticalLine(x,25,(float)r.getBottom());if(i<ticks){g.setColour(Colour(0xff9eb4c5));g.drawText(String(duration*i/ticks,1),x+3,25,55,17,Justification::centredLeft);}}
  auto lane=r.withHeight(std::min(64,r.getHeight()));g.setColour(Colour(0xff22313d));g.fillRect(5,lane.getY(),96,lane.getHeight());g.setColour(Colour(0xff4cb6bd));g.fillRect(5,lane.getY(),3,lane.getHeight());
  g.setColour(Colour(0xffd1dce5));g.drawFittedText(fileName.isEmpty()?"Audio preview":fileName,13,lane.getY()+3,84,lane.getHeight()-6,Justification::centredLeft,2);
  if(length>0){auto clip=lane.withWidth(std::max(1,(int)(lane.getWidth()*length/duration))).reduced(1);g.setColour(Colour(0xff24596a));g.fillRoundedRectangle(clip.toFloat(),3);g.setColour(Colour(0xff8bd5dd));waveform.drawChannels(g,clip.reduced(2),0,length,1.f);}
  else {g.setColour(Colour(0xff8a9cac));g.drawFittedText("Double-click an audio file in Browser or select Import",lane.reduced(6),Justification::centred,2);}
  for(int y=lane.getBottom();y<r.getBottom();y+=25){g.setColour(Colour(0xff22313c));g.drawHorizontalLine(y,(float)r.getX(),(float)r.getRight());}
  int x=r.getX()+(int)(r.getWidth()*jlimit(0.0,1.0,engine.transport.getCurrentPosition()/duration));g.setColour(Colour(0xff36d4cf));g.drawVerticalLine(x,25,(float)r.getBottom());
 }
 void mouseDown(const MouseEvent& e) override {auto r=track();if(r.contains(e.getPosition()) && waveform.getTotalLength()>0)engine.transport.setPosition(jlimit(0.0,waveform.getTotalLength(),(double)(e.x-r.getX())/std::max(1,r.getWidth())*span()));repaint();}
};
