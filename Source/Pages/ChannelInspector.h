#pragma once
#include "../MixerStrip.h"
#include "../UI/EnterpriseTheme.h"
class ChannelInspector : public Component {
 OwnedArray<Strip>& strips;ComboBox selected,page;std::array<Slider,3> knobs;std::array<Label,3> labels;Label heading,hint;TextButton bypass{"Tone bypass"};
 Strip* current() const {int i=selected.getSelectedId()-1;return i>=0&&i<strips.size()?strips[i]:nullptr;}
 int section() const {return page.getSelectedId();}
 void syncValues(){if(auto* s=current()){
  std::array<float,3> v{};if(section()==1)v={s->c.hp.load(),s->c.threshold.load(),s->c.ratio.load()};
  else if(section()==2)v={s->c.gateThreshold.load(),s->c.deessThreshold.load(),0};
  else {int b=jlimit(0,2,section()-3);v={s->c.eqFreq[b].load(),s->c.eqGain[b].load(),s->c.eqQ[b].load()};}
  for(int i=0;i<3;++i)if(!knobs[i].isMouseButtonDown())knobs[i].setValue(v[i],dontSendNotification);
  bypass.setToggleState(s->c.toneBypass.load(),dontSendNotification);
 }}
 void configure(){
  auto set=[&](int i,const String& name,double min,double max,double step,const String& suffix){labels[i].setText(name,dontSendNotification);knobs[i].setRange(min,max,step);knobs[i].setTextValueSuffix(suffix);knobs[i].setSkewFactor(1);knobs[i].setVisible(true);labels[i].setVisible(true);};
  if(section()==1){set(0,"High-pass",0,250,1," Hz");set(1,"Comp threshold",-60,0,.1," dB");set(2,"Comp ratio",1,8,.1,":1");hint.setText("Existing channel compressor and HPF.",dontSendNotification);}
  else if(section()==2){set(0,"Gate threshold",-80,-10,.1," dB");set(1,"De-ess threshold",-80,-6,.1," dB");knobs[2].setVisible(false);labels[2].setVisible(false);hint.setText("-80 dB = off; de-esser targets high band.",dontSendNotification);}
  else {set(0,"Frequency",20,18000,1," Hz");knobs[0].setSkewFactorFromMidPoint(1000);set(1,"EQ gain",-12,12,.1," dB");set(2,"Q",.3,8,.1,"");hint.setText("Parametric bell filter; 0 dB = flat.",dontSendNotification);}
  syncValues();
 }
 void change(int i){if(auto* s=current()){float v=(float)knobs[i].getValue();if(section()==1){if(i==0)s->hp.setValue(v,sendNotificationSync);else if(i==1)s->c.threshold=v;else s->ratio.setValue(v,sendNotificationSync);}else if(section()==2){if(i==0)s->c.gateThreshold=v;else if(i==1)s->c.deessThreshold=v;}else{int b=jlimit(0,2,section()-3);if(i==0)s->c.eqFreq[b]=v;else if(i==1)s->c.eqGain[b]=v;else s->c.eqQ[b]=v;}}}
public:
 explicit ChannelInspector(OwnedArray<Strip>& s):strips(s){
  for(int i=0;i<N;++i)selected.addItem("Channel "+String(i+1),i+1);selected.setSelectedId(1,dontSendNotification);
  for(auto name:{"HPF / Compressor","Gate / De-esser","Low EQ","Mid EQ","High EQ"})page.addItem(name,page.getNumItems()+1);page.setSelectedId(1,dontSendNotification);
  for(auto* c:std::array<Component*,5>{&selected,&page,&heading,&hint,&bypass})addAndMakeVisible(c);
  heading.setText("CHANNEL PROCESSING",dontSendNotification);PulseTheme::style(heading,12,true);PulseTheme::style(hint,10,false,PulseTheme::muted);
  bypass.setClickingTogglesState(true);bypass.setColour(TextButton::buttonOnColourId,Colour(0xff967322));bypass.onClick=[this]{if(auto* s=current())s->c.toneBypass=bypass.getToggleState();};
  for(int i=0;i<3;++i){addAndMakeVisible(knobs[i]);addAndMakeVisible(labels[i]);PulseTheme::style(labels[i],11,false,PulseTheme::muted);knobs[i].setSliderStyle(Slider::LinearHorizontal);knobs[i].setTextBoxStyle(Slider::TextBoxRight,false,62,20);knobs[i].onValueChange=[this,i]{change(i);};}
  selected.onChange=[this]{syncValues();};page.onChange=[this]{configure();};configure();
 }
 void refresh(){for(int i=0;i<strips.size();++i){auto n=strips[i]->name.getText();if(selected.getItemText(i)!=n)selected.changeItemText(i+1,n);}syncValues();}
 void paint(Graphics& g) override {g.setColour(PulseTheme::panel);g.fillRoundedRectangle(getLocalBounds().toFloat(),5);}
 void resized() override {auto r=getLocalBounds().reduced(6);auto row=r.removeFromTop(24);bypass.setBounds(row.removeFromRight(std::min(105,row.getWidth()/2)).reduced(1));heading.setBounds(row);row=r.removeFromTop(27);selected.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(1));page.setBounds(row.reduced(1));for(int i=0;i<3;++i){auto line=r.removeFromTop(25);labels[i].setBounds(line.removeFromLeft(std::min(100,line.getWidth()/2)));knobs[i].setBounds(line);}hint.setBounds(r.removeFromTop(20));}
};
