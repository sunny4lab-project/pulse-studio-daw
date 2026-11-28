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
  juce::Label title,peak;juce::ComboBox input,rightInput;juce::StringArray inputNames;juce::Slider level,pan;juce::TextButton mute{"Mute"},lock{"Lock"};
  std::function<void(const juce::String&)> onChange;
  float anchor=0,envelope=0;bool manualHold=false;
  ChannelRow(Engine& e,juce::ComboBox& m,int i,std::function<void(const juce::String&)> callback):engine(e),mode(m),index(i),onChange(std::move(callback)){
   for(auto* c:std::array<juce::Component*,8>{&title,&peak,&input,&rightInput,&level,&pan,&mute,&lock})addAndMakeVisible(c);
   title.setText("CH "+juce::String(i+1),juce::dontSendNotification);title.setFont(juce::Font(12.f,juce::Font::bold));title.addMouseListener(this,false);peak.setFont(juce::Font(10.f));
   title.setJustificationType(juce::Justification::centred);input.setTooltip("Broadcast left / mono input. Follow mixer uses existing mixer source and processing.");rightInput.setTooltip("Broadcast right input; Mono uses left input only.");input.onChange=[this]{if(!engine.canEditInserts())return;auto& c=engine.ch[index];c.broadcastSource=input.getSelectedId()-3;if(onChange)onChange("CH "+juce::String(index+1)+" input: "+input.getText());};rightInput.onChange=[this]{if(!engine.canEditInserts())return;engine.ch[index].broadcastRightSource=rightInput.getSelectedId()-2;if(onChange)onChange("CH "+juce::String(index+1)+" right: "+rightInput.getText());};peak.setJustificationType(juce::Justification::centred);
   level.setSliderStyle(juce::Slider::LinearVertical);level.setRange(-60,3.52,.1);level.setTextValueSuffix(" dB");
   level.setTextBoxStyle(juce::Slider::TextBoxBelow,false,74,20);
   anchor=e.ch[i].broadcastGain.load();level.setValue(juce::Decibels::gainToDecibels(anchor,-60.f),juce::dontSendNotification);
   level.onValueChange=[this]{auto& c=engine.ch[index];float next=juce::Decibels::decibelsToGain((float)level.getValue(),-60.f);c.broadcastGain=next;anchor=next;
    if(mode.getSelectedId()==3||mode.getSelectedId()==4){c.broadcastLock=true;lock.setToggleState(true,juce::dontSendNotification);manualHold=true;}
    if(!level.isMouseButtonDown()&&onChange)onChange("CH "+juce::String(index+1)+" fader "+juce::String(level.getValue(),1)+" dB");};level.onDragEnd=[this]{if(onChange)onChange("CH "+juce::String(index+1)+" fader "+juce::String(level.getValue(),1)+" dB");};
   pan.setSliderStyle(juce::Slider::LinearHorizontal);pan.setRange(-1.,1.,.01);pan.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);pan.setDoubleClickReturnValue(true,0.);pan.setValue(e.ch[i].broadcastPan.load(),juce::dontSendNotification);pan.setTooltip("Independent broadcast pan (centre = 0)");pan.onValueChange=[this]{engine.ch[index].broadcastPan=(float)pan.getValue();};
   for(auto* b:{&mute,&lock})b->setClickingTogglesState(true);
   mute.setToggleState(e.ch[i].broadcastMute.load(),juce::dontSendNotification);
   lock.setToggleState(e.ch[i].broadcastLock.load(),juce::dontSendNotification);
   mute.onClick=[this]{engine.ch[index].broadcastMute=mute.getToggleState();};
   lock.onClick=[this]{engine.ch[index].broadcastLock=lock.getToggleState();if(!lock.getToggleState())manualHold=false;};
  }
  void setAutomatic(float value){engine.ch[index].broadcastGain=value;level.setValue(juce::Decibels::gainToDecibels(value,-60.f),juce::dontSendNotification);}
  void mouseDown(const juce::MouseEvent& ev)override{if(ev.mods.isPopupMenu())openChannelColourMenu(*this,engine.ch[index]);}
  void paint(juce::Graphics& g)override{g.setColour(juce::Colour(0xff111923));g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1),5);g.setColour(juce::Colour(0xff233447));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1),5,1);g.setColour(getChannelColour(engine.ch[index]));g.fillRoundedRectangle(juce::Rectangle<float>(1.f,5.f,4.f,(float)getHeight()-10.f),2.f);auto meter=juce::Rectangle<float>((float)getWidth()-15.f,105.f,7.f,(float)std::max(0,getHeight()-166));g.setColour(juce::Colour(0xff080c11));g.fillRect(meter);const float db=juce::Decibels::gainToDecibels(engine.ch[index].broadcastPeak.load(),-60.f);g.setColour(juce::Colour(0xff14e8c6));g.fillRect(meter.withTop(meter.getBottom()-meter.getHeight()*juce::jlimit(0.f,1.f,(db+60.f)/60.f)));}
  void refresh(){auto& c=engine.ch[index];title.setColour(juce::Label::textColourId,getChannelColour(c));if(!level.isMouseButtonDown())level.setValue(juce::Decibels::gainToDecibels(c.broadcastGain.load(),-60.f),juce::dontSendNotification);
   if(!pan.isMouseButtonDown())pan.setValue(c.broadcastPan.load(),juce::dontSendNotification);const int source=c.broadcastSource.load(),right=c.broadcastRightSource.load();const auto names=engine.activeChannelNames(true);
   if(!input.isPopupActive()&&!rightInput.isPopupActive()){
    if(inputNames!=names||input.getNumItems()==0){inputNames=names;input.clear(juce::dontSendNotification);rightInput.clear(juce::dontSendNotification);input.addItem("Follow mixer",1);input.addItem("No input",2);rightInput.addItem("Mono",1);for(int i=0;i<names.size();++i){input.addItem("IN "+juce::String(i+1),i+3);rightInput.addItem("IN "+juce::String(i+1),i+2);}}
    if(source>=0&&input.indexOfItemId(source+3)<0)input.addItem("IN "+juce::String(source+1)+" (off)",source+3);if(right>=0&&rightInput.indexOfItemId(right+2)<0)rightInput.addItem("IN "+juce::String(right+1)+" (off)",right+2);
    input.setItemEnabled(1,true);if(input.getItemText(0)!=(c.source.load()<0?"Patch input":"Follow mixer"))input.changeItemText(1,c.source.load()<0?"Patch input":"Follow mixer");
    input.setSelectedId(source+3,juce::dontSendNotification);rightInput.setSelectedId(right+2,juce::dontSendNotification);
   }
   input.setEnabled(engine.canEditInserts());rightInput.setEnabled(engine.canEditInserts()&&source>=0);input.setTooltip(source>=0&&source<names.size()?names[source]:"Choose an input or follow the mixer");repaint();
   mute.setToggleState(c.broadcastMute.load(),juce::dontSendNotification);lock.setToggleState(c.broadcastLock.load(),juce::dontSendNotification);
   peak.setText(manualHold?"Manual hold":juce::String(juce::Decibels::gainToDecibels(c.broadcastPeak.load(),-100.f),0)+" dBFS",juce::dontSendNotification);}
  void resized()override{auto r=getLocalBounds().reduced(7,5);title.setBounds(r.removeFromTop(26));input.setBounds(r.removeFromTop(25).reduced(1));rightInput.setBounds(r.removeFromTop(23).reduced(1));pan.setBounds(r.removeFromTop(25));auto buttons=r.removeFromBottom(25);mute.setBounds(buttons.removeFromLeft(buttons.getWidth()/2).reduced(1));lock.setBounds(buttons.reduced(1));peak.setBounds(r.removeFromBottom(22));r.removeFromRight(13);level.setBounds(r);}
 };
 struct Snapshot {std::vector<float> gain,pan;std::vector<bool> mute,lock;int mode=1,profile=1;bool saved=false;};
 Engine& e;EnterpriseCard route,loudness;MeterWidget output;juce::ComboBox outputPair;juce::StringArray outputNames;juce::Slider masterGain;juce::Label peakInfo;juce::TextButton masterMute{"Mute master"};float restoreGain=1.f;
 juce::ComboBox mode,profile,slot;juce::TextButton enable{"Enable broadcast feed"},arm{"Arm broadcast"},save{"Save scene"},recall{"Recall scene"};
 juce::Label note;juce::TextButton patchInputs{"Auto patch"},showLog{"Log"};bool logVisible=false;juce::Component bank;juce::Viewport viewport;juce::OwnedArray<ChannelRow> rows;
 juce::TextEditor history;juce::StringArray changes;std::array<Snapshot,4> scenes;int ticks=0,speechHold=0;
 void log(const juce::String& message){changes.add(juce::Time::getCurrentTime().formatted("%H:%M:%S")+"  "+message);while(changes.size()>100)changes.remove(0);
  if(logVisible&&isShowing())history.setText(changes.joinIntoString("\n"),false);}
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
  addAndMakeVisible(outputPair);addAndMakeVisible(masterGain);addAndMakeVisible(peakInfo);addAndMakeVisible(masterMute);outputPair.setTooltip("Broadcast hardware stereo pair; routing changes require LIVE off");outputPair.onChange=[this]{if(!e.setOutputPair(1,outputPair.getSelectedId()-2))log("Output unavailable or already assigned; stop live first");refresh();};masterGain.setSliderStyle(juce::Slider::LinearHorizontal);masterGain.setRange(-60,0,.1);masterGain.setTextValueSuffix(" dB");masterGain.setTextBoxStyle(juce::Slider::TextBoxRight,false,60,20);masterGain.onValueChange=[this]{e.auxGain[0]=juce::Decibels::decibelsToGain((float)masterGain.getValue(),-60.f);};masterMute.onClick=[this]{if(e.auxGain[0].load()>.00001f){restoreGain=e.auxGain[0].load();e.auxGain[0]=0.f;}else e.auxGain[0]=restoreGain;refresh();};peakInfo.setFont(juce::Font(10.f));peakInfo.setJustificationType(juce::Justification::topLeft);addAndMakeVisible(note);addAndMakeVisible(viewport);addAndMakeVisible(history);history.setVisible(false);addAndMakeVisible(patchInputs);addAndMakeVisible(showLog);showLog.setClickingTogglesState(true);showLog.onClick=[this]{logVisible=showLog.getToggleState();history.setVisible(logVisible);if(logVisible)history.setText(changes.joinIntoString("\n"),false);resized();};patchInputs.onClick=[this]{if(!e.canEditInserts())return;auto names=e.activeChannelNames(true);int patched=0;for(int i=0;i<e.channelCount()&&i<names.size();++i){auto& c=e.ch[i];if(c.broadcastSource.load()==-2&&c.source.load()<0){c.broadcastSource=i;c.broadcastRightSource=-1;c.broadcastMute=true;++patched;}}log("Auto patched "+juce::String(patched)+" empty channels 1:1; muted for safety");refresh();};patchInputs.setTooltip("Map unpatched channels to matching enabled hardware input slots, muted for safety. Existing patches are retained.");viewport.setViewedComponent(&bank,false);
  mode.addItem("Manual",1);mode.addItem("Assisted",2);mode.addItem("Autonomous",3);mode.addItem("Hybrid",4);mode.setSelectedId(1,juce::dontSendNotification);
  profile.addItem("Balanced",1);profile.addItem("Praise",2);profile.addItem("Worship",3);profile.addItem("Sermon",4);profile.setSelectedId(1,juce::dontSendNotification);
  for(int i=1;i<=4;++i)slot.addItem("Scene "+juce::String(i),i);slot.setSelectedId(1,juce::dontSendNotification);
  enable.onClick=[this]{if(!e.canEditInserts()){log("Stop live audio before changing broadcast routing");return;}e.broadcastDirectEnabled=!e.broadcastDirectEnabled.load();refresh();};
  arm.onClick=[this]{if(e.auxArmed[0].load())e.auxArmed[0]=false;else if(!e.enabled.load()&&e.auxAvailable[0].load())e.auxArmed[0]=true;refresh();};
  save.onClick=[this]{saveScene();};recall.onClick=[this]{recallScene();};
  note.setText("Patch with LIVE off; arm, then enable LIVE.",juce::dontSendNotification);
  note.setFont(juce::Font(11.f));note.setJustificationType(juce::Justification::centredLeft);
  history.setMultiLine(true);history.setReadOnly(true);history.setScrollbarsShown(true);history.setFont(juce::Font(10.f));ensureRows();refresh();
 }
 void tick(){ensureRows();++ticks;
  if(!e.broadcastDirectEnabled.load()||!e.auxArmed[0].load()||!e.auxAvailable[0].load()||!e.enabled.load()||e.deviceFault.load()||e.alignmentFault.load()||e.callbackDeadline.latestPermille.load()>=900){speechHold=0;return;}
  bool speaking=false;for(auto& c:e.ch)if(c.mixRole.load()==3&&!c.broadcastMute.load()&&c.broadcastGain.load()>.001f&&c.broadcastInputPeak.load()>.015f)speaking=true;
  speechHold=speaking?30:std::max(0,speechHold-1);
  bool autoMode=mode.getSelectedId()==3||mode.getSelectedId()==4;
  for(int i=0;i<rows.size();++i){auto* row=rows[i];auto& c=e.ch[i];row->envelope=.85f*row->envelope+.15f*c.broadcastInputPeak.load();
   if(!autoMode||c.broadcastLock.load()||c.broadcastMute.load()||row->anchor<.001f||c.broadcastInputPeak.load()<=.0032f||row->envelope<=.0032f)continue;
   float current=c.broadcastGain.load(),target=pulse::AutoMixControl::propose(row->anchor,row->envelope,c.mixRole.load(),profile.getSelectedId(),speechHold>0);
   float next=pulse::AutoMixControl::ride(current,target,mode.getSelectedId()==4);
   if(std::abs(next-current)>.000001f){row->setAutomatic(next);if(ticks%15==0)log("Auto CH "+juce::String(i+1)+": "+juce::String(juce::Decibels::gainToDecibels(next,-60.f),1)+" dB ("+profile.getText()+")");}
  }
 }
 void refresh(){ensureRows();auto* device=e.devices.getCurrentAudioDevice();bool online=device&&device->isPlaying();int pair=e.auxPair[0].load();bool armed=e.auxArmed[0].load();
  route.set("BROADCAST MASTER",pair>=0?"Out "+juce::String(2*pair+1)+"-"+juce::String(2*pair+2):"No output",armed?"Armed":e.auxAvailable[0].load()?"Disarmed":"Output unavailable");patchInputs.setEnabled(e.canEditInserts()&&!e.activeChannelNames(true).isEmpty());
  bool active=online&&armed&&e.enabled.load()&&e.broadcastDirectEnabled.load()&&e.auxAvailable[0].load()&&!e.deviceFault.load()&&!e.alignmentFault.load();
  float integrated=e.broadcastIntegratedLufs.load(),shortTerm=e.broadcastShortLufs.load();loudness.set("BROADCAST SHORT-TERM",active&&shortTerm>-99?juce::String(shortTerm,1)+" LUFS":"--",active&&integrated>-99?"Integrated "+juce::String(integrated,1)+" LUFS":"Post-output / waiting for signal");
  auto db=[](float v){return juce::String(juce::Decibels::gainToDecibels(v,-100.f),1);};peakInfo.setText(active?"L "+db(e.broadcastLeftPeak.load())+" / R "+db(e.broadcastRightPeak.load())+" dBFS\nPeak estimate "+db(e.broadcastTruePeakEstimate.load())+" dBTP":"L / R offline\nPeak estimate unavailable",juce::dontSendNotification);
  auto names=e.activeChannelNames(false);if(names!=outputNames||outputPair.getNumItems()==0){outputNames=names;outputPair.clear(juce::dontSendNotification);outputPair.addItem("Disabled",1);for(int i=0;i<names.size()/2;++i)outputPair.addItem(names[2*i]+" / "+names[2*i+1],i+2);}if(pair>=0&&outputPair.indexOfItemId(pair+2)<0)outputPair.addItem("Pair "+juce::String(pair+1)+" (unavailable)",pair+2);outputPair.setSelectedId(pair+2,juce::dontSendNotification);outputPair.setEnabled(e.canEditInserts());if(!masterGain.isMouseButtonDown())masterGain.setValue(juce::Decibels::gainToDecibels(e.auxGain[0].load(),-60.f),juce::dontSendNotification);masterMute.setButtonText(e.auxGain[0].load()<=.00001f?"Restore master":"Mute master");
  output.setPeak(e.auxPeak[0].load(),"Sample peak");
  enable.setButtonText(e.broadcastDirectEnabled.load()?"Disable broadcast feed":"Enable broadcast feed");enable.setEnabled(e.canEditInserts());
  arm.setButtonText(armed?"Disarm broadcast":"Arm broadcast");arm.setEnabled(armed||(!e.enabled.load()&&e.auxAvailable[0].load()));
  if(isShowing())for(auto* row:rows)if(row->getBounds().intersects(viewport.getViewArea()))row->refresh();
 }
 void resized()override{auto r=getLocalBounds().reduced(4);auto side=r.removeFromRight(std::min(184,r.getWidth()/3));route.setBounds(side.removeFromTop(64));loudness.setBounds(side.removeFromTop(64));outputPair.setBounds(side.removeFromTop(26).reduced(2));masterGain.setBounds(side.removeFromTop(28).reduced(2));masterMute.setBounds(side.removeFromTop(24).reduced(2));output.setBounds(side.removeFromTop(60));peakInfo.setBounds(side.removeFromTop(38).reduced(2));note.setBounds(side.removeFromTop(std::min(34,side.getHeight())));showLog.setBounds(side.removeFromTop(23).reduced(2,1));if(logVisible)history.setBounds(side);else history.setBounds(0,0,0,0);
  auto c=r.removeFromTop(29);mode.setBounds(c.removeFromLeft(108).reduced(1));profile.setBounds(c.removeFromLeft(110).reduced(1));enable.setBounds(c.removeFromLeft(std::min(180,c.getWidth()/2)).reduced(1));arm.setBounds(c.reduced(1));
  c=r.removeFromTop(28);slot.setBounds(c.removeFromLeft(96).reduced(1));save.setBounds(c.removeFromLeft(105).reduced(1));recall.setBounds(c.removeFromLeft(105).reduced(1));patchInputs.setBounds(c.removeFromLeft(std::min(90,c.getWidth())).reduced(1));viewport.setBounds(r);
  const int stripWidth=82;const int stripHeight=std::max(255,viewport.getHeight()-18);bank.setSize(std::max(viewport.getWidth()-12,rows.size()*stripWidth),stripHeight);for(int i=0;i<rows.size();++i)rows[i]->setBounds(i*stripWidth,0,stripWidth,stripHeight);
 }
};
