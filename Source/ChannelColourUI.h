#pragma once
#include "AudioEngine.h"

inline juce::Colour getChannelColour(const Channel& channel) {
 auto custom=channel.customColour.load();
 return juce::Colour(channel.automaticColour.load()||!pulse::validChannelColour(custom)
  ?pulse::defaultChannelColour(channel.mixRole.load(),channel.bus.load()):custom);
}

// Use a safe component reference: removing a channel while a popup is open
// must not leave the asynchronous menu callback with a dangling Channel.
inline void openChannelColourMenu(juce::Component& target,Channel& channel) {
 juce::PopupMenu menu;
 menu.addItem(1,"Automatic by channel role",true,channel.automaticColour.load());
 menu.addSeparator();
 const char* names[]={"Gold / Speech","Coral / Lead","Purple / BGV","Blue / Keys",
  "Green / Bass","Orange / Guitar","Crimson / Drums","Teal / Instruments",
  "Silver","Yellow","Pink","Sky Blue"};
 for(int i=0;i<pulse::channelPaletteSize;++i)
  menu.addItem(i+2,names[i],true,!channel.automaticColour.load()&&channel.customColour.load()==pulse::channelPalette[i]);
 juce::Component::SafePointer<juce::Component> safeTarget(&target);
 menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&target),
  [safeTarget,&channel](int choice){if(!safeTarget||choice==0)return;
   if(choice==1)channel.automaticColour=true;
   else if(choice>=2&&choice<2+pulse::channelPaletteSize){channel.customColour=pulse::channelPalette[choice-2];channel.automaticColour=false;}
   safeTarget->repaint();});
}
