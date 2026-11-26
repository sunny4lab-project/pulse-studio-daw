#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
namespace PulseTheme {
 inline const juce::Colour bgPrimary{0xff081018},cyan{0xff31d5d5},panel{0xff0c1623},header{0xff0b1420},border{0xff26394a},text{0xffe3edf6},muted{0xff91aabd},active{0xff123b45},button{0xff1e293b};
 inline juce::Font font(float size,bool bold=false){return juce::Font(size,bold?juce::Font::bold:juce::Font::plain);}
 inline void style(juce::Label& l,float size,bool bold=false,juce::Colour c=text){l.setFont(font(size,bold));l.setColour(juce::Label::textColourId,c);}
 inline void apply(juce::LookAndFeel& l){l.setColour(juce::Slider::thumbColourId,cyan);l.setColour(juce::TextButton::buttonColourId,button);l.setColour(juce::TextButton::textColourOffId,text);l.setColour(juce::TextButton::textColourOnId,text);l.setColour(juce::ComboBox::backgroundColourId,panel);l.setColour(juce::ComboBox::outlineColourId,border);l.setColour(juce::Label::textColourId,text);l.setColour(juce::ListBox::backgroundColourId,panel);}
}
