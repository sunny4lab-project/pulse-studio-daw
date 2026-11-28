#pragma once
#include "../EnterprisePanels.h"
#include "../UI/EnterpriseTheme.h"
#include "OutputMixPage.h"
#include "InputPatchPage.h"
class RoutingPage : public juce::Component {
 InputPatchPage patch;RoutingMatrix matrix;OutputMixPage outputs;juce::Label note;
 juce::TextButton patchTab{"Input patch"},busTab{"Bus routing"},outputTab{"Output patch"};
 int page=0;
 void update(){patch.setVisible(page==0);matrix.setVisible(page==1);outputs.setVisible(page==2);patchTab.setToggleState(page==0,juce::dontSendNotification);busTab.setToggleState(page==1,juce::dontSendNotification);outputTab.setToggleState(page==2,juce::dontSendNotification);note.setText(page==0?"Hardware input → mixer channel → bus":page==1?"Channel → one of ten stereo buses":"Master / auxiliary mix → hardware stereo pair",juce::dontSendNotification);resized();refresh();}
public:
 RoutingPage(Engine& e,juce::OwnedArray<Strip>& s,std::array<juce::String,B>& n):patch(e,s,n),matrix(s,n),outputs(e,n){for(auto* c:std::array<juce::Component*,7>{&patch,&matrix,&outputs,&note,&patchTab,&busTab,&outputTab})addAndMakeVisible(c);PulseTheme::style(note,11,false,PulseTheme::muted);patchTab.onClick=[this]{page=0;update();};busTab.onClick=[this]{page=1;update();};outputTab.onClick=[this]{page=2;update();};update();}
 void refresh(){if(page==0)patch.refresh();else if(page==2)outputs.refresh();else matrix.repaint();}
 void resized() override {auto r=getLocalBounds();auto row=r.removeFromTop(30);int n=3;for(auto* b:{&patchTab,&busTab,&outputTab})b->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2));note.setBounds(r.removeFromBottom(22));if(page==0)patch.setBounds(r);else if(page==1)matrix.setBounds(r);else outputs.setBounds(r);}
};
