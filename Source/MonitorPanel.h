#pragma once
#include "AudioEngine.h"
#include "UI/EnterpriseTheme.h"
class MonitorPanel : public Component {
 Engine& e;bool compact=true;
 TextButton collapse{"Expand"};
 static String dbText(float p){return String(Decibels::gainToDecibels(p,-100.f),1);}
 static void horizontalMeter(Graphics& g,Rectangle<int> bar,float p){
  if(bar.isEmpty())return;float v=jlimit(0.f,1.f,(Decibels::gainToDecibels(p,-60.f)+60.f)/60.f);
  g.setColour(Colour(0xff243445));g.fillRoundedRectangle(bar.toFloat(),2);
  if(v>0){g.setColour(p>=.99f?Colours::orange:PulseTheme::cyan);g.fillRoundedRectangle(bar.withWidth((int)(bar.getWidth()*v)).toFloat(),2);}
 }
public:
 int mixMode=1;
 std::array<String,B> names{};
 std::function<void()> onToggle;
 explicit MonitorPanel(Engine& engine):e(engine){
  for(int b=0;b<B;++b)names[b]="Bus "+String(b+1);
  addAndMakeVisible(collapse);collapse.onClick=[this]{if(onToggle)onToggle();};setCompact(true);
 }
 void setCompact(bool value){compact=value;collapse.setButtonText(compact?String("<"):String("Collapse >"));collapse.setTooltip(compact?"Expand monitoring panel":"Collapse monitoring panel");resized();repaint();}
 void resized()override{auto r=getLocalBounds().reduced(5);collapse.setBounds(r.removeFromTop(24).removeFromRight(compact?r.getWidth():84));}
 void paint(Graphics& g)override{
  auto bounds=getLocalBounds();g.setColour(PulseTheme::panel);g.fillRoundedRectangle(bounds.toFloat(),5);g.setColour(PulseTheme::border);g.drawRoundedRectangle(bounds.toFloat().reduced(.5f),5,1);
  auto r=bounds.reduced(compact?5:9,5);
  auto line=[&](const String& s,int size,Colour c,int height,bool bold=false){auto q=r.removeFromTop(std::min(height,r.getHeight()));if(q.isEmpty())return;g.setColour(c);g.setFont(PulseTheme::font((float)size,bold));g.drawFittedText(s,q,compact?Justification::centred:Justification::centredLeft,1,.85f);};
  float peak=e.peak.load();float lufs=e.shortLufs.load();
  if(compact){
   r.removeFromTop(30);line("OUT",10,PulseTheme::muted,16,true);
   int meterHeight=std::min(146,std::max(0,r.getHeight()-112));auto meter=r.removeFromTop(meterHeight).withSizeKeepingCentre(10,meterHeight); 
   if(!meter.isEmpty()){g.setColour(Colour(0xff243445));g.fillRoundedRectangle(meter.toFloat(),3);float v=jlimit(0.f,1.f,(Decibels::gainToDecibels(peak,-60.f)+60.f)/60.f);g.setColour(peak>=.99f?Colours::orange:PulseTheme::cyan);if(v>0)g.fillRoundedRectangle(meter.removeFromBottom((int)(meter.getHeight()*v)).toFloat(),3);}
   line(peak>0?dbText(peak):String("-inf"),11,PulseTheme::text,17,true);line("dBFS",9,PulseTheme::muted,13);
   r.removeFromTop(5);line(mixMode==1?"MAN":mixMode==2?"AST":mixMode==4?"HYB":"AUTO",10,PulseTheme::cyan,19,true);line(e.enabled.load()?"LIVE":"OFF",10,e.enabled.load()?Colour(0xff53d69a):PulseTheme::muted,18,true);return;
  }
  line("MONITOR",12,PulseTheme::text,24,true);r.removeFromTop(5);
  line("OUTPUT",10,PulseTheme::muted,16,true);
  line(dbText(peak)+" dBFS",18,peak>=.99f?Colours::orange:PulseTheme::cyan,25,true);
  horizontalMeter(g,r.removeFromTop(9).reduced(0,1),peak);line(lufs>-99?String(lufs,1)+" LUFS short-term":"LUFS — waiting for audio",11,PulseTheme::muted,19);
  r.removeFromTop(5);line("MIX BUSES",10,PulseTheme::muted,18,true);
  int rowHeight=jlimit(13,19,(r.getHeight()-58)/B);
  for(int i=0;i<B&&r.getHeight()>=rowHeight;++i){auto row=r.removeFromTop(rowHeight);auto label=row.removeFromLeft(juce::jlimit(65,100,row.getWidth()/2));g.setColour(PulseTheme::text);g.setFont(PulseTheme::font(11));g.drawFittedText(names[i],label,Justification::centredLeft,1,.8f);auto number=row.removeFromRight(32);horizontalMeter(g,row.reduced(4,5),e.busPeak[i].load());g.setColour(PulseTheme::muted);g.setFont(PulseTheme::font(9));g.drawFittedText(dbText(e.busPeak[i].load()),number,Justification::centredRight,1,.8f);}
  r.removeFromTop(5);line("AI ENGINEER",10,PulseTheme::muted,17,true);
  line(String(mixMode==1?"Manual":mixMode==2?"Assisted":mixMode==4?"Hybrid":"Autonomous")+(e.enabled.load()?" / Live":" / Live off"),12,PulseTheme::cyan,20,true);
  line("Feedback analysis unavailable",10,PulseTheme::muted,16);
 }
};
