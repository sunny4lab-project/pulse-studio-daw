#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseCard.h"
class AnalyticsPage : public juce::Component {
 Engine& e;EnterpriseCard cpu,errors;std::array<float,120> history{};int next=0,count=0,ticks=0;
public:
 explicit AnalyticsPage(Engine& engine):e(engine){addAndMakeVisible(cpu);addAndMakeVisible(errors);}
 void refresh(){if(ticks++%15!=0)return;auto* d=e.devices.getCurrentAudioDevice();bool ready=d&&d->isPlaying();float usage=(float)e.devices.getCpuUsage()*100.f;history[next]=ready?usage:-1.f;next=(next+1)%120;count=std::min(120,count+1);cpu.set("AUDIO CPU",ready?juce::String(usage,1)+"%":"Offline","Callback load");errors.set("RECORDING ERRORS",juce::String((int)e.missedWrites.load()),"Failed writer submissions");repaint();}
 void resized() override {auto r=getLocalBounds().reduced(4).removeFromTop(104);cpu.setBounds(r.removeFromLeft(r.getWidth()/2));errors.setBounds(r);}
 void paint(juce::Graphics& g) override {auto r=getLocalBounds().reduced(7).withTrimmedTop(112);g.setColour(PulseTheme::panel);g.fillRoundedRectangle(r.toFloat(),6);g.setColour(PulseTheme::text);g.setFont(PulseTheme::font(12,true));g.drawText("AUDIO CPU HISTORY / LAST 120 SECONDS",r.removeFromTop(24),juce::Justification::centredLeft);r=r.reduced(10);for(int i=0;i<=4;++i){g.setColour(PulseTheme::border);int y=r.getY()+r.getHeight()*i/4;g.drawHorizontalLine(y,(float)r.getX(),(float)r.getRight());}juce::Path p;bool started=false;for(int i=0;i<count;++i){float v=history[(next-count+i+120)%120];if(v<0){started=false;continue;}float x=r.getX()+r.getWidth()*(120-count+i)/119.f,y=r.getBottom()-r.getHeight()*juce::jlimit(0.f,1.f,v/100.f);if(!started)p.startNewSubPath(x,y);else p.lineTo(x,y);started=true;}g.setColour(PulseTheme::cyan);g.strokePath(p,juce::PathStrokeType(2.f));}
};
