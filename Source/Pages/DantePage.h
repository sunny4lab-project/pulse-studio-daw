#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseCard.h"
class DantePage : public juce::Component {
 Engine& e;EnterpriseCard driver,clock;juce::TextButton configure{"Open audio devices"};juce::Label note;
public:
 std::function<void()> onConfigure;
 explicit DantePage(Engine& engine):e(engine){for(auto* x:std::array<juce::Component*,4>{&driver,&clock,&configure,&note})addAndMakeVisible(x);configure.onClick=[this]{if(onConfigure)onConfigure();};note.setText("Select an installed Dante driver in audio devices. Network discovery, PTP clock and subscriptions require native integration.",juce::dontSendNotification);PulseTheme::style(note,12,false,PulseTheme::muted);note.setJustificationType(juce::Justification::topLeft);refresh();}
 void refresh(){auto* d=e.devices.getCurrentAudioDevice();bool active=d&&d->isPlaying()&&d->getName().containsIgnoreCase("Dante");driver.set("DANTE AUDIO DEVICE",active?"Driver active":"Not verified",d?d->getName():"No active device");clock.set("NETWORK CLOCK","Unverified","No Dante PTP telemetry");}
 void resized() override {auto r=getLocalBounds().reduced(5);auto row=r.removeFromTop(108);driver.setBounds(row.removeFromLeft(row.getWidth()/2));clock.setBounds(row);configure.setBounds(r.removeFromTop(36).removeFromLeft(210).reduced(3));note.setBounds(r.removeFromTop(85));}
};
