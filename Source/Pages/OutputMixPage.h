#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseTheme.h"
class OutputMixPage : public Component {
 Engine& e; std::array<String,B>& names;
 ComboBox bus; Label title,note; std::array<Label,pulse::auxMixCount> heading,status;
 std::array<Slider,pulse::auxMixCount> send,level;
 std::array<TextButton,pulse::auxMixCount> arm;
 TextButton disarm{"Disarm all"};
 int selectedBus() const {return jlimit(0,B-1,bus.getSelectedId()-1);}
 void syncSends(){for(int a=0;a<pulse::auxMixCount;++a)send[a].setValue(Decibels::gainToDecibels(e.getAuxSend(selectedBus(),a),-60.f),dontSendNotification);}
public:
 OutputMixPage(Engine& engine,std::array<String,B>& n):e(engine),names(n){
  for(auto* c:std::array<Component*,4>{&title,&note,&bus,&disarm})addAndMakeVisible(c);
  title.setText("AUXILIARY OUTPUT MIXES",dontSendNotification);PulseTheme::style(title,14,true);
  note.setText("Post-bus sends. Master remains pair 1. Verify callback slots against the device. Aux excludes playback and master FX.",dontSendNotification);PulseTheme::style(note,11,false,PulseTheme::muted);note.setJustificationType(Justification::topLeft);
  for(int b=0;b<B;++b)bus.addItem(names[b].isEmpty()?busName(b):names[b],b+1);bus.setSelectedId(1,dontSendNotification);bus.onChange=[this]{syncSends();};
  disarm.onClick=[this]{e.disarmAux();refresh();};
  for(int a=0;a<pulse::auxMixCount;++a){
   for(auto* c:std::array<Component*,5>{&heading[a],&status[a],&send[a],&level[a],&arm[a]})addAndMakeVisible(c);
   heading[a].setText("AUX "+String(a+1)+" | slots "+String(2*a+3)+"/"+String(2*a+4),dontSendNotification);PulseTheme::style(heading[a],12,true);
   PulseTheme::style(status[a],11,false,PulseTheme::muted);
   for(auto* s:{&send[a],&level[a]}){s->setSliderStyle(Slider::LinearHorizontal);s->setRange(-60,0,.1);s->setTextBoxStyle(Slider::TextBoxRight,false,57,20);s->setTextValueSuffix(" dB");}
   send[a].setTooltip("Selected bus send level");level[a].setTooltip("Aux master level");
   send[a].onValueChange=[this,a]{e.auxSend[selectedBus()*pulse::auxMixCount+a]=Decibels::decibelsToGain((float)send[a].getValue(),-60.f);};
   level[a].onValueChange=[this,a]{e.auxGain[a]=Decibels::decibelsToGain((float)level[a].getValue(),-60.f);};
   arm[a].setColour(TextButton::buttonOnColourId,Colour(0xff17665b));arm[a].onClick=[this,a]{if(e.auxArmed[a].load()){e.auxArmed[a]=false;}else if(!e.enabled.load()&&e.auxAvailable[a].load()){e.auxArmed[a]=true;}refresh();};
  }
  syncSends();refresh();
 }
 void refresh(){for(int b=0;b<B;++b){auto n=names[b].isEmpty()?busName(b):names[b];if(bus.getItemText(b)!=n)bus.changeItemText(b+1,n);}
  bool live=e.enabled.load();bus.setEnabled(!live);
  for(int a=0;a<pulse::auxMixCount;++a){bool armed=e.auxArmed[a].load(),available=e.auxAvailable[a].load();
   arm[a].setButtonText(armed?"Disarm":available?"Arm":"Unavailable");arm[a].setToggleState(armed,dontSendNotification);arm[a].setEnabled(armed||(!live&&available));
   send[a].setEnabled(!live&&!armed);level[a].setEnabled(!live&&!armed);
   if(!send[a].isMouseButtonDown())send[a].setValue(Decibels::gainToDecibels(e.getAuxSend(selectedBus(),a),-60.f),dontSendNotification);
   if(!level[a].isMouseButtonDown())level[a].setValue(Decibels::gainToDecibels(e.auxGain[a].load(),-60.f),dontSendNotification);
   status[a].setText(!available?"No callback pair":!armed?"Disarmed":!live?"Armed / live off":String(Decibels::gainToDecibels(e.auxPeak[a].load(),-100.f),1)+" dBFS sample peak",dontSendNotification);
  }repaint();}
 void resized() override {auto r=getLocalBounds().reduced(8);auto row=r.removeFromTop(26);disarm.setBounds(row.removeFromRight(100).reduced(2));title.setBounds(row);note.setBounds(r.removeFromTop(42));bus.setBounds(r.removeFromTop(26).reduced(2));for(int a=0;a<pulse::auxMixCount;++a){auto q=r.removeFromTop(88).reduced(2);auto h=q.removeFromTop(24);arm[a].setBounds(h.removeFromRight(96));heading[a].setBounds(h);auto s=q.removeFromTop(24);send[a].setBounds(s.removeFromLeft(s.getWidth()/2).reduced(2));level[a].setBounds(s.reduced(2));status[a].setBounds(q.removeFromTop(22));}}
 void paint(Graphics& g) override {g.fillAll(PulseTheme::panel);}
};
