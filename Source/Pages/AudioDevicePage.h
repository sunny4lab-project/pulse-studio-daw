#pragma once
#include "../DeviceMonitor.h"
#include "../UI/EnterpriseCard.h"

// Operational device screen: show active endpoints first, discovery/debugging on demand.
class AudioDevicePage : public juce::Component, private juce::ListBoxModel {
 Engine& e; pulse::DeviceMonitor& monitor;
 juce::Label heading,deviceName,format,connection,state,section,columns,tip,result;
 juce::TextButton configure{"Configure device"},scan{"Refresh devices"},save{"Save setup"},bulk{"Create mixer inputs"},routing{"Open routing"},autoReconnect{"Auto reconnect"};
 juce::TextButton inputs{"Active inputs"},outputs{"Active outputs"},discovered{"Discovered devices"},debug{"Diagnostics"};
 juce::ListBox list{"Audio endpoints",this};
 juce::StringArray lines; int tab=0; bool live=false;
 int getNumRows() override {return lines.size();}
 void paintListBoxItem(int row,juce::Graphics& g,int w,int h,bool selected) override {
  if(!juce::isPositiveAndBelow(row,lines.size()))return;
  g.fillAll(selected?PulseTheme::active:row%2?PulseTheme::panel:PulseTheme::bgPrimary);
  auto r=juce::Rectangle<int>(8,0,w-16,h);
  g.setFont(PulseTheme::font(12));g.setColour(PulseTheme::text);
  if(tab<2){g.setColour(PulseTheme::cyan);g.drawText(juce::String(row+1),r.removeFromLeft(50),juce::Justification::centredLeft);g.setColour(PulseTheme::text);}
  g.drawFittedText(lines[row].trim(),r,juce::Justification::centredLeft,1);
  g.setColour(PulseTheme::border);g.drawHorizontalLine(h-1,0,(float)w);
 }
 void select(int n){tab=n;for(int i=0;i<4;++i)std::array<juce::TextButton*,4>{&inputs,&outputs,&discovered,&debug}[i]->setToggleState(i==tab,juce::dontSendNotification);list.deselectAllRows();refresh();}
 void updateLines(const juce::StringArray& next){if(lines==next)return;lines=next;list.updateContent();list.repaint();}
public:
 std::function<void()> onConfigure,onBulkAdd,onRouting;
 AudioDevicePage(Engine& engine,pulse::DeviceMonitor& watcher):e(engine),monitor(watcher){
  for(auto* x:std::array<juce::Component*,20>{&heading,&deviceName,&format,&connection,&state,&section,&columns,&tip,&result,&configure,&scan,&save,&bulk,&routing,&autoReconnect,&inputs,&outputs,&discovered,&debug,&list})addAndMakeVisible(x);
  heading.setText("AUDIO DEVICE SETUP",juce::dontSendNotification);PulseTheme::style(heading,16,true);
  PulseTheme::style(deviceName,16,true);PulseTheme::style(format,12,false,PulseTheme::muted);PulseTheme::style(connection,12,true,PulseTheme::cyan);PulseTheme::style(state,12,false,PulseTheme::muted);PulseTheme::style(section,11,true);PulseTheme::style(columns,11,false,PulseTheme::muted);PulseTheme::style(tip,11,false,PulseTheme::muted);PulseTheme::style(result,11,false,PulseTheme::cyan);
  configure.setColour(juce::TextButton::buttonColourId,PulseTheme::active);configure.onClick=[this]{if(onConfigure)onConfigure();};
  scan.onClick=[this]{if(!monitor.scanNow()){result.setText("Switch LIVE off and stop playback/recording before scanning.",juce::dontSendNotification);return;}result.setText("Device inventory refreshed.",juce::dontSendNotification);refresh();};
  save.onClick=[this]{auto error=e.saveDeviceSettings();result.setText(error.isEmpty()?juce::String("Device setup saved."):error,juce::dontSendNotification);};
  bulk.onClick=[this]{if(onBulkAdd)onBulkAdd();};routing.onClick=[this]{if(onRouting)onRouting();};
  autoReconnect.setClickingTogglesState(true);autoReconnect.setToggleState(monitor.autoEnabled(),juce::dontSendNotification);autoReconnect.onClick=[this]{monitor.setAutoEnabled(autoReconnect.getToggleState());};
  inputs.onClick=[this]{select(0);};outputs.onClick=[this]{select(1);};discovered.onClick=[this]{select(2);};debug.onClick=[this]{select(3);};
  list.setRowHeight(26);list.setColour(juce::ListBox::backgroundColourId,PulseTheme::bgPrimary);list.setOutlineThickness(1);list.setColour(juce::ListBox::outlineColourId,PulseTheme::border);select(0);
 }
 ~AudioDevicePage() override {list.setModel(nullptr);}
 void refresh(){
  auto* d=e.devices.getCurrentAudioDevice();bool active=d&&d->isPlaying()&&!e.deviceFault.load();live=e.enabled.load();bool idle=e.canEditInserts(),review=monitor.requiresReview();
  auto ins=e.activeChannelNames(true),outs=e.activeChannelNames(false);
  deviceName.setText(d?d->getName():"No audio interface connected",juce::dontSendNotification);
  connection.setText(active?"CONNECTED":"NOT CONNECTED",juce::dontSendNotification);connection.setColour(juce::Label::textColourId,active?PulseTheme::cyan:juce::Colour(0xfff59e0b));
  format.setText(d?d->getTypeName()+"  |  "+juce::String(d->getCurrentSampleRate(),0)+" Hz  |  "+juce::String(d->getCurrentBufferSizeSamples())+" samples  |  "+juce::String(ins.size())+" IN / "+juce::String(outs.size())+" OUT":"Select Configure device to choose a driver and interface.",juce::dontSendNotification);
  state.setText(live?"LIVE ON — turn LIVE off to change device or routing.":!idle?"Stop playback or recording to change device settings.":review?"Device needs verification — open Configure device, check and reconnect.":active?"LIVE OFF — ready to patch inputs and outputs.":"Choose and connect an audio device.",juce::dontSendNotification);
  configure.setEnabled(idle);scan.setEnabled(idle);save.setEnabled(idle&&active&&!review);bulk.setEnabled(idle&&active&&!ins.isEmpty()&&!review);
  bulk.setTooltip(!idle?"Turn LIVE off and stop playback/recording":!active?"Connect an audio device first":review?"Verify the device configuration first":ins.isEmpty()?"Enable hardware inputs first":"Create muted mixer strips for unused active input slots");
  inputs.setButtonText("Inputs ("+juce::String(ins.size())+")");outputs.setButtonText("Outputs ("+juce::String(outs.size())+")");
  juce::StringArray next;
  if(tab<2){
   next=tab==0?ins:outs;section.setText(tab==0?"ENABLED HARDWARE INPUTS":"ENABLED HARDWARE OUTPUTS",juce::dontSendNotification);columns.setText("SLOT        HARDWARE CHANNEL / DRIVER NAME",juce::dontSendNotification);
   tip.setText(next.isEmpty()?"No channels enabled. Open Configure device.":tab==0?"Create mixer inputs, then open Routing to assign mono/stereo sources.":"Use Routing to assign master and auxiliary stereo output pairs.",juce::dontSendNotification);
  }else if(tab==2){next=monitor.devices();section.setText("DISCOVERED DRIVER ENDPOINTS",juce::dontSendNotification);columns.setText("DRIVER / DEVICE (not necessarily connected)",juce::dontSendNotification);tip.setText("Discovery is separate from the active device. Select Configure device to connect.",juce::dontSendNotification);}
  else{
   section.setText("TECHNICAL DIAGNOSTICS",juce::dontSendNotification);columns.setText("ENGINE / DRIVER STATUS",juce::dontSendNotification);
 #if JUCE_WINDOWS && JUCE_ASIO
   next.add("Windows ASIO backend: ENABLED");
 #endif
   next.add(monitor.status());if(e.deviceError.isNotEmpty())next.add(e.deviceError);if(d){next.add("Device capacity: "+juce::String(d->getInputChannelNames().size())+" in / "+juce::String(d->getOutputChannelNames().size())+" out");juce::String rates="Available rates: ";for(auto v:d->getAvailableSampleRates())rates<<juce::String(v,0)<<" ";next.add(rates);juce::String buffers="Available buffers: ";for(auto v:d->getAvailableBufferSizes())buffers<<v<<" ";next.add(buffers);}
   tip.setText("Driver details for troubleshooting only.",juce::dontSendNotification);
  }
  updateLines(next);
 }
 void paint(juce::Graphics& g) override {g.fillAll(PulseTheme::bgPrimary);g.setColour(PulseTheme::panel);g.fillRoundedRectangle(4.f,34.f,(float)std::max(1,getWidth()-8),94.f,6.f);g.setColour(PulseTheme::border);g.drawRoundedRectangle(4.f,34.f,(float)std::max(1,getWidth()-8),94.f,6.f,1.f);}
 void resized() override {
  auto r=getLocalBounds().reduced(8);heading.setBounds(r.removeFromTop(26));auto card=r.removeFromTop(94).reduced(8,3);auto row=card.removeFromTop(26);connection.setBounds(row.removeFromRight(std::min(132,row.getWidth()/3)));deviceName.setBounds(row);format.setBounds(card.removeFromTop(24));state.setBounds(card.removeFromTop(24));r.removeFromTop(4);
  row=r.removeFromTop(29);int n=3;for(auto* b:{&configure,&scan,&save})b->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2,1));
  row=r.removeFromTop(29);n=3;for(auto* b:{&bulk,&routing,&autoReconnect})b->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2,1));
  result.setBounds(r.removeFromTop(18));row=r.removeFromTop(29);n=4;for(auto* b:{&inputs,&outputs,&discovered,&debug})b->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2,1));
  section.setBounds(r.removeFromTop(22));columns.setBounds(r.removeFromTop(18).reduced(8,0));tip.setBounds(r.removeFromBottom(24));list.setBounds(r.reduced(1));
 }
};
