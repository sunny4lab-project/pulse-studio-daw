#pragma once
#include "../MixerStrip.h"
#include "../UI/EnterpriseTheme.h"
// One scrollable list, one editor; no per-row widget allocations.
class InputPatchPage : public juce::Component, private juce::ListBoxModel {
 Engine& e; juce::OwnedArray<Strip>& strips; std::array<juce::String,B>& names;
 juce::ListBox list{"Input patch",this};juce::Label header,columns,selected,info,leftLabel,rightLabel,busLabel;
 juce::ComboBox left,right,bus;juce::StringArray inputs;int lastCount=-1;bool editing=false;
 int getNumRows() override{return strips.size();}
 Strip* current() const {int i=list.getSelectedRow();return juce::isPositiveAndBelow(i,strips.size())?strips[i]:nullptr;}
 juce::String inputName(int i,bool r=false) const {return i<0?(r?"Mono":"Unassigned"):i<inputs.size()?inputs[i]:"Slot "+juce::String(i+1)+" (inactive)";}
 void paintListBoxItem(int i,juce::Graphics& g,int w,int h,bool chosen) override {
  if(!juce::isPositiveAndBelow(i,strips.size()))return;auto& s=*strips[i];g.fillAll(chosen?PulseTheme::active:i%2?PulseTheme::panel:PulseTheme::bgPrimary);
  auto r=juce::Rectangle<int>(8,0,w-16,h);g.setFont(PulseTheme::font(12));g.setColour(PulseTheme::text);
  g.drawFittedText(s.name.getText(),r.removeFromLeft(r.getWidth()/4),juce::Justification::centredLeft,1);g.drawFittedText(inputName(s.c.source.load()),r.removeFromLeft(r.getWidth()/3),juce::Justification::centredLeft,1);g.drawFittedText(inputName(s.c.rightSource.load(),true),r.removeFromLeft(r.getWidth()/2),juce::Justification::centredLeft,1);
  g.setColour(PulseTheme::cyan);g.drawFittedText(names[juce::jlimit(0,B-1,s.c.bus.load())],r,juce::Justification::centredLeft,1);
 }
 void selectedRowsChanged(int) override{sync();}
 void sync(){
  auto* s=current();bool allowed=s&&e.canEditInserts();left.setEnabled(allowed);right.setEnabled(allowed);bus.setEnabled(allowed);editing=true;
  selected.setText(s?"PATCH: "+s->name.getText():"Select a mixer channel",juce::dontSendNotification);
  if(s){auto addMissing=[](juce::ComboBox& box,int i){if(i>=0&&box.indexOfItemId(i+2)<0)box.addItem("Slot "+juce::String(i+1)+" (inactive)",i+2);};addMissing(left,s->c.source.load());addMissing(right,s->c.rightSource.load());left.setSelectedId(s->c.source.load()+2,juce::dontSendNotification);right.setSelectedId(s->c.rightSource.load()+2,juce::dontSendNotification);bus.setSelectedId(s->c.bus.load()+1,juce::dontSendNotification);}
  else{left.setSelectedId(0,juce::dontSendNotification);right.setSelectedId(0,juce::dontSendNotification);bus.setSelectedId(0,juce::dontSendNotification);}editing=false;
 }
public:
 InputPatchPage(Engine& engine,juce::OwnedArray<Strip>& s,std::array<juce::String,B>& n):e(engine),strips(s),names(n){
  for(auto* c:std::array<juce::Component*,11>{&list,&header,&columns,&selected,&info,&leftLabel,&rightLabel,&busLabel,&left,&right,&bus})addAndMakeVisible(c);
  header.setText("HARDWARE INPUT PATCH",juce::dontSendNotification);PulseTheme::style(header,14,true);columns.setText("CHANNEL                         INPUT L                         INPUT R                         BUS",juce::dontSendNotification);PulseTheme::style(columns,11,false,PulseTheme::muted);PulseTheme::style(selected,12,true);PulseTheme::style(info,11,false,PulseTheme::muted);
  leftLabel.setText("Input L / mono",juce::dontSendNotification);rightLabel.setText("Input R / stereo",juce::dontSendNotification);busLabel.setText("Destination bus",juce::dontSendNotification);
  list.setRowHeight(28);list.setColour(juce::ListBox::backgroundColourId,PulseTheme::bgPrimary);
  left.onChange=[this]{if(editing)return;if(auto* s=current();s&&e.canEditInserts()){s->refreshInputs(inputs,true);s->input.setSelectedId(left.getSelectedId(),juce::sendNotificationSync);}sync();list.repaint();};
  right.onChange=[this]{if(editing)return;if(auto* s=current();s&&e.canEditInserts()){s->refreshInputs(inputs,true);s->rightInput.setSelectedId(right.getSelectedId(),juce::sendNotificationSync);}sync();list.repaint();};
  bus.onChange=[this]{if(editing)return;if(auto* s=current();s&&e.canEditInserts())s->group.setSelectedId(bus.getSelectedId(),juce::sendNotificationSync);sync();list.repaint();};
  refresh();
 }
 ~InputPatchPage()override{list.setModel(nullptr);}
 void refresh(){
  auto now=e.activeChannelNames(true);if(now!=inputs||left.getNumItems()==0){inputs=now;left.clear(juce::dontSendNotification);right.clear(juce::dontSendNotification);left.addItem("Unassigned",1);right.addItem("Mono",1);for(int i=0;i<inputs.size();++i){left.addItem(inputs[i],i+2);right.addItem(inputs[i],i+2);}}
  if(bus.getNumItems()!=B){bus.clear(juce::dontSendNotification);for(int b=0;b<B;++b)bus.addItem(names[b].isEmpty()?busName(b):names[b],b+1);}else for(int b=0;b<B;++b)if(names[b].isNotEmpty()&&bus.getItemText(b)!=names[b])bus.changeItemText(b+1,names[b]);
  if(lastCount!=strips.size()){lastCount=strips.size();list.updateContent();if(lastCount>0&&list.getSelectedRow()<0)list.selectRow(0);}
  info.setText(!e.canEditInserts()?"Turn LIVE off and stop playback/recording to change patches.":strips.isEmpty()?"Audio Devices → Create mixer inputs, or add tracks in Mixer.":"Select a channel and choose its hardware source and destination. Mono uses Input L only.",juce::dontSendNotification);
  if(!left.isPopupActive()&&!right.isPopupActive()&&!bus.isPopupActive())sync();list.repaint();
 }
 void resized()override{auto r=getLocalBounds().reduced(6);header.setBounds(r.removeFromTop(24));info.setBounds(r.removeFromTop(26));auto edit=r.removeFromBottom(84);selected.setBounds(edit.removeFromTop(24));auto labels=edit.removeFromTop(20);auto boxes=edit.removeFromTop(28);int n=3;for(auto* l:{&leftLabel,&rightLabel,&busLabel})l->setBounds(labels.removeFromLeft(labels.getWidth()/n--).reduced(2,0));n=3;for(auto* b:{&left,&right,&bus})b->setBounds(boxes.removeFromLeft(boxes.getWidth()/n--).reduced(2,1));columns.setBounds(r.removeFromTop(20));list.setBounds(r.reduced(1));}
};
