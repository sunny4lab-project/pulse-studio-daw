#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseCard.h"
class AnalyticsPage : public juce::Component {
 Engine& e;EnterpriseCard cpu,callbackPeak,deadlineMisses,errors;std::array<float,120> history{};int next=0,count=0,ticks=0;
public:
 explicit AnalyticsPage(Engine& engine):e(engine){for(auto* card:{&cpu,&callbackPeak,&deadlineMisses,&errors})addAndMakeVisible(*card);}
 void refresh(){
  if(ticks++%15!=0)return;
  auto* d=e.devices.getCurrentAudioDevice();bool ready=d&&d->isPlaying();float usage=(float)e.devices.getCpuUsage()*100.f;
  history[next]=ready?usage:-1.f;next=(next+1)%120;count=std::min(120,count+1);
  cpu.set("AUDIO CPU",ready?juce::String(usage,1)+"%":"Offline","Driver-reported average");
  auto peak=e.callbackDeadline.peakPermille.load(std::memory_order_relaxed);
  auto missed=e.callbackDeadline.missedDeadlines.load(std::memory_order_relaxed);
  callbackPeak.set("CALLBACK PEAK",ready?juce::String(peak/10.f,1)+"%":"Offline","Worst block since device start",peak>=900?juce::Colours::orange:PulseTheme::cyan);
  deadlineMisses.set("DEADLINE WARNINGS",juce::String(missed),"Estimated callback overruns",missed?juce::Colours::orange:PulseTheme::cyan);
  errors.set("RECORDING ERRORS",juce::String(e.missedWrites.load()),"Failed writer submissions");repaint();
 }
 int cardsHeight()const{return getWidth()>=900?104:198;}
 void resized() override {
  auto r=getLocalBounds().reduced(4).removeFromTop(cardsHeight());
  if(getWidth()>=900){int w=r.getWidth()/4;cpu.setBounds(r.removeFromLeft(w));callbackPeak.setBounds(r.removeFromLeft(w));deadlineMisses.setBounds(r.removeFromLeft(w));errors.setBounds(r);}
  else {auto top=r.removeFromTop(r.getHeight()/2);cpu.setBounds(top.removeFromLeft(top.getWidth()/2));callbackPeak.setBounds(top);deadlineMisses.setBounds(r.removeFromLeft(r.getWidth()/2));errors.setBounds(r);}
 }
 void paint(juce::Graphics& g) override {auto r=getLocalBounds().reduced(7).withTrimmedTop(cardsHeight()+8);g.setColour(PulseTheme::panel);g.fillRoundedRectangle(r.toFloat(),6);g.setColour(PulseTheme::text);g.setFont(PulseTheme::font(12,true));g.drawText("AUDIO CPU HISTORY / LAST 120 SECONDS",r.removeFromTop(24),juce::Justification::centredLeft);r=r.reduced(10);for(int i=0;i<=4;++i){g.setColour(PulseTheme::border);int y=r.getY()+r.getHeight()*i/4;g.drawHorizontalLine(y,(float)r.getX(),(float)r.getRight());}juce::Path p;bool started=false;for(int i=0;i<count;++i){float v=history[(next-count+i+120)%120];if(v<0){started=false;continue;}float x=r.getX()+r.getWidth()*(120-count+i)/119.f,y=r.getBottom()-r.getHeight()*juce::jlimit(0.f,1.f,v/100.f);if(!started)p.startNewSubPath(x,y);else p.lineTo(x,y);started=true;}g.setColour(PulseTheme::cyan);g.strokePath(p,juce::PathStrokeType(2.f));}
};
