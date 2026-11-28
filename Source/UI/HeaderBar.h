#pragma once
#include "EnterpriseTheme.h"
#include <array>
class HeaderBar : public juce::Component {
 juce::Label *title=nullptr,*subtitle=nullptr,*metrics=nullptr,*time=nullptr;
 std::array<juce::TextButton*,7> controls{};
 juce::TextButton sidebarToggle{"≡"},monitorToggle{"Monitor"},filesToggle{"Files"};
public:
 std::function<void()> onSidebarToggle,onMonitorToggle,onFilesToggle;
 void setMonitorMode(int mode){monitorToggle.setButtonText(mode==0?"Meters >":mode==1?"Monitor":"Meters off");monitorToggle.setTooltip("Cycle monitor: expanded, compact, hidden");}
 void setSidebarMode(int mode){sidebarToggle.setButtonText(mode==0?"≡":mode==1?"▸":"☰");sidebarToggle.setTooltip(mode==0?"Collapse sidebar":mode==1?"Hide sidebar":"Show sidebar");}
 void bind(juce::Label& t,juce::Label& sub,juce::Label& m,juce::Label& clock,std::array<juce::TextButton*,7> buttons){
  title=&t;subtitle=&sub;metrics=&m;time=&clock;controls=buttons;
  for(auto* l:{title,subtitle,metrics,time})addAndMakeVisible(l);
  for(auto* b:controls)addAndMakeVisible(b);
  addAndMakeVisible(filesToggle);filesToggle.onClick=[this]{if(onFilesToggle)onFilesToggle();};addAndMakeVisible(monitorToggle);monitorToggle.onClick=[this]{if(onMonitorToggle)onMonitorToggle();};addAndMakeVisible(sidebarToggle);sidebarToggle.onClick=[this]{if(onSidebarToggle)onSidebarToggle();};setSidebarMode(0);
  PulseTheme::style(t,13,true);PulseTheme::style(sub,10,false,PulseTheme::muted);PulseTheme::style(m,11,false,PulseTheme::cyan);PulseTheme::style(clock,13,true);
 }
 static int preferredHeight(int width){return width<1080?80:57;}
 void paint(juce::Graphics& g) override {
  g.fillAll(PulseTheme::header);g.setColour(PulseTheme::border);g.drawHorizontalLine(getHeight()-1,0,(float)getWidth());
  g.setColour(PulseTheme::active);g.fillRoundedRectangle(10,9,34,34,7);
  juce::Path p;p.startNewSubPath(15,26);p.lineTo(21,26);p.lineTo(25,16);p.lineTo(29,34);p.lineTo(33,23);p.lineTo(39,23);g.setColour(PulseTheme::cyan);g.strokePath(p,juce::PathStrokeType(1.8f));
 }
 void resized() override {
  if(!title)return;auto r=getLocalBounds().reduced(10,5);auto brand=r.removeFromTop(getWidth()<1080?36:r.getHeight());if(getWidth()>=1080){r=brand;brand=r.removeFromLeft(247);}auto metricsArea=getWidth()<1080?brand.removeFromRight(178):r.removeFromRight(155);
  metrics->setBounds(metricsArea);sidebarToggle.setBounds(brand.getX()+40,brand.getY()+5,32,27);brand.removeFromLeft(78);title->setBounds(brand.removeFromTop(23));subtitle->setBounds(brand);
  time->setBounds(r.removeFromRight(72).reduced(2));monitorToggle.setBounds(r.removeFromRight(76).reduced(2,3));filesToggle.setBounds(r.removeFromRight(45).reduced(2,3));int n=7;for(auto* b:controls)b->setBounds(r.removeFromLeft(std::min(92,r.getWidth()/n--)).reduced(2,3));
 }
};
