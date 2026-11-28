#pragma once
#include "../EnterprisePanels.h"
#include "../UI/EnterpriseTheme.h"
#include "OutputMixPage.h"
class RoutingPage : public juce::Component {
 RoutingMatrix matrix;OutputMixPage outputs;juce::Label note;
 juce::TextButton inputTab{"Channel to bus"},outputTab{"Aux output mixes"};
 bool showOutputs=false;
 void update(){matrix.setVisible(!showOutputs);outputs.setVisible(showOutputs);inputTab.setToggleState(!showOutputs,juce::dontSendNotification);outputTab.setToggleState(showOutputs,juce::dontSendNotification);note.setText(showOutputs?"Select master and auxiliary stereo output pairs; arm with live inputs off.":"Dynamic channels / 10 stereo buses | one destination per channel",juce::dontSendNotification);resized();}
public:
 RoutingPage(Engine& e,juce::OwnedArray<Strip>& s,std::array<juce::String,B>& n):matrix(s,n),outputs(e,n){for(auto* c:std::array<juce::Component*,5>{&matrix,&outputs,&note,&inputTab,&outputTab})addAndMakeVisible(c);PulseTheme::style(note,11,false,PulseTheme::muted);inputTab.onClick=[this]{showOutputs=false;update();};outputTab.onClick=[this]{showOutputs=true;update();outputs.refresh();};update();}
 void refresh(){if(showOutputs)outputs.refresh();else matrix.repaint();}
 void resized() override {auto r=getLocalBounds();auto row=r.removeFromTop(30);inputTab.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));outputTab.setBounds(row.reduced(2));note.setBounds(r.removeFromBottom(25));if(showOutputs)outputs.setBounds(r);else matrix.setBounds(r);}
};
