#pragma once
#include "EnterpriseTheme.h"
#include "WorkspaceManager.h"
class NavigationRail : public juce::Component {
 std::array<juce::TextButton*,WorkspaceManager::count> items{};
 std::array<juce::TextButton*,4> actions{};
 int selected=0;
 int displayMode=0; // 0 = full, 1 = compact icon rail, 2 = hidden
public:
 void setDisplayMode(int mode){displayMode=juce::jlimit(0,2,mode);for(int i=0;i<WorkspaceManager::count;++i)if(items[i]){
  items[i]->setButtonText(displayMode==1?juce::String(i+1):WorkspaceManager::names[i]);
  items[i]->setTooltip(WorkspaceManager::names[i]);}resized();repaint();}
 void bind(std::array<juce::TextButton,WorkspaceManager::count>& buttons,std::array<juce::TextButton*,4> extras){
  actions=extras;for(int i=0;i<WorkspaceManager::count;++i){items[i]=&buttons[i];items[i]->setButtonText(WorkspaceManager::names[i]);addAndMakeVisible(items[i]);}for(auto* b:actions)addAndMakeVisible(b);setSelected(0);
 }
 void setSelected(int i){selected=i;for(int j=0;j<WorkspaceManager::count;++j)if(items[j]){items[j]->setColour(juce::TextButton::buttonColourId,i==j?PulseTheme::active:PulseTheme::panel);items[j]->setColour(juce::TextButton::textColourOffId,i==j?PulseTheme::cyan:PulseTheme::text);}repaint();}
 void paint(juce::Graphics& g) override {
  g.fillAll(PulseTheme::panel);g.setColour(PulseTheme::border);g.drawVerticalLine(getWidth()-1,0,(float)getHeight());g.setColour(PulseTheme::muted);g.setFont(PulseTheme::font(10,true));if(displayMode==0)g.drawText("WORKSPACES",12,4,getWidth()-24,19,juce::Justification::centredLeft);
  if(items[selected]){auto b=items[selected]->getBounds();g.setColour(PulseTheme::cyan);g.fillRoundedRectangle(2.f,(float)b.getY()+3,3.f,(float)b.getHeight()-6,1.5f);}
 }
 void resized() override {if(!items[0])return;auto r=getLocalBounds().reduced(displayMode==1?3:8,5);r.removeFromTop(displayMode==1?5:22);int height=juce::jlimit(22,35,(r.getHeight()-60)/WorkspaceManager::count);for(auto* b:items)b->setBounds(r.removeFromTop(height).reduced(1,2));r.removeFromTop(5);for(int i=0;i<4;++i)actions[i]->setBounds(displayMode==1?juce::Rectangle<int>{0,0,0,0}:r.removeFromTop(25).reduced(1));}
};
