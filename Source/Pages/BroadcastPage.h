#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseCard.h"
#include "../UI/MeterWidget.h"
class BroadcastPage : public juce::Component {
 Engine& e;EnterpriseCard route,loudness;MeterWidget output;
public:
 explicit BroadcastPage(Engine& engine):e(engine){addAndMakeVisible(route);addAndMakeVisible(loudness);addAndMakeVisible(output);refresh();}
 void refresh(){auto* d=e.devices.getCurrentAudioDevice();route.set("STEREO OUTPUT",d&&d->isPlaying()?"Device active":"Offline","External broadcast route not verified");float i=e.integratedLufs.load();loudness.set("INTEGRATED LOUDNESS",d&&d->isPlaying()&&i>-99?juce::String(i,1)+" LUFS":"--","Estimated intersample peak "+juce::String(juce::Decibels::gainToDecibels(e.estimatedTruePeak.load(),-100.f),1)+" dB | GR "+juce::String(e.limiterReduction.load(),1)+" dB",PulseTheme::cyan);output.setPeak(e.peak.load(),"Master sample peak");}
 void resized() override {auto r=getLocalBounds().reduced(3);auto row=r.removeFromTop(98);route.setBounds(row.removeFromLeft(row.getWidth()/2));loudness.setBounds(row);output.setBounds(r);}
};
