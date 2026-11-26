#pragma once
#include "../MixerStrip.h"
#include "../UI/EnterpriseTheme.h"
class ChannelInspector : public Component {
 OwnedArray<Strip>& strips;ComboBox selected;Slider cutoff,threshold,ratio;Label hpLabel,thLabel,ratioLabel,heading;
 Strip* current() const {int i=selected.getSelectedId()-1;return i>=0&&i<strips.size()?strips[i]:nullptr;}
 void update(){if(auto* s=current()){cutoff.setValue(s->c.hp.load(),dontSendNotification);threshold.setValue(s->c.threshold.load(),dontSendNotification);ratio.setValue(s->c.ratio.load(),dontSendNotification);}}
public:
 explicit ChannelInspector(OwnedArray<Strip>& s):strips(s){
  for(int i=0;i<N;++i)selected.addItem("Channel "+String(i+1),i+1);selected.setSelectedId(1,dontSendNotification);
  for(auto* c:std::array<Component*,8>{&selected,&cutoff,&threshold,&ratio,&hpLabel,&thLabel,&ratioLabel,&heading})addAndMakeVisible(c);
  heading.setText("CHANNEL PROCESSING",dontSendNotification);PulseTheme::style(heading,12,true);
  hpLabel.setText("High-pass",dontSendNotification);thLabel.setText("Comp threshold",dontSendNotification);ratioLabel.setText("Comp ratio",dontSendNotification);
  for(auto* l:{&hpLabel,&thLabel,&ratioLabel})PulseTheme::style(*l,11,false,PulseTheme::muted);
  for(auto* k:{&cutoff,&threshold,&ratio}){k->setSliderStyle(Slider::LinearHorizontal);k->setTextBoxStyle(Slider::TextBoxRight,false,54,20);}
  cutoff.setRange(0,250,1);cutoff.setTextValueSuffix(" Hz");threshold.setRange(-60,0,.1);threshold.setTextValueSuffix(" dB");ratio.setRange(1,8,.1);ratio.setTextValueSuffix(":1");
  selected.onChange=[this]{update();};cutoff.onValueChange=[this]{if(auto* s=current())s->hp.setValue(cutoff.getValue(),sendNotificationSync);};threshold.onValueChange=[this]{if(auto* s=current())s->c.threshold=(float)threshold.getValue();};ratio.onValueChange=[this]{if(auto* s=current())s->ratio.setValue(ratio.getValue(),sendNotificationSync);};
 }
 void refresh(){for(int i=0;i<strips.size();++i){auto n=strips[i]->name.getText();if(selected.getItemText(i)!=n)selected.changeItemText(i+1,n);}if(!cutoff.isMouseButtonDown()&&!threshold.isMouseButtonDown()&&!ratio.isMouseButtonDown())update();}
 void paint(Graphics& g) override {g.setColour(PulseTheme::panel);g.fillRoundedRectangle(getLocalBounds().toFloat(),5);}
 void resized() override {auto r=getLocalBounds().reduced(6);auto row=r.removeFromTop(25);heading.setBounds(row.removeFromLeft(std::min(170,row.getWidth()/2)));selected.setBounds(row.removeFromLeft(std::min(180,row.getWidth())).reduced(2));for(auto pair:std::array<std::pair<Label*,Slider*>,3>{{{&hpLabel,&cutoff},{&thLabel,&threshold},{&ratioLabel,&ratio}}}){auto line=r.removeFromTop(25);pair.first->setBounds(line.removeFromLeft(110));pair.second->setBounds(line);}}
};
