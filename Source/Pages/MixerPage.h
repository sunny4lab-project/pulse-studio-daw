#pragma once
#include "../UI/EnterpriseTheme.h"
// Owns the mixer workspace surface; existing controls are reparented, not duplicated.
class MixerPage : public juce::Component {
public:
 void attach(juce::Component& c){addAndMakeVisible(c);}
 void paint(juce::Graphics& g) override {g.setColour(PulseTheme::panel);g.fillRoundedRectangle(getLocalBounds().toFloat(),5);g.setColour(PulseTheme::border);g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f),5,1);}
};
