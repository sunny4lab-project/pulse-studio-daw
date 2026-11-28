#pragma once
#include "EnterpriseTheme.h"
class EnterpriseCard : public juce::Component {
 juce::String heading,value,detail;juce::Colour colour=PulseTheme::cyan;
public:
 void set(juce::String h,juce::String v,juce::String d,juce::Colour c=PulseTheme::cyan){if(heading==h&&value==v&&detail==d&&colour==c)return;heading=h;value=v;detail=d;colour=c;repaint();}
 void paint(juce::Graphics& g) override {
  auto r=getLocalBounds().reduced(2);g.setColour(PulseTheme::panel);g.fillRoundedRectangle(r.toFloat(),5);g.setColour(PulseTheme::border);g.drawRoundedRectangle(r.toFloat(),5,1);
  r=r.reduced(9);g.setColour(PulseTheme::muted);g.setFont(PulseTheme::font(11,true));g.drawFittedText(heading,r.removeFromTop(19),juce::Justification::centredLeft,1);
  g.setColour(colour);g.setFont(PulseTheme::font(25,true));g.drawFittedText(value,r.removeFromTop(36),juce::Justification::centredLeft,1);
  g.setColour(PulseTheme::muted);g.setFont(PulseTheme::font(11));g.drawFittedText(detail,r,juce::Justification::centredLeft,2);
 }
};
