#pragma once
#include "EnterpriseTheme.h"
#include "WorkspaceManager.h"
class NavigationRail : public juce::Component {
 std::array<juce::TextButton*,WorkspaceManager::count> items{};
 std::array<juce::TextButton*,4> actions{};
 int selected=0;
public:
 void bind(std::array<juce::TextButton,WorkspaceManager::count>& buttons,std::array<juce::TextButton*,4> extras){
  actions=extras;for(int i=0;i<WorkspaceManager::count;++i){items[i]=&buttons[i];items[i]->setButtonText(WorkspaceManager::names[i]);addAndMakeVisible(items[i]);}for(auto* b:actions)addAndMakeVisible(b);setSelected(0);
 }
 void setSelected(int i){selected=i;for(int j=0;j<WorkspaceManager::count;++j)if(items[j]){items[j]->setColour(juce::TextButton::buttonColourId,i==j?PulseTheme::active:PulseTheme::panel);items[j]->setColour(juce::TextButton::textColourOffId,i==j?PulseTheme::cyan:PulseTheme::text);}repaint();}
 void paint(juce::Graphics& g) override {
  g.fillAll(PulseTheme::panel);g.setColour(PulseTheme::border);g.drawVerticalLine(getWidth()-1,0,(float)getHeight());g.setColour(PulseTheme::muted);g.setFont(PulseTheme::font(10,true));g.drawText("WORKSPACES",12,4,getWidth()-24,19,juce::Justification::centredLeft);
  if(items[selected]){auto b=items[selected]->getBounds();g.setColour(PulseTheme::cyan);g.fillRoundedRectangle(2.f,(float)b.getY()+3,3.f,(float)b.getHeight()-6,1.5f);}
 }
 void resized() override {if(!items[0])return;auto r=getLocalBounds().reduced(8,5);r.removeFromTop(22);int height=juce::jlimit(22,35,(r.getHeight()-60)/WorkspaceManager::count);for(auto* b:items)b->setBounds(r.removeFromTop(height).reduced(1,2));r.removeFromTop(5);for(int i=0;i<2;++i){auto row=r.removeFromTop(25);actions[i*2]->setBounds(row.removeFromLeft(row.getWidth()/2).reduced(1));actions[i*2+1]->setBounds(row.reduced(1));}}
};
