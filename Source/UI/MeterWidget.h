#pragma once
#include "EnterpriseTheme.h"
class MeterWidget : public juce::Component {
 float peak=0;juce::String label{"Output"};
public:
 void setPeak(float p,juce::String name){p=std::max(0.f,p);if(std::abs(peak-p)<0.0001f&&label==name)return;peak=p;label=name;if(isShowing())repaint();}
 void paint(juce::Graphics& g) override {
  auto r=getLocalBounds().reduced(5);auto row=r.removeFromTop(20);g.setFont(PulseTheme::font(12));g.setColour(PulseTheme::text);g.drawText(label,row.removeFromLeft(row.getWidth()/2),juce::Justification::centredLeft);
  float db=peak>0?20.f*std::log10(peak):-100.f;g.drawText(juce::String(db,1)+" dBFS",row,juce::Justification::centredRight);
  auto bar=r.removeFromTop(15);int segments=40;float n=juce::jlimit(0.f,1.f,(db+60.f)/60.f);
  for(int i=0;i<segments;++i){auto cell=bar.removeFromLeft(bar.getWidth()/(segments-i)).reduced(1,1);auto c=i>=38?juce::Colour(0xffe25a61):i>=32?juce::Colour(0xfff59e0b):juce::Colour(0xff14e8c6);g.setColour(i<n*segments?c:PulseTheme::border);g.fillRoundedRectangle(cell.toFloat(),2);}
  g.setFont(PulseTheme::font(10));g.setColour(PulseTheme::muted);g.drawText("-60               -30               -12              0",r.removeFromTop(16),juce::Justification::centred);
 }
};
