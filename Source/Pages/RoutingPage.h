#pragma once
#include "../EnterprisePanels.h"
#include "../UI/EnterpriseTheme.h"
class RoutingPage : public juce::Component {
 RoutingMatrix matrix;juce::Label note;
public:
 RoutingPage(juce::OwnedArray<Strip>& s,std::array<juce::String,B>& n):matrix(s,n){addAndMakeVisible(matrix);addAndMakeVisible(note);note.setText("32 inputs / 32 stereo buses | one destination per input",juce::dontSendNotification);PulseTheme::style(note,12,false,PulseTheme::muted);}
 void refresh(){matrix.repaint();}
 void resized() override {auto r=getLocalBounds();note.setBounds(r.removeFromBottom(26));matrix.setBounds(r);}
};
