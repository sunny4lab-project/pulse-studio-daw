#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseCard.h"
class DantePage : public juce::Component {
 Engine& e;EnterpriseCard driver,clock;
 juce::TextButton configure{"Open audio devices"},scan{"Scan ASIO drivers"},save{"Save device setup"};
 juce::TextEditor diagnostics;juce::Label note,result;
 juce::StringArray discovered;
 bool scanned=false;
public:
 std::function<void()> onConfigure;
 explicit DantePage(Engine& engine):e(engine){
  for(auto* x:std::array<juce::Component*,8>{&driver,&clock,&configure,&scan,&save,&diagnostics,&note,&result})addAndMakeVisible(x);
  configure.onClick=[this]{if(onConfigure)onConfigure();};
  scan.onClick=[this]{if(!e.canEditInserts()){result.setText("Stop live audio, playback and recording first.",juce::dontSendNotification);return;}discovered=e.discoverAsioDrivers();scanned=true;result.setText(discovered.isEmpty()?"No ASIO driver found. Check DVS installation and restart Pulse Studio.":"Driver list refreshed.",juce::dontSendNotification);refresh();};
  save.onClick=[this]{auto error=e.saveDeviceSettings();result.setText(error.isEmpty()?juce::String("Saved. Setup will be recalled when Device is opened next launch."):error,juce::dontSendNotification);};
  diagnostics.setMultiLine(true);diagnostics.setReadOnly(true);diagnostics.setScrollbarsShown(true);diagnostics.setCaretVisible(false);
  note.setText("Windows: start Dante Virtual Soundcard in ASIO mode, then select ASIO and Dante Virtual Soundcard under Device. macOS: use CoreAudio. Network subscriptions and clock remain managed in Dante Controller.",juce::dontSendNotification);
  PulseTheme::style(note,12,false,PulseTheme::muted);note.setJustificationType(juce::Justification::topLeft);PulseTheme::style(result,12,false,PulseTheme::muted);
 #if ! JUCE_WINDOWS
  scan.setVisible(false);
 #endif
  refresh();
 }
 void refresh(){
  auto* d=e.devices.getCurrentAudioDevice();bool active=d&&d->isPlaying(),dante=active&&d->getName().containsIgnoreCase("Dante");
  driver.set("DANTE AUDIO DEVICE",dante?"Driver active":"Not verified",d?d->getName():"No active device");
  clock.set("NETWORK CLOCK","Unverified","Use Dante Controller for PTP status");
  juce::String text;
 #if JUCE_WINDOWS && JUCE_ASIO
  text<<"Windows ASIO backend: ENABLED\n";
 #elif JUCE_MAC
  text<<"CoreAudio backend available\n";
 #else
  text<<"ASIO backend not available on this platform\n";
 #endif
  if(scanned){text<<"Registered ASIO drivers: "<<discovered.size()<<"\n";for(auto& n:discovered)text<<"  "<<n<<"\n";}
  if(d){text<<"\nActive driver: "<<d->getTypeName()<<" | "<<d->getName()<<"\n";
   text<<"Sample rate: "<<juce::String(d->getCurrentSampleRate(),0)<<" Hz | Buffer: "<<d->getCurrentBufferSizeSamples()<<" samples\n";
   text<<"Device capacity: "<<d->getInputChannelNames().size()<<" in / "<<d->getOutputChannelNames().size()<<" out\n";
   auto ins=e.activeChannelNames(true),outs=e.activeChannelNames(false);
   text<<"Enabled channels: "<<ins.size()<<" in / "<<outs.size()<<" out\n";
   text<<"\nInput slots (in active-channel order):\n";for(int i=0;i<ins.size();++i)text<<juce::String(i+1)<<": "<<ins[i]<<"\n";
   text<<"\nOutput slots (master 1/2, AUX 3/4, 5/6, 7/8):\n";for(int i=0;i<outs.size();++i)text<<juce::String(i+1)<<": "<<outs[i]<<"\n";
  }else text<<"\nOpen audio devices to configure a driver.";
  if(e.deviceError.isNotEmpty()&&!active)text<<"\nError: "<<e.deviceError;
  if(diagnostics.getText()!=text)diagnostics.setText(text,false);
  bool idle=!e.enabled.load()&&!e.transport.isPlaying()&&!e.multitrack.active();scan.setEnabled(idle);save.setEnabled(idle&&active);
 }
 void resized() override {auto r=getLocalBounds().reduced(5);auto row=r.removeFromTop(100);driver.setBounds(row.removeFromLeft(row.getWidth()/2));clock.setBounds(row);row=r.removeFromTop(34);configure.setBounds(row.removeFromLeft(row.getWidth()/3).reduced(2));scan.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));save.setBounds(row.reduced(2));result.setBounds(r.removeFromTop(28));note.setBounds(r.removeFromTop(62));diagnostics.setBounds(r.reduced(2));}
};
