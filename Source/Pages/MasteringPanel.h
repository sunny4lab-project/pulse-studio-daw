#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseTheme.h"
class MasteringPanel : public Component {
 Engine& e;std::array<String,B>& names;ComboBox bus;TextButton fx{"Enable sends"},limit{"Enable limiter"},reset{"Reset loudness"},apply{"Apply level suggestion"};
 std::array<Slider,8> controls;std::array<Label,8> labels;Label title,metrics,advice;
 bool syncing=false;float suggested=0;
 int selectedBus()const{return jlimit(0,B-1,bus.getSelectedId()-1);}
 static float db(float gain){return Decibels::gainToDecibels(gain,-60.f);}
 void sync(){syncing=true;std::array<float,8> v={db(e.sends.reverbSend[selectedBus()].load()),db(e.sends.delaySend[selectedBus()].load()),db(e.sends.reverbReturn.load()),db(e.sends.delayReturn.load()),e.sends.delayMs.load(),e.sends.feedback.load()*100,e.limiterCeiling.load(),e.limiterRelease.load()};for(int i=0;i<8;++i)if(!controls[i].isMouseButtonDown())controls[i].setValue(v[i],dontSendNotification);fx.setToggleState(e.sends.enabled.load(),dontSendNotification);limit.setToggleState(e.limiterEnabled.load(),dontSendNotification);syncing=false;}
 void change(int i){if(syncing)return;float v=(float)controls[i].getValue();switch(i){case 0:e.sends.reverbSend[selectedBus()]=Decibels::decibelsToGain(v,-60.f);break;case 1:e.sends.delaySend[selectedBus()]=Decibels::decibelsToGain(v,-60.f);break;case 2:e.sends.reverbReturn=Decibels::decibelsToGain(v,-60.f);break;case 3:e.sends.delayReturn=Decibels::decibelsToGain(v,-60.f);break;case 4:e.sends.delayMs=v;break;case 5:e.sends.feedback=v/100;break;case 6:e.limiterCeiling=v;break;case 7:e.limiterRelease=v;break;}}
public:
 std::function<void(float)> onApplyMaster;
 MasteringPanel(Engine& engine,std::array<String,B>& n):e(engine),names(n){
  for(auto* c:std::array<Component*,8>{&bus,&fx,&limit,&reset,&title,&metrics,&advice,&apply})addAndMakeVisible(c);
  title.setText("MASTERING / SHARED SEND EFFECTS",dontSendNotification);PulseTheme::style(title,16,true);PulseTheme::style(metrics,13,true,PulseTheme::cyan);PulseTheme::style(advice,11,false,PulseTheme::muted);
  for(int b=0;b<B;++b)bus.addItem(busName(b),b+1);bus.setSelectedId(1,dontSendNotification);bus.onChange=[this]{sync();};
  fx.setClickingTogglesState(true);limit.setClickingTogglesState(true);fx.onClick=[this]{e.sends.enabled=fx.getToggleState();};limit.onClick=[this]{e.limiterEnabled=limit.getToggleState();};reset.onClick=[this]{e.resetLoudness=true;};apply.onClick=[this]{if(onApplyMaster&&std::abs(suggested)>=.1f){onApplyMaster(suggested);e.resetLoudness=true;suggested=0;apply.setEnabled(false);}};
  const char* text[]={"Bus reverb send","Bus delay send","Reverb return","Delay return","Delay time","Feedback","Limiter ceiling","Limiter release"};
  for(int i=0;i<8;++i){addAndMakeVisible(controls[i]);addAndMakeVisible(labels[i]);labels[i].setText(text[i],dontSendNotification);PulseTheme::style(labels[i],12,false,PulseTheme::muted);auto& k=controls[i];k.setSliderStyle(Slider::LinearHorizontal);k.setTextBoxStyle(Slider::TextBoxRight,false,65,20);k.setRange(i<4?-60:i==4?20:i==5?0:i==6?-12:10,i<4?0:i==4?1000:i==5?65:i==6?0:500,i==4||i==7?1:.1);k.setTextValueSuffix(i==4||i==7?" ms":i==5?" %":" dB");k.onValueChange=[this,i]{change(i);};}sync();
 }
 void refresh(){for(int i=0;i<B;++i){auto n=names[i].isEmpty()?busName(i):names[i];if(bus.getItemText(i)!=n)bus.changeItemText(i+1,n);}sync();auto fmt=[](float v){return v<=-99?String("--"):String(v,1);};float i=e.integratedLufs.load();metrics.setText("I "+fmt(i)+" LUFS | M "+fmt(e.momentaryLufs.load())+" | S "+fmt(e.shortLufs.load())+" | GR "+String(e.limiterReduction.load(),1)+" dB",dontSendNotification);float held=e.estimatedTruePeak.load();float headroom=-1.f-Decibels::gainToDecibels(held,-100.f);suggested=i<=-99?0.f:jlimit(-3.f,3.f,-18.f-i);if(suggested>0)suggested=std::max(0.f,std::min(suggested,headroom));apply.setEnabled(i>-99&&e.shortLufs.load()>-99&&std::abs(suggested)>=.1f);apply.setButtonText("Apply "+String(suggested,1)+" dB (master)");advice.setText(i<=-99?"Waiting for measured audio.":i>-16?"Level above -18 LUFS reference; consider reducing master gain.":i<-20?"Below -18 LUFS reference; check headroom before raising gain.":"Near -18 LUFS reference. Check speech and music balance.",dontSendNotification);}
 void paint(Graphics& g) override{g.fillAll(PulseTheme::panel);}
 void resized() override{auto r=getLocalBounds().reduced(8);title.setBounds(r.removeFromTop(28));auto row=r.removeFromTop(28);fx.setBounds(row.removeFromLeft(row.getWidth()/3).reduced(2));limit.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));reset.setBounds(row.reduced(2));bus.setBounds(r.removeFromTop(28));for(int i=0;i<8;++i){auto line=r.removeFromTop(25);labels[i].setBounds(line.removeFromLeft(std::min(140,line.getWidth()/2)));controls[i].setBounds(line);}metrics.setBounds(r.removeFromTop(28));advice.setBounds(r.removeFromTop(26));apply.setBounds(r.removeFromTop(28).removeFromLeft(std::min(240,r.getWidth())));}
};
