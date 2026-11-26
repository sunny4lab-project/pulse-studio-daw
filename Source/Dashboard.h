#pragma once
#include "AudioEngine.h"
#if JUCE_WINDOWS
 #include <windows.h>
 #include <psapi.h>
#elif JUCE_MAC
 #include <mach/mach.h>
#elif JUCE_LINUX
 #include <fstream>
 #include <unistd.h>
#endif
class Dashboard : public Component {
 Engine& e;
 String ram{"--"};
 int tick=0;
 static uint64 residentBytes(){
#if JUCE_WINDOWS
  PROCESS_MEMORY_COUNTERS m{};m.cb=sizeof(m);
  return K32GetProcessMemoryInfo(GetCurrentProcess(),&m,sizeof(m))?(uint64)m.WorkingSetSize:0;
#elif JUCE_MAC
  mach_task_basic_info_data_t m{};mach_msg_type_number_t n=MACH_TASK_BASIC_INFO_COUNT;
  return task_info(mach_task_self(),MACH_TASK_BASIC_INFO,reinterpret_cast<task_info_t>(&m),&n)==KERN_SUCCESS?(uint64)m.resident_size:0;
#elif JUCE_LINUX
  std::ifstream f("/proc/self/statm");unsigned long total=0,resident=0;
  if(f>>total>>resident){auto page=sysconf(_SC_PAGESIZE);if(page>0)return (uint64)resident*(uint64)page;}return 0;
#else
  return 0;
#endif
 }
public:
 explicit Dashboard(Engine& engine):e(engine){}
 void refresh(){if(tick++%15==0){auto bytes=residentBytes();ram=bytes?String((double)bytes/(1024.0*1024.0),1)+" MB":"Unavailable";}repaint();}
 void paint(Graphics& g) override {
  auto r=getLocalBounds().reduced(8);auto* d=e.devices.getCurrentAudioDevice();
  bool ready=d&&d->isPlaying();double rate=d?d->getCurrentSampleRate():0;
  int block=d?d->getCurrentBufferSizeSamples():0;
  float peak=e.peak.load();bool clipping=peak>=1.f;
  auto heading=r.removeFromTop(45);
  g.setColour(Colour(0xffedf6fc));g.setFont(Font(22.f,Font::bold));g.drawText("PULSE STUDIO",heading.removeFromTop(27),Justification::centredLeft);
  g.setColour(Colour(0xff92a9bb));g.setFont(11.f);g.drawText("ENTERPRISE / ENGINE OVERVIEW",heading,Justification::centredLeft);
  auto panel=[&](Rectangle<int> b){g.setColour(Colour(0xff0c1623));g.fillRoundedRectangle(b.toFloat(),7);g.setColour(Colour(0xff293c4d));g.drawRoundedRectangle(b.toFloat(),7,1);};
  auto title=[&](Rectangle<int> b,const String& text){g.setColour(Colour(0xff91aabd));g.setFont(Font(11.f,Font::bold));g.drawText(text,b,Justification::centredLeft);};
  auto metric=[&](Rectangle<int> b,const String& name,const String& value,const String& detail){b=b.reduced(3);panel(b);b=b.reduced(9);title(b.removeFromTop(16),name);g.setColour(Colour(0xff36d6d5));g.setFont(Font(25.f,Font::bold));g.drawFittedText(value,b.removeFromTop(34),Justification::centredLeft,1);g.setFont(10.f);g.setColour(Colour(0xff91aabd));g.drawFittedText(detail,b,Justification::centredLeft,1);};
  int h=std::clamp(r.getHeight()/5,70,100);
  auto row=r.removeFromTop(h);metric(row.removeFromLeft(row.getWidth()/2),"CPU",ready?String(e.devices.getCpuUsage()*100,1)+"%":"--","Audio callback load");metric(row,"RAM",ram,"Process resident memory");
  row=r.removeFromTop(h);metric(row.removeFromLeft(row.getWidth()/2),"SAMPLE RATE",ready&&rate>0?String(rate/1000,1)+" kHz":"--","Active audio device");metric(row,"LATENCY",ready&&rate>0?String(block*1000.0/rate,2)+" ms":"--","One buffer, not round-trip");
  r.removeFromTop(5);auto card=r.removeFromTop(std::min(r.getHeight(),204)).reduced(3);panel(card);card=card.reduced(10);title(card.removeFromTop(22),"SERVICE STATUS");
  auto status=[&](const String& name,const String& value,Colour color){auto b=card.removeFromTop(std::min(32,card.getHeight()));g.setColour(Colour(0xff1c2b39));g.drawHorizontalLine(b.getBottom()-1,(float)b.getX(),(float)b.getRight());g.setColour(Colour(0xffd2dfe9));g.setFont(12.f);g.drawFittedText(name,b.removeFromLeft(b.getWidth()*42/100),Justification::centredLeft,1);g.setColour(color);g.setFont(Font(11.f,Font::bold));g.drawFittedText(value,b,Justification::centredRight,1);};
  const auto neutral=Colour(0xff99afc1),green=Colour(0xff36d99b),warn=Colour(0xffe6b45c);
  status("FOH Status","NOT CONFIGURED",neutral);
  status("Broadcast Status",ready?(clipping?"OUTPUT CLIPPING":"ROUTE UNVERIFIED"):"DEVICE OFFLINE",ready&&clipping?warn:neutral);
  bool dante=ready&&d->getName().containsIgnoreCase("Dante");
  status("Dante Status",dante?"DRIVER ACTIVE":"NOT VERIFIED",dante?green:neutral);
  status("OBS Status","NOT CONNECTED",neutral);
  if(r.getHeight()>18){g.setColour(neutral);g.setFont(10.f);g.drawFittedText("Status reflects verified local information only.",r.reduced(5),Justification::topLeft,2);}
 }
};
