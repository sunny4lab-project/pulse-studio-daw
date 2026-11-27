#pragma once
#include "AudioEngine.h"
#include "AutoMixControl.h"
#include "ChannelColourUI.h"
struct ColourNameLabel : juce::Label {
 std::function<void()> openColours;
 void mouseDown(const juce::MouseEvent& ev) override {
  if(ev.mods.isPopupMenu()){if(openColours)openColours();return;}
  juce::Label::mouseDown(ev);
 }
};
class Strip : public Component {
public:
 bool detailed=false; Channel& c; ColourNameLabel name;Label panText; ComboBox group, input, rightInput, role; Slider gain,pan,hp,ratio; TextButton lock{"Lock"}, apply{"Apply"}; Label advice; float anchor=0.25f, envelope=0; int currentMode=1,currentProfile=1;bool speechPresent=false,manualOverride=false; TextButton mute{"M"},solo{"S"};
 Strip(Channel& s,int i):c(s){name.openColours=[this]{openChannelColourMenu(name,c);};name.setText("IN "+String(i+1),dontSendNotification); name.setEditable(false,true,false); for(int b=0;b<B;++b)group.addItem(busName(b),b+1);group.setSelectedId(4);group.onChange=[this]{c.bus=group.getSelectedId()-1;};addAndMakeVisible(group);input.addItem("None",1);input.setSelectedId(1,dontSendNotification);input.onChange=[this]{c.source=input.getSelectedId()-2;};addAndMakeVisible(input);rightInput.addItem("Mono",1);rightInput.setSelectedId(1);rightInput.onChange=[this]{c.rightSource=rightInput.getSelectedId()-2;};addAndMakeVisible(rightInput);for(auto label:{"General","Lead vocal","Speech","Backing vocal","Drums","Bass","Keys / guitar"})role.addItem(label,role.getNumItems()+1);role.setSelectedId(1);role.onChange=[this]{c.mixRole=role.getSelectedId();repaint();};addAndMakeVisible(role); name.setFont(Font(12.f,Font::bold));name.setColour(Label::textColourId,Colour(0xff081018));name.setJustificationType(Justification::centred);panText.setFont(Font(10.f));panText.setJustificationType(Justification::centred);panText.setColour(Label::textColourId,Colour(0xff92a9bb));addAndMakeVisible(panText); addAndMakeVisible(name); for(auto* b:{&mute,&solo}){addAndMakeVisible(b);b->setClickingTogglesState(true);} mute.setColour(TextButton::buttonOnColourId,Colour(0xffa83240)); solo.setColour(TextButton::buttonOnColourId,Colour(0xff967322)); mute.setToggleState(true,dontSendNotification); mute.onClick=[this]{c.mute=mute.getToggleState();}; solo.onClick=[this]{c.solo=solo.getToggleState();}; gain.setSliderStyle(Slider::LinearVertical); gain.setRange(-60,3.52,0.01);gain.setSkewFactorFromMidPoint(-12);gain.setValue(Decibels::gainToDecibels(c.gain.load()));gain.setNumDecimalPlacesToDisplay(1);gain.setTextBoxStyle(Slider::TextBoxBelow,false,48,17);gain.onValueChange=[this]{c.gain=Decibels::decibelsToGain((float)gain.getValue(),-60.f);anchor=c.gain.load();if(currentMode==3||currentMode==4){manualOverride=true;lock.setToggleState(true,dontSendNotification);}}; lock.setClickingTogglesState(true);lock.onClick=[this]{if(!lock.getToggleState())manualOverride=false;}; lock.setColour(TextButton::buttonOnColourId,Colour(0xff967322)); addAndMakeVisible(lock); addAndMakeVisible(apply); addAndMakeVisible(advice); advice.setFont(Font(11.f)); advice.setJustificationType(Justification::centred); apply.onClick=[this]{if(apply.isEnabled() && anchor>=0.001f) setAuto(suggest());}; pan.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);pan.setRange(-1,1,0.01);pan.setValue(0);pan.setTextBoxStyle(Slider::NoTextBox,false,0,0);pan.onValueChange=[this]{c.pan=(float)pan.getValue();};addAndMakeVisible(gain);addAndMakeVisible(pan); for(auto* k:{&hp,&ratio}){k->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);k->setTextBoxStyle(Slider::TextBoxBelow,false,65,16);addAndMakeVisible(k);} hp.setRange(0,250,1);hp.setTextValueSuffix(" Hz");hp.onValueChange=[this]{c.hp=(float)hp.getValue();};ratio.setRange(1,8,0.1);ratio.setValue(1);ratio.setTextValueSuffix(":1");ratio.onValueChange=[this]{c.ratio=(float)ratio.getValue();};}
 float suggest() const {return pulse::AutoMixControl::propose(anchor,envelope,role.getSelectedId(),currentProfile,speechPresent);}
 void setAuto(float v){c.gain=v; gain.setValue(Decibels::gainToDecibels(v,-60.f),dontSendNotification);}
 void updateAuto(int mode,bool active,int profile,bool speechActive){
  currentMode=mode;currentProfile=profile;speechPresent=speechActive;
  float p=c.pan.load();panText.setText(std::abs(p)<0.01f?String("CENTER"):(p<0?String("L "):String("R "))+String((int)std::round(std::abs(p)*100)),dontSendNotification);
  envelope=.85f*envelope+.15f*c.peak.load();
  bool eligible=active&&!c.mute.load()&&!lock.getToggleState()&&anchor>=.001f&&envelope>.0032f&&c.peak.load()>.0032f;
  apply.setEnabled((mode==2||mode==4)&&eligible);
  if(!eligible||mode==1){advice.setText(manualOverride?"Manual hold":lock.getToggleState()?"Locked":c.mute.load()?"Muted":!active?"Paused":mode==1?"Manual":anchor<.001f?"Fader off":"No signal",dontSendNotification);return;}
  float target=suggest(),current=std::max(.001f,c.gain.load());
  float delta=20.f*std::log10(target/current);
  advice.setText((mode==3?String("Auto "):String())+String(delta,1)+" dB",dontSendNotification);
  if((mode==3||mode==4)&&!gain.isMouseButtonDown()){
   auto next=pulse::AutoMixControl::ride(current,target,mode==4);
   if(std::abs(next-current)>.000001f)setAuto(next);
  }
 }
 StringArray displayedInputs;
 bool inputsInitialised=false;
 void refreshInputs(const StringArray& names,bool force=false){
  if(!force&&inputsInitialised&&names==displayedInputs)return;
  inputsInitialised=true;displayedInputs=names;int count=names.size(),l=c.source.load(),r=c.rightSource.load();
  input.clear(dontSendNotification);rightInput.clear(dontSendNotification);
  input.addItem("None",1);rightInput.addItem("Mono",1);
  for(int j=0;j<count;++j){input.addItem(names[j],j+2);rightInput.addItem("R: "+names[j],j+2);}
  if(l>=count)input.addItem("Input slot "+String(l+1)+" (inactive)",l+2);
  if(r>=count)rightInput.addItem("Input slot "+String(r+1)+" (inactive)",r+2);
  input.setSelectedId(l+2,dontSendNotification);rightInput.setSelectedId(r+2,dontSendNotification);
 }
 void setDetailed(bool v){detailed=v;for(auto* x:std::array<Component*,6>{&input,&rightInput,&role,&group,&hp,&ratio})x->setVisible(v);resized();repaint();}
 void paint(Graphics& g) override {
  auto bounds=getLocalBounds().toFloat().reduced(1);g.setColour(Colour(0xff13202c));g.fillRoundedRectangle(bounds,5);g.setColour(Colour(0xff2a3e50));g.drawRoundedRectangle(bounds,5,1);
  g.setColour(getChannelColour(c));g.fillRoundedRectangle(bounds.removeFromTop(25),4);if(c.recordArm.load()){g.setColour(Colour(0xfff35d67));g.fillEllipse(5.f,7.f,7.f,7.f);}
  auto meter=Rectangle<int>(getWidth()-19,gain.getY()+10,7,std::max(1,gain.getHeight()-37));
  for(int k=0;k<2;++k){auto r=meter.translated(k*8,0);g.setColour(Colour(0xff10151b));g.fillRect(r);float p=k?c.postPeak.load():c.peak.load();float v=jlimit(0.f,1.f,(Decibels::gainToDecibels(p,-60.f)+60.f)/60.f);int segments=std::max(1,r.getHeight()/6);for(int j=0;j<segments;++j){auto cell=r.removeFromBottom(r.getHeight()/(segments-j)).reduced(0,1);g.setColour(j<v*segments?(j>=segments*0.95f?Colour(0xffe25a61):j>=segments*0.8f?Colour(0xffe8bc62):k?Colour(0xff7caaff):Colour(0xff27d6ad)):Colour(0xff253746));g.fillRect(cell);}}
 }
 void resized() override {
  auto r=getLocalBounds().reduced(4);name.setBounds(r.removeFromTop(23));
  if(detailed){input.setBounds(r.removeFromTop(22));rightInput.setBounds(r.removeFromTop(22));role.setBounds(r.removeFromTop(22));group.setBounds(r.removeFromTop(22));}
  pan.setBounds(r.removeFromTop(30));panText.setBounds(r.removeFromTop(13));
  if(detailed){auto knobs=r.removeFromTop(48);hp.setBounds(knobs.removeFromLeft(knobs.getWidth()/2));ratio.setBounds(knobs);}
  auto buttons=r.removeFromBottom(21);mute.setBounds(buttons.removeFromLeft(buttons.getWidth()/2).reduced(1));solo.setBounds(buttons.reduced(1));
  auto tools=r.removeFromBottom(19);lock.setBounds(tools.removeFromLeft(tools.getWidth()/2).reduced(1));apply.setBounds(tools.reduced(1));advice.setBounds(r.removeFromBottom(15));
  r.removeFromRight(18);gain.setBounds(r);
 }
};
