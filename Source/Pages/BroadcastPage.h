#pragma once
#include "../AudioEngine.h"
#include "../ChannelColourUI.h"
#include "../AutoMixControl.h"
#include "../UI/EnterpriseCard.h"
#include "../UI/MeterWidget.h"
#include <vector>

// AUX 1 can carry an independently gained broadcast feed. The FOH master and
// other auxiliary mixes keep their existing routing and behavior.
class BroadcastPage : public juce::Component {
 struct ChannelRow : juce::Component {
  Engine& engine;juce::ComboBox& mode;int index;
  juce::Label title,peak,input;juce::Slider level,pan;juce::TextButton mute{"Mute"},lock{"Lock"};
  std::function<void(const juce::String&)> onChange;
  float anchor=0,envelope=0;bool manualHold=false;
  ChannelRow(Engine& e,juce::ComboBox& m,int i,std::function<void(const juce::String&)> callback):engine(e),mode(m),index(i),onChange(std::move(callback)){
   for(auto* c:std::array<juce::Component*,7>{&title,&peak,&input,&level,&pan,&mute,&lock})addAndMakeVisible(c);
   title.setText("CH "+juce::String(i+1),juce::dontSendNotification);title.setFont(juce::Font(12.f,juce::Font::bold));title.addMouseListener(this,false);peak.setFont(juce::Font(10.f));
   title.setJustificationType(juce::Justification::centred);input.setFont(juce::Font(10.f));input.setJustificationType(juce::Justification::centred);peak.setJustificationType(juce::Justification::centred);
   level.setSliderStyle(juce::Slider::LinearVertical);level.setRange(-60,3.52,.1);level.setTextValueSuffix(" dB");
   level.setTextBoxStyle(juce::Slider::TextBoxBelow,false,74,20);
   anchor=e.ch[i].broadcastGain.load();level.setValue(juce::Decibels::gainToDecibels(anchor,-60.f),juce::dontSendNotification);
   level.onValueChange=[this]{auto& c=engine.ch[index];float next=juce::Decibels::decibelsToGain((float)level.getValue(),-60.f);c.broadcastGain=next;anchor=next;
    if(mode.getSelectedId()==3||mode.getSelectedId()==4){c.broadcastLock=true;lock.setToggleState(true,juce::dontSendNotification);manualHold=true;}
    if(onChange)onChange("CH "+juce::String(index+1)+" manual fader "+juce::String(level.getValue(),1)+" dB");};
   pan.setSliderStyle(juce::Slider::LinearHorizontal);pan.setRange(-1.,1.,.01);pan.setTextBoxStyle(juce::Slider::TextBoxBelow,false,74,18);pan.setDoubleClickReturnValue(true,0.);pan.setValue(e.ch[i].broadcastPan.load(),juce::dontSendNotification);pan.setTooltip("Independent broadcast pan (centre = 0)");pan.onValueChange=[this]{engine.ch[index].broadcastPan=(float)pan.getValue();};
   for(auto* b:{&mute,&lock})b->setClickingTogglesState(true);
   mute.setToggleState(e.ch[i].broadcastMute.load(),juce::dontSendNotification);
   lock.setToggleState(e.ch[i].broadcastLock.load(),juce::dontSendNotification);
   mute.onClick=[this]{engine.ch[index].broadcastMute=mute.getToggleState();};
   lock.onClick=[this]{engine.ch[index].broadcastLock=lock.getToggleState();if(!lock.getToggleState())manualHold=false;};
  }
  void setAutomatic(float value){engine.ch[index].broadcastGain=value;level.setValue(juce::Decibels::gainToDecibels(value,-60.f),juce::dontSendNotification);}
  void mouseDown(const juce::MouseEvent& ev)override{if(ev.mods.isPopupMenu())openChannelColourMenu(*this,engine.ch[index]);}
  void paint(juce::Graphics& g)override{g.setColour(getChannelColour(engine.ch[index]));g.fillRoundedRectangle(juce::Rectangle<float>(1.f,5.f,4.f,(float)getHeight()-10.f),2.f);auto meter=juce::Rectangle<float>((float)getWidth()-15.f,105.f,7.f,(float)std::max(0,getHeight()-166));g.setColour(juce::Colour(0xff080c11));g.fillRect(meter);const float db=juce::Decibels::gainToDecibels(engine.ch[index].broadcastPeak.load(),-60.f);g.setColour(juce::Colour(0xff14e8c6));g.fillRect(meter.withTop(meter.getBottom()-meter.getHeight()*juce::jlimit(0.f,1.f,(db+60.f)/60.f)));}
  void refresh(){auto& c=engine.ch[index];title.setColour(juce::Label::textColourId,getChannelColour(c));if(!level.isMouseButtonDown())level.setValue(juce::Decibels::gainToDecibels(c.broadcastGain.load(),-60.f),juce::dontSendNotification);
   if(!pan.isMouseButtonDown())pan.setValue(c.broadcastPan.load(),juce::dontSendNotification);const int source=c.source.load(),right=c.rightSource.load();input.setText(source<0?juce::String("Unassigned"):("IN "+juce::String(source+1)+(right>=0?(" / "+juce::String(right+1)):juce::String())),juce::dontSendNotification);repaint();
   mute.setToggleState(c.broadcastMute.load(),juce::dontSendNotification);lock.setToggleState(c.broadcastLock.load(),juce::dontSendNotification);
   peak.setText(manualHold?"Manual hold":juce::String(juce::Decibels::gainToDecibels(c.broadcastPeak.load(),-100.f),0)+" dBFS",juce::dontSendNotification);}
  void resized()override{auto r=getLocalBounds().reduced(7,5);title.setBounds(r.removeFromTop(26));input.setBounds(r.removeFromTop(22));pan.setBounds(r.removeFromTop(46));auto buttons=r.removeFromBottom(25);mute.setBounds(buttons.removeFromLeft(buttons.getWidth()/2).reduced(1));lock.setBounds(buttons.reduced(1));peak.setBounds(r.removeFromBottom(22));r.removeFromRight(13);level.setBounds(r);}
 };
 struct Snapshot {std::vector<float> gain,pan;std::vector<bool> mute,lock;int mode=1,profile=1;bool saved=false;};
 Engine& e;EnterpriseCard route,loudness;MeterWidget output;
 juce::ComboBox mode,profile,slot;juce::TextButton enable{"Enable broadcast feed"},arm{"Arm broadcast"},save{"Save scene"},recall{"Recall scene"};
 juce::Label note;juce::Component bank;juce::Viewport viewport;juce::OwnedArray<ChannelRow> rows;
 juce::TextEditor history;juce::StringArray changes;std::array<Snapshot,4> scenes;int ticks=0,speechHold=0;
 void log(const juce::String& message){changes.add(juce::Time::getCurrentTime().formatted("%H:%M:%S")+"  "+message);while(changes.size()>100)changes.remove(0);
  if(isVisible())history.setText(changes.joinIntoString("\n"),false);}
 void ensureRows(){if(rows.size()==e.channelCount())return;rows.clear(true);
  for(int i=0;i<e.channelCount();++i){auto* row=rows.add(new ChannelRow(e,mode,i,[this](const juce::String& text){log(text);}));bank.addAndMakeVisible(row);}resized();}
 void saveScene(){auto& s=scenes[(size_t)std::clamp(slot.getSelectedId()-1,0,3)];s=Snapshot{};s.saved=true;s.mode=mode.getSelectedId();s.profile=profile.getSelectedId();
  for(auto& c:e.ch){s.gain.push_back(c.broadcastGain.load());s.pan.push_back(c.broadcastPan.load());s.mute.push_back(c.broadcastMute.load());s.lock.push_back(c.broadcastLock.load());}
  log("Saved broadcast scene "+juce::String(slot.getSelectedId()));}
 void recallScene(){auto& s=scenes[(size_t)std::clamp(slot.getSelectedId()-1,0,3)];if(!s.saved||s.gain.size()!=(size_t)e.channelCount()){log("Scene unavailable: save again after changing channel count");return;}
  mode.setSelectedId(s.mode,juce::dontSendNotification);profile.setSelectedId(s.profile,juce::dontSendNotification);
  for(int i=0;i<e.channelCount();++i){auto& c=e.ch[i];c.broadcastGain=s.gain[(size_t)i];c.broadcastPan=s.pan[(size_t)i];c.broadcastMute=s.mute[(size_t)i];c.broadcastLock=s.lock[(size_t)i];
   rows[i]->anchor=s.gain[(size_t)i];rows[i]->manualHold=false;rows[i]->refresh();}log("Recalled scene "+juce::String(slot.getSelectedId())+" (routing and arming unchanged)");}
public:
 explicit BroadcastPage(Engine& engine):e(engine){
  for(auto* c:std::array<juce::Component*,10>{&route,&loudness,&output,&mode,&profile,&slot,&enable,&arm,&save,&recall})addAndMakeVisible(c);
  addAndMakeVisible(note);addAndMakeVisible(viewport);addAndMakeVisible(history);viewport.setViewedComponent(&bank,false);
  mode.addItem("Manual",1);mode.addItem("Assisted",2);mode.addItem("Autonomous",3);mode.addItem("Hybrid",4);mode.setSelectedId(1,juce::dontSendNotification);
  profile.addItem("Balanced",1);profile.addItem("Praise",2);profile.addItem("Worship",3);profile.addItem("Sermon",4);profile.setSelectedId(1,juce::dontSendNotification);
  for(int i=1;i<=4;++i)slot.addItem("Scene "+juce::String(i),i);slot.setSelectedId(1,juce::dontSendNotification);
  enable.onClick=[this]{if(!e.canEditInserts()){log("Stop live audio before changing broadcast routing");return;}e.broadcastDirectEnabled=!e.broadcastDirectEnabled.load();refresh();};
  arm.onClick=[this]{if(e.auxArmed[0].load())e.auxArmed[0]=false;else if(!e.enabled.load()&&e.auxAvailable[0].load())e.auxArmed[0]=true;refresh();};
  save.onClick=[this]{saveScene();};recall.onClick=[this]{recallScene();};
  note.setText("Broadcast uses AUX 1 on a separate hardware stereo pair. Configure its pair in Routing; arm while LIVE is off. Independent channel faders never move FOH faders.",juce::dontSendNotification);
  note.setFont(juce::Font(11.f));note.setJustificationType(juce::Justification::centredLeft);
  history.setMultiLine(true);history.setReadOnly(true);history.setScrollbarsShown(true);history.setFont(juce::Font(10.f));ensureRows();refresh();
 }
 void tick(){ensureRows();++ticks;
  if(!e.broadcastDirectEnabled.load()||!e.auxArmed[0].load()||!e.auxAvailable[0].load()||!e.enabled.load()||e.deviceFault.load()||e.alignmentFault.load()||e.callbackDeadline.latestPermille.load()>=900){speechHold=0;return;}
  bool speaking=false;for(auto& c:e.ch)if(c.mixRole.load()==3&&!c.broadcastMute.load()&&c.broadcastGain.load()>.001f&&c.peak.load()>.015f)speaking=true;
  speechHold=speaking?30:std::max(0,speechHold-1);
  bool autoMode=mode.getSelectedId()==3||mode.getSelectedId()==4;
  for(int i=0;i<rows.size();++i){auto* row=rows[i];auto& c=e.ch[i];row->envelope=.85f*row->envelope+.15f*c.peak.load();
   if(!autoMode||c.broadcastLock.load()||c.broadcastMute.load()||row->anchor<.001f||c.peak.load()<=.0032f||row->envelope<=.0032f)continue;
   float current=c.broadcastGain.load(),target=pulse::AutoMixControl::propose(row->anchor,row->envelope,c.mixRole.load(),profile.getSelectedId(),speechHold>0);
   float next=pulse::AutoMixControl::ride(current,target,mode.getSelectedId()==4);
   if(std::abs(next-current)>.000001f){row->setAutomatic(next);if(ticks%15==0)log("Auto CH "+juce::String(i+1)+": "+juce::String(juce::Decibels::gainToDecibels(next,-60.f),1)+" dB ("+profile.getText()+")");}
  }
 }
 void refresh(){ensureRows();auto* device=e.devices.getCurrentAudioDevice();bool online=device&&device->isPlaying();int pair=e.auxPair[0].load();bool armed=e.auxArmed[0].load();
  route.set("BROADCAST AUX 1",pair>=0?"Outputs "+juce::String(2*pair+1)+"-"+juce::String(2*pair+2):"Unrouted",armed?"Armed / independent of FOH":"Disarmed — route and arm first");
  float integrated=e.integratedLufs.load();loudness.set("HOUSE MASTER LUFS",online&&integrated>-99?juce::String(integrated,1)+" LUFS":"--","Broadcast has separate faders; loudness meter is FOH only");
  output.setPeak(e.auxPeak[0].load(),"Broadcast sample peak");
  enable.setButtonText(e.broadcastDirectEnabled.load()?"Disable broadcast feed":"Enable broadcast feed");enable.setEnabled(e.canEditInserts());
  arm.setButtonText(armed?"Disarm broadcast":"Arm broadcast");arm.setEnabled(armed||(!e.enabled.load()&&e.auxAvailable[0].load()));
  for(auto* row:rows)row->refresh();
 }
 void resized()override{auto r=getLocalBounds().reduced(3);auto top=r.removeFromTop(67);route.setBounds(top.removeFromLeft(top.getWidth()/2));loudness.setBounds(top);
  auto c=r.removeFromTop(29);mode.setBounds(c.removeFromLeft(108).reduced(1));profile.setBounds(c.removeFromLeft(110).reduced(1));enable.setBounds(c.removeFromLeft(std::min(180,c.getWidth()/2)).reduced(1));arm.setBounds(c.reduced(1));
  c=r.removeFromTop(28);slot.setBounds(c.removeFromLeft(96).reduced(1));save.setBounds(c.removeFromLeft(105).reduced(1));recall.setBounds(c.removeFromLeft(105).reduced(1));output.setBounds(c.reduced(1));
  note.setBounds(r.removeFromTop(32));history.setBounds(r.removeFromBottom(std::min(65,r.getHeight()/4)));viewport.setBounds(r);
  const int stripWidth=104;const int stripHeight=std::max(255,viewport.getHeight()-18);bank.setSize(std::max(viewport.getWidth()-12,rows.size()*stripWidth),stripHeight);for(int i=0;i<rows.size();++i)rows[i]->setBounds(i*stripWidth,0,stripWidth,stripHeight);
 }
};
