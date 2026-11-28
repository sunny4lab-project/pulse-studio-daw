#pragma once
#include "../MixerStrip.h"
#include "../UI/EnterpriseTheme.h"
class ChannelInspector : public Component {
 OwnedArray<Strip>& strips;ComboBox selected,page;std::array<Slider,3> knobs;std::array<Label,3> labels;Label heading,hint;TextButton bypass{"Tone bypass"};bool configuring=false;
 Strip* current() const {int i=selected.getSelectedId()-1;return i>=0&&i<strips.size()?strips[i]:nullptr;}
 int section() const {return page.getSelectedId();}
 void syncValues(){if(auto* s=current()){
  std::array<float,3> v{};if(section()==1)v={s->c.hp.load(),s->c.threshold.load(),s->c.ratio.load()};
  else if(section()==2)v={s->c.gateThreshold.load(),s->c.deessThreshold.load(),0};
  else if(section()<=10){int b=jlimit(0,pulse::eqBandCount-1,section()-3);v={s->c.eqFreq[b].load(),s->c.eqGain[b].load(),s->c.eqQ[b].load()};}
  else if(section()==11)v={s->c.trimDb.load(),s->c.polarity.load()?1.f:0.f,s->c.lowPass.load()};
  else if(section()==12)v={s->c.compAttack.load(),s->c.compRelease.load(),s->c.attackShape.load()*100};
  else v={s->c.saturation.load()*100,s->c.channelLimit.load()?1.f:0.f,s->c.limitDb.load()};
  for(int i=0;i<3;++i)if(!knobs[i].isMouseButtonDown())knobs[i].setValue(v[i],dontSendNotification);
  bypass.setToggleState(s->c.toneBypass.load(),dontSendNotification);
 }}
 void configure(){configuring=true;
  auto set=[&](int i,const String& name,double min,double max,double step,const String& suffix){labels[i].setText(name,dontSendNotification);knobs[i].setRange(min,max,step);knobs[i].setTextValueSuffix(suffix);knobs[i].setSkewFactor(1);knobs[i].setVisible(true);labels[i].setVisible(true);};
  if(section()==1){set(0,"High-pass",0,250,1," Hz");set(1,"Comp threshold",-60,0,.1," dB");set(2,"Comp ratio",1,8,.1,":1");hint.setText("Existing channel compressor and HPF.",dontSendNotification);}
  else if(section()==2){set(0,"Gate threshold",-80,-10,.1," dB");set(1,"De-ess threshold",-80,-6,.1," dB");knobs[2].setVisible(false);labels[2].setVisible(false);hint.setText("-80 dB = off; de-esser targets high band.",dontSendNotification);}
  else if(section()==11){set(0,"Input trim",-24,24,.1," dB");set(1,"Invert polarity",0,1,1,"");set(2,"Low-pass",0,20000,10," Hz");hint.setText("Polarity 1 = inverted; LPF 0 = off.",dontSendNotification);}
  else if(section()==12){set(0,"Comp attack",.1,100,.1," ms");set(1,"Comp release",10,1000,1," ms");set(2,"Transient attack",-80,100,1," %");hint.setText("Adjust compressor timing and transient emphasis.",dontSendNotification);}
  else if(section()==13){set(0,"Saturation",0,100,1," %");set(1,"Peak ceiling on",0,1,1,"");set(2,"Peak ceiling",-18,0,.1," dB");hint.setText("Soft saturation and sample-peak clip ceiling before fader.",dontSendNotification);}
  else {set(0,"Frequency",20,18000,1," Hz");knobs[0].setSkewFactorFromMidPoint(1000);set(1,"EQ gain",-12,12,.1," dB");set(2,"Q",.3,8,.1,"");hint.setText("Parametric bell filter; 0 dB = flat.",dontSendNotification);}
  syncValues();configuring=false;
 }
 void change(int i){if(configuring)return;if(auto* s=current()){float v=(float)knobs[i].getValue();if(section()==1){if(i==0)s->hp.setValue(v,sendNotificationSync);else if(i==1)s->c.threshold=v;else s->ratio.setValue(v,sendNotificationSync);}else if(section()==2){if(i==0)s->c.gateThreshold=v;else if(i==1)s->c.deessThreshold=v;}else if(section()<=10){int b=jlimit(0,pulse::eqBandCount-1,section()-3);if(i==0)s->c.eqFreq[b]=v;else if(i==1)s->c.eqGain[b]=v;else s->c.eqQ[b]=v;}else if(section()==11){if(i==0)s->c.trimDb=v;else if(i==1)s->c.polarity=v>.5f;else s->c.lowPass=v;}else if(section()==12){if(i==0)s->c.compAttack=v;else if(i==1)s->c.compRelease=v;else s->c.attackShape=v/100.f;}else{if(i==0)s->c.saturation=v/100.f;else if(i==1)s->c.channelLimit=v>.5f;else s->c.limitDb=v;}}}
public:
 explicit ChannelInspector(OwnedArray<Strip>& s):strips(s){
  
  for(auto name:{"HPF / Compressor","Gate / De-esser","EQ 1 (low)","EQ 2 (mid)","EQ 3 (high)","EQ 4","EQ 5","EQ 6","EQ 7","EQ 8","Input / Polarity / LPF","Dynamics / Transient","Saturation / Peak ceiling"})page.addItem(name,page.getNumItems()+1);page.setSelectedId(1,dontSendNotification);
  for(auto* c:std::array<Component*,5>{&selected,&page,&heading,&hint,&bypass})addAndMakeVisible(c);
  heading.setText("CHANNEL PROCESSING",dontSendNotification);PulseTheme::style(heading,12,true);PulseTheme::style(hint,10,false,PulseTheme::muted);
  bypass.setClickingTogglesState(true);bypass.setColour(TextButton::buttonOnColourId,Colour(0xff967322));bypass.onClick=[this]{if(auto* s=current())s->c.toneBypass=bypass.getToggleState();};
  for(int i=0;i<3;++i){addAndMakeVisible(knobs[i]);addAndMakeVisible(labels[i]);PulseTheme::style(labels[i],11,false,PulseTheme::muted);knobs[i].setSliderStyle(Slider::LinearHorizontal);knobs[i].setTextBoxStyle(Slider::TextBoxRight,false,62,20);knobs[i].onValueChange=[this,i]{change(i);};}
  selected.onChange=[this]{syncValues();};page.onChange=[this]{configure();};configure();
 }
 void refresh(){if(selected.getNumItems()!=strips.size()){int id=selected.getSelectedId();selected.clear(dontSendNotification);for(int i=0;i<strips.size();++i)selected.addItem(strips[i]->name.getText(),i+1);selected.setSelectedId(strips.isEmpty()?0:jlimit(1,strips.size(),id),dontSendNotification);}selected.setEnabled(!strips.isEmpty());for(auto& k:knobs)k.setEnabled(!strips.isEmpty());bypass.setEnabled(!strips.isEmpty());for(int i=0;i<strips.size();++i){auto n=strips[i]->name.getText();if(selected.getItemText(i)!=n)selected.changeItemText(i+1,n);}syncValues();}
 void paint(Graphics& g) override {g.setColour(PulseTheme::panel);g.fillRoundedRectangle(getLocalBounds().toFloat(),5);}
 void resized() override {auto r=getLocalBounds().reduced(6);auto row=r.removeFromTop(24);bypass.setBounds(row.removeFromRight(std::min(105,row.getWidth()/2)).reduced(1));heading.setBounds(row);row=r.removeFromTop(27);selected.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(1));page.setBounds(row.reduced(1));for(int i=0;i<3;++i){auto line=r.removeFromTop(25);labels[i].setBounds(line.removeFromLeft(std::min(100,line.getWidth()/2)));knobs[i].setBounds(line);}hint.setBounds(r.removeFromTop(20));}
};
