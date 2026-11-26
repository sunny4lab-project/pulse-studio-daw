#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseCard.h"
// Preflight screen: selecting a driver or endpoint never opens it.
// Opening is explicit so the user can choose a compatible rate and buffer first.
class DeviceConnectPanel final : public juce::Component {
 Engine& e;
 juce::ComboBox backend,input,output,sampleRate,bufferSize,inputCount,outputCount;
 juce::Label header,note;
 std::array<juce::Label,7> labels;
 juce::TextEditor report;
 juce::TextButton refresh{"Refresh devices"},connect{"Connect"},advanced{"Advanced settings"},back{"Back to connection"};
 juce::StringArray backendNames,inputNames,outputNames;
 std::unique_ptr<juce::AudioDeviceSelectorComponent> selector;
 bool shared=false;
 juce::AudioIODeviceType* selectedType(){
  for(auto* t:e.devices.getAvailableDeviceTypes())if(t->getTypeName()==backend.getText())return t;
  return nullptr;
 }
 static void fill(juce::ComboBox& box,const juce::StringArray& names,const juce::String& selected,bool allowNone){
  box.clear(juce::dontSendNotification);
  if(allowNone)box.addItem("<none>",1);
  for(int i=0;i<names.size();++i)box.addItem(names[i],i+2);
  int index=names.indexOf(selected);box.setSelectedId(index>=0?index+2:allowNone?1:0,juce::dontSendNotification);
 }
 static juce::String selectedName(const juce::ComboBox& box,const juce::StringArray& names){int i=box.getSelectedId()-2;return juce::isPositiveAndBelow(i,names.size())?names[i]:juce::String();}
 void endpoints(const juce::String& in={},const juce::String& out={}){
  inputNames.clear();outputNames.clear();shared=false;
  if(auto* t=selectedType()){inputNames=t->getDeviceNames(true);outputNames=t->getDeviceNames(false);shared=!t->hasSeparateInputsAndOutputs();}
  fill(output,outputNames,out,true);fill(input,inputNames,in,true);
  if(shared){auto name=selectedName(output,outputNames);int idx=inputNames.indexOf(name);input.setSelectedId(idx>=0?idx+2:1,juce::dontSendNotification);}
  input.setEnabled(!shared);
  connect.setEnabled(!outputNames.isEmpty());
 }
 void inventory(){
  if(!e.canEditInserts()){report.setText("Stop live audio, playback and recording before scanning.",false);return;}
  auto oldType=backend.getText(),oldIn=selectedName(input,inputNames),oldOut=selectedName(output,outputNames);
  e.discoverAudioDevices(true);backendNames=e.registeredAudioBackends();
  fill(backend,backendNames,oldType,false);if(!backend.getSelectedId()&&!backendNames.isEmpty())backend.setSelectedId(2,juce::dontSendNotification);
  endpoints(oldIn,oldOut);
 }
 void showResult(const juce::String& error){
  if(error.isNotEmpty()){report.setText(error+"\n\nTry the interface's default rate/buffer, 2 inputs and 2 outputs. Close other audio applications, check the manufacturer's driver and hardware clock, then retry. No alternate device will be opened automatically.",false);return;}
  auto* d=e.devices.getCurrentAudioDevice();
  if(d&&d->isPlaying())report.setText("Connected: "+d->getName()+"\n"+juce::String(d->getCurrentSampleRate(),0)+" Hz | "+juce::String(d->getCurrentBufferSizeSamples())+" samples | "+juce::String(d->getActiveInputChannels().countNumberOfSetBits())+" inputs / "+juce::String(d->getActiveOutputChannels().countNumberOfSetBits())+" outputs\nClose this window, verify routing, then enable LIVE manually.",false);
  else report.setText("Choose the audio backend and output device, then Connect. No audio device is opened by selection alone.",false);
  advanced.setEnabled(d&&d->isPlaying());
 }
 void setAdvanced(bool enabled){
  if(enabled){
   if(!e.canEditInserts()||!e.devices.getCurrentAudioDevice())return;
   selector=std::make_unique<juce::AudioDeviceSelectorComponent>(e.devices,0,4096,2,pulse::maxOutputChannels,false,false,true,false);addAndMakeVisible(*selector);
  }else selector.reset();
  for(auto* box:{&backend,&input,&output,&sampleRate,&bufferSize,&inputCount,&outputCount})box->setVisible(!enabled);
  for(auto& l:labels)l.setVisible(!enabled);
  for(auto* c:std::array<juce::Component*,5>{&note,&report,&refresh,&connect,&advanced})c->setVisible(!enabled);
  back.setVisible(enabled);resized();if(!enabled)showResult(e.deviceError);
 }
 static void choices(juce::ComboBox& box,const juce::StringArray& names,int selected){
  for(int i=0;i<names.size();++i)box.addItem(names[i],i+1);box.setSelectedId(selected,juce::dontSendNotification);
 }
public:
 explicit DeviceConnectPanel(Engine& engine):e(engine){
  header.setText("Universal audio connection",juce::dontSendNotification);PulseTheme::style(header,18,true);addAndMakeVisible(header);
  note.setText("Select the interface and startup parameters before opening the driver. Use driver defaults or a known compatible sample rate and buffer.",juce::dontSendNotification);note.setJustificationType(juce::Justification::topLeft);PulseTheme::style(note,12,false,PulseTheme::muted);addAndMakeVisible(note);
  const char* names[]={"Audio backend","Input device","Output / shared device","Sample rate (Hz)","Buffer (samples)","Input channels","Output channels"};
  juce::ComboBox* boxes[]={&backend,&input,&output,&sampleRate,&bufferSize,&inputCount,&outputCount};
  for(int i=0;i<7;++i){labels[(size_t)i].setText(names[i],juce::dontSendNotification);addAndMakeVisible(labels[(size_t)i]);addAndMakeVisible(*boxes[i]);}
  choices(sampleRate,{"Driver default","44100","48000","88200","96000","176400","192000"},1);
  choices(bufferSize,{"Driver default","64","128","256","512","1024","2048"},1);
  choices(inputCount,{"0","1","2","4","8","16","24","32","48","64","128","256"},3);
  choices(outputCount,{"2","4","6","8"},1);
  sampleRate.setEditableText(true);bufferSize.setEditableText(true);inputCount.setEditableText(true);
  report.setMultiLine(true);report.setReadOnly(true);report.setScrollbarsShown(true);addAndMakeVisible(report);
  for(auto* b:{&refresh,&connect,&advanced,&back})addAndMakeVisible(*b);back.setVisible(false);
  refresh.onClick=[this]{inventory();};backend.onChange=[this]{endpoints();};
  output.onChange=[this]{if(shared){int idx=inputNames.indexOf(selectedName(output,outputNames));input.setSelectedId(idx>=0?idx+2:1,juce::dontSendNotification);}};
  connect.onClick=[this]{
   auto in=selectedName(input,inputNames),out=selectedName(output,outputNames);
   auto error=e.openSelectedDevice(backend.getText(),in,out,sampleRate.getText().getDoubleValue(),bufferSize.getText().getIntValue(),in.isEmpty()?0:inputCount.getText().getIntValue(),outputCount.getText().getIntValue());
   advanced.setEnabled(error.isEmpty());showResult(error);
  };
  advanced.onClick=[this]{setAdvanced(true);};back.onClick=[this]{setAdvanced(false);};
  inventory();auto* d=e.devices.getCurrentAudioDevice();
  if(d&&d->isPlaying()){
   auto current=e.devices.getAudioDeviceSetup();int idx=backendNames.indexOf(e.devices.getCurrentAudioDeviceType());if(idx>=0)backend.setSelectedId(idx+2,juce::dontSendNotification);endpoints(current.inputDeviceName,current.outputDeviceName);
   sampleRate.setText(juce::String(d->getCurrentSampleRate(),0),juce::dontSendNotification);bufferSize.setText(juce::String(d->getCurrentBufferSizeSamples()),juce::dontSendNotification);inputCount.setText(juce::String(d->getActiveInputChannels().countNumberOfSetBits()),juce::dontSendNotification);outputCount.setText(juce::String(d->getActiveOutputChannels().countNumberOfSetBits()),juce::dontSendNotification);
  }
  advanced.setEnabled(d&&d->isPlaying());showResult(e.deviceError);setSize(610,550);
 }
 void resized() override {
  auto r=getLocalBounds().reduced(12);header.setBounds(r.removeFromTop(32));
  if(selector){back.setBounds(r.removeFromBottom(32).removeFromLeft(180));selector->setBounds(r);return;}
  note.setBounds(r.removeFromTop(44));
  juce::ComboBox* boxes[]={&backend,&input,&output,&sampleRate,&bufferSize,&inputCount,&outputCount};
  for(int i=0;i<7;++i){auto row=r.removeFromTop(30);labels[(size_t)i].setBounds(row.removeFromLeft(170));boxes[i]->setBounds(row.reduced(2));}
  auto row=r.removeFromTop(36);refresh.setBounds(row.removeFromLeft(row.getWidth()/3).reduced(2));connect.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));advanced.setBounds(row.reduced(2));report.setBounds(r.reduced(2));
 }
};
