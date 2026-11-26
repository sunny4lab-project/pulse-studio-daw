#pragma once
#include "EnterpriseTheme.h"
class StatusBar : public juce::Component {
 juce::Label *system=nullptr,*routing=nullptr;
public:
 void bind(juce::Label& s,juce::Label& r){system=&s;routing=&r;addAndMakeVisible(system);addAndMakeVisible(routing);PulseTheme::style(s,11,false,PulseTheme::muted);PulseTheme::style(r,11,false,PulseTheme::cyan);}
 void paint(juce::Graphics& g) override {g.fillAll(PulseTheme::header);g.setColour(PulseTheme::border);g.drawHorizontalLine(0,0,(float)getWidth());}
 void resized() override {if(!system)return;auto r=getLocalBounds().reduced(10,2);routing->setBounds(r.removeFromTop(18));system->setBounds(r);}
};
