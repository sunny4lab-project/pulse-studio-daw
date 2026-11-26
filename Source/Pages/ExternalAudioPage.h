#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseCard.h"
class ExternalAudioPage : public juce::Component {
 Engine& e;EnterpriseCard device,io;juce::TextButton configure{"Configure audio device"},effect{"Load local effect"},bypass{"Toggle bypass"};juce::Label note;
public:
 std::function<void()> onConfigure,onEffect,onBypass;
 explicit ExternalAudioPage(Engine& engine):e(engine){for(auto* c:std::array<juce::Component*,6>{&device,&io,&configure,&effect,&bypass,&note})addAndMakeVisible(c);configure.onClick=[this]{if(onConfigure)onConfigure();};effect.onClick=[this]{if(onEffect)onEffect();};bypass.onClick=[this]{if(onBypass)onBypass();};note.setText("Use an installed SoundGrid audio driver or external interface. Local VST3/AU effects are not SoundGrid server plugins. Native server discovery and SuperRack control are not implemented.",juce::dontSendNotification);PulseTheme::style(note,12,false,PulseTheme::muted);note.setJustificationType(juce::Justification::topLeft);}
 void refresh(){auto* d=e.devices.getCurrentAudioDevice();bool ready=d&&d->isPlaying();bool sg=ready&&d->getName().containsIgnoreCase("SoundGrid");device.set("SOUNDGRID AUDIO",sg?"Driver active":"Unverified",d?d->getName():"No active audio device");io.set("ACTIVE CHANNELS",ready?juce::String(d->getActiveInputChannels().countNumberOfSetBits())+" in / "+juce::String(d->getActiveOutputChannels().countNumberOfSetBits())+" out":"Offline","Current local device");bypass.setButtonText(e.pluginBypass.load()?"Enable master 1":"Bypass master 1");}
 void resized() override {auto r=getLocalBounds().reduced(4);auto cards=r.removeFromTop(100);device.setBounds(cards.removeFromLeft(cards.getWidth()/2));io.setBounds(cards);auto row=r.removeFromTop(34);for(auto* b:{&configure,&effect,&bypass}){int n=b==&configure?3:b==&effect?2:1;b->setBounds(row.removeFromLeft(row.getWidth()/n).reduced(2));}note.setBounds(r.removeFromTop(90).reduced(4));}
};
