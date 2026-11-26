#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseCard.h"
class AudioDevicePage : public juce::Component {
 Engine& e;EnterpriseCard driver,channels;
 juce::TextButton configure{"Configure device"},scan{"Scan all devices"},save{"Save setup"};
 juce::TextEditor diagnostics;juce::Label note,result;
 juce::StringArray discovered;
 bool scanned=false;
public:
 std::function<void()> onConfigure;
 explicit AudioDevicePage(Engine& engine):e(engine){
  for(auto* x:std::array<juce::Component*,8>{&driver,&channels,&configure,&scan,&save,&diagnostics,&note,&result})addAndMakeVisible(x);
  configure.onClick=[this]{if(onConfigure)onConfigure();};
  scan.onClick=[this]{if(!e.canEditInserts()){result.setText("Stop live audio, playback and recording first.",juce::dontSendNotification);return;}discovered=e.discoverAudioDevices();scanned=true;result.setText(discovered.isEmpty()?"No audio backends reported. Check the system driver installation.":"Device inventory refreshed. Choose a device using Configure device.",juce::dontSendNotification);refresh();};
  save.onClick=[this]{auto error=e.saveDeviceSettings();result.setText(error.isEmpty()?juce::String("Saved. Setup will be recalled when Device is opened next launch."):error,juce::dontSendNotification);};
  diagnostics.setMultiLine(true);diagnostics.setReadOnly(true);diagnostics.setScrollbarsShown(true);diagnostics.setCaretVisible(false);
  note.setText("Connect any supported USB, Thunderbolt, built-in or network audio interface. Install its system driver if required, scan, then configure. Windows: prefer ASIO for professional multichannel audio. macOS and Linux: select the available system backend.",juce::dontSendNotification);
  PulseTheme::style(note,12,false,PulseTheme::muted);note.setJustificationType(juce::Justification::topLeft);PulseTheme::style(result,12,false,PulseTheme::muted);
  refresh();
 }
 void refresh(){
  auto* d=e.devices.getCurrentAudioDevice();bool active=d&&d->isPlaying();
  auto ins=e.activeChannelNames(true),outs=e.activeChannelNames(false);
  driver.set("ACTIVE AUDIO DEVICE",active?"Driver active":"Not connected",d?d->getName():"Choose an audio device");
  channels.set("ACTIVE CHANNELS",active?juce::String(ins.size())+" in / "+juce::String(outs.size())+" out":juce::String("--"),d?d->getTypeName():"No active backend");
  juce::String text;
 #if JUCE_WINDOWS && JUCE_ASIO
  text<<"Windows ASIO backend: ENABLED\n";
 #endif
  if(scanned){text<<"\nDISCOVERED AUDIO DEVICES (last scan)\n";for(auto& n:discovered)text<<n<<"\n";}
  else text<<"\nSelect Scan all devices to list available drivers and endpoints.\n";
  if(d){
   text<<"\nCURRENT DEVICE\n"<<d->getTypeName()<<" | "<<d->getName()<<"\n";
   text<<"Sample rate: "<<juce::String(d->getCurrentSampleRate(),0)<<" Hz | Buffer: "<<d->getCurrentBufferSizeSamples()<<" samples\n";
   text<<"Device capacity: "<<d->getInputChannelNames().size()<<" in / "<<d->getOutputChannelNames().size()<<" out\n";
   text<<"Enabled channels: "<<ins.size()<<" in / "<<outs.size()<<" out\n";
   text<<"Available sample rates: ";for(auto rate:d->getAvailableSampleRates())text<<juce::String(rate,0)<<" ";text<<"Hz\n";
   text<<"Available buffer sizes: ";for(auto size:d->getAvailableBufferSizes())text<<size<<" ";text<<"samples\n";
   text<<"\nInput slots (active-channel order):\n";for(int i=0;i<ins.size();++i)text<<juce::String(i+1)<<": "<<ins[i]<<"\n";
   text<<"\nOutput slots (master 1/2, AUX 3/4, 5/6, 7/8):\n";for(int i=0;i<outs.size();++i)text<<juce::String(i+1)<<": "<<outs[i]<<"\n";
  }else text<<"\nNo active audio device. Open Configure device to select one.";
  if(e.deviceError.isNotEmpty()&&!active)text<<"\nError: "<<e.deviceError;
  if(diagnostics.getText()!=text)diagnostics.setText(text,false);
  bool idle=!e.enabled.load()&&!e.transport.isPlaying()&&!e.multitrack.active();
  configure.setEnabled(idle);scan.setEnabled(idle);save.setEnabled(idle&&active);
 }
 void resized() override {auto r=getLocalBounds().reduced(5);auto row=r.removeFromTop(100);driver.setBounds(row.removeFromLeft(row.getWidth()/2));channels.setBounds(row);row=r.removeFromTop(34);configure.setBounds(row.removeFromLeft(row.getWidth()/3).reduced(2));scan.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));save.setBounds(row.reduced(2));result.setBounds(r.removeFromTop(30));note.setBounds(r.removeFromTop(70));diagnostics.setBounds(r.reduced(2));}
};
