#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseCard.h"
#include "../UI/MeterWidget.h"
class BroadcastPage : public juce::Component {
 Engine& e;EnterpriseCard route,loudness;MeterWidget output;
public:
 explicit BroadcastPage(Engine& engine):e(engine){addAndMakeVisible(route);addAndMakeVisible(loudness);addAndMakeVisible(output);refresh();}
 void refresh(){auto* d=e.devices.getCurrentAudioDevice();route.set("STEREO OUTPUT",d&&d->isPlaying()?"Device active":"Offline","External broadcast route not verified");loudness.set("BLOCK RMS",d&&d->isPlaying()?juce::String(juce::Decibels::gainToDecibels(e.rms.load(),-100.f),1)+" dBFS":"Offline","Clipped blocks: "+juce::String((juce::int64)e.clippedBlocks.load())+" | LUFS pending",e.clippedBlocks.load()?juce::Colour(0xffe6b45c):PulseTheme::cyan);output.setPeak(e.peak.load(),"Master sample peak");}
 void resized() override {auto r=getLocalBounds().reduced(3);auto row=r.removeFromTop(98);route.setBounds(row.removeFromLeft(row.getWidth()/2));loudness.setBounds(row);output.setBounds(r);}
};
