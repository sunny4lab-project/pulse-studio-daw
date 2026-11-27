#pragma once
#include "../MixerStrip.h"
#include "../UI/EnterpriseCard.h"
// Advisory DSP, not a trained model. All EQ edits are opt-in and bounded.
class AIEngineerPage : public juce::Component {
 Engine& e;OwnedArray<Strip>& strips;EnterpriseCard modeCard,guard;Label recommendation;
 TextButton learn{"Analyze / suggest"},apply{"Apply EQ cut"},autoMaster{"Auto master"};ComboBox target,profile;
 float anchor=0;int selected=-1;float cut=0;int lastMode=0;bool armed=false;
public:
 std::function<void(float)> onMasterStep;
 AIEngineerPage(Engine& engine,OwnedArray<Strip>& s):e(engine),strips(s){
  for(auto* c:std::array<Component*,8>{&modeCard,&guard,&recommendation,&learn,&apply,&autoMaster,&target,&profile})addAndMakeVisible(c);
  recommendation.setFont(Font(11.f));learn.setClickingTogglesState(true);learn.onClick=[this]{e.analyzeMix=learn.getToggleState();};
  target.addItem("-18 LUFS",1);target.addItem("-16 LUFS",2);target.addItem("-14 LUFS",3);target.setSelectedId(1,dontSendNotification);
  profile.addItem("Balanced",1);profile.addItem("Praise",2);profile.addItem("Worship",3);profile.addItem("Sermon",4);profile.setSelectedId(1,dontSendNotification);
  autoMaster.setClickingTogglesState(true);autoMaster.onClick=[this]{armed=autoMaster.getToggleState();anchor=Decibels::gainToDecibels(e.master.load(),-60.f);};
  apply.onClick=[this]{if(selected<0||selected>=strips.size()||cut>=-.1f)return;auto* s=strips[selected];if(s->lock.getToggleState()||s->c.mute.load()||!e.enabled.load()||e.peak.load()>=.9f)return;s->c.eqFreq[6]=2500;s->c.eqQ[6]=1;s->c.eqGain[6]=std::max(-3.f,cut);};
 }
 int getMixProfile()const{return profile.getSelectedId();}
 void refresh(int mode){
  if(mode!=lastMode){if(mode==1){armed=false;autoMaster.setToggleState(false,dontSendNotification);}lastMode=mode;}
  bool solo=false;for(auto& c:e.ch)solo|=c.solo.load();
  bool ready=e.enabled.load()&&e.devices.getCurrentAudioDevice()!=nullptr&&!solo&&e.peak.load()<.9f;
  modeCard.set("MIXING MODE",mode==1?"Manual":mode==2?"Assisted":mode==4?"Hybrid":"Autonomous",profile.getText()+" · Rule-based fader control");
  guard.set("AUTOMATION GUARD",ready?"Ready":"Paused",ready?"Locks and gain bounds apply":"Live off, solo or high peak",ready?PulseTheme::cyan:juce::Colour(0xffe8bc62));
  selected=-1;cut=0;float vocal=0;if(e.analyzeMix.load()&&ready){for(auto* s:strips)if(!s->c.mute.load()&&s->c.peak.load()>.0032f&&(s->role.getSelectedId()==2||s->role.getSelectedId()==3))vocal+=s->c.bandEnergy[1].load()*s->c.gain.load()*s->c.gain.load();
   for(int i=0;i<strips.size();++i){auto* s=strips[i];if(s->role.getSelectedId()!=7&&s->role.getSelectedId()!=6)continue;if(s->lock.getToggleState()||s->c.mute.load()||s->c.peak.load()<.0032f||s->c.toneBypass.load()||std::abs(s->c.eqGain[6].load())>.01f)continue;float next=pulse::maskingCut(s->c.bandEnergy[1].load()*s->c.gain.load()*s->c.gain.load(),vocal);if(next<cut){selected=i;cut=next;}}}
  apply.setEnabled(selected>=0&&mode!=1);recommendation.setText(selected>=0?strips[selected]->name.getText()+": suggested "+String(cut,1)+" dB at 2.5 kHz (unused EQ 7)":String("No masking cut suggested — assign vocal and instrument roles; enable analysis."),dontSendNotification);
  if(armed&&ready&&mode!=1&&e.shortLufs.load()>-60&&onMasterStep){float db=Decibels::gainToDecibels(e.master.load(),-60.f);float step=pulse::masterStep(e.shortLufs.load(),target.getSelectedId()==3?-14.f:target.getSelectedId()==2?-16.f:-18.f,Decibels::gainToDecibels(e.estimatedTruePeak.load(),-100.f),db,anchor);if(std::abs(step)>.0001f)onMasterStep(step);}
 }
 void resized()override{auto r=getLocalBounds();auto row=r.removeFromTop(64);modeCard.setBounds(row.removeFromLeft(row.getWidth()/2));guard.setBounds(row);row=r.removeFromTop(27);int n=5;for(auto* c:std::array<Component*,5>{&learn,&apply,&autoMaster,&target,&profile})c->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(1));recommendation.setBounds(r);}
};
