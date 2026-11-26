#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseCard.h"
#include "../UI/MeterWidget.h"
class BroadcastPage : public juce::Component {
 Engine& e;EnterpriseCard route,loudness;MeterWidget output;
public:
 explicit BroadcastPage(Engine& engine):e(engine){addAndMakeVisible(route);addAndMakeVisible(loudness);addAndMakeVisible(output);refresh();}
 void refresh(){auto* d=e.devices.getCurrentAudioDevice();route.set("STEREO OUTPUT",d&&d->isPlaying()?"Device active":"Offline","External broadcast route not verified");loudness.set("LOUDNESS","Not measured","Integrated LUFS / true peak pending",PulseTheme::muted);output.setPeak(e.peak.load(),"Master sample peak");}
 void resized() override {auto r=getLocalBounds().reduced(3);auto row=r.removeFromTop(98);route.setBounds(row.removeFromLeft(row.getWidth()/2));loudness.setBounds(row);output.setBounds(r);}
};
