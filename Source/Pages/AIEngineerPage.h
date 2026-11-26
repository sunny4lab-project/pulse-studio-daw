#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseCard.h"
class AIEngineerPage : public juce::Component {
 Engine& e;EnterpriseCard modeCard,guard;
public:
 explicit AIEngineerPage(Engine& engine):e(engine){addAndMakeVisible(modeCard);addAndMakeVisible(guard);}
 void refresh(int mode){bool solo=false;for(auto& c:e.ch)solo|=c.solo.load();modeCard.set("MIXING MODE",mode==1?"Manual":mode==2?"Assisted":"Auto level","Rule-based peak matching, not trained AI");bool ready=e.enabled.load()&&e.devices.getCurrentAudioDevice()!=nullptr&&!solo&&e.peak.load()<0.9f;guard.set("AUTOMATION GUARD",ready?"Ready":"Paused",ready?"Channel locks and level bounds apply":"Live off, device absent, solo or high peak",ready?PulseTheme::cyan:juce::Colour(0xffe8bc62));}
 void resized() override {auto r=getLocalBounds();modeCard.setBounds(r.removeFromLeft(r.getWidth()/2));guard.setBounds(r);}
};
