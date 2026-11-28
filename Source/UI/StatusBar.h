#pragma once
#include "EnterpriseTheme.h"
class StatusBar : public juce::Component {
 juce::Label *system=nullptr,*routing=nullptr;
public:
 static int preferredHeight(int width){return width>=1100?24:36;}
 void bind(juce::Label& s,juce::Label& r){system=&s;routing=&r;addAndMakeVisible(system);addAndMakeVisible(routing);PulseTheme::style(s,11,false,PulseTheme::muted);PulseTheme::style(r,11,false,PulseTheme::cyan);s.setMinimumHorizontalScale(.75f);r.setMinimumHorizontalScale(.8f);}
 void paint(juce::Graphics& g) override {g.fillAll(PulseTheme::bgPrimary);g.setColour(PulseTheme::border);g.drawHorizontalLine(0,0,(float)getWidth());}
 void resized() override {if(!system)return;auto r=getLocalBounds().reduced(8,2);if(getWidth()>=1100){system->setBounds(r.removeFromLeft(r.getWidth()*65/100));routing->setBounds(r);}else{routing->setBounds(r.removeFromTop(16));system->setBounds(r);}}
};
