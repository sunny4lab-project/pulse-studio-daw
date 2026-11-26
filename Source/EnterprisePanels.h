#pragma once
#include "MixerStrip.h"

// A matrix cell selects the channel's one existing stereo summing bus.
// This is not a multi-send or hardware-output patch matrix.
class RoutingMatrix : public Component {
 OwnedArray<Strip>& strips;
 std::array<String,B>& names;
 int channelPage=0,busPage=0;
 TextButton prevCh{"< Inputs"},nextCh{"Inputs >"},prevBus{"< Buses"},nextBus{"Buses >"};
 Rectangle<int> grid() const {return getLocalBounds().reduced(6).withTrimmedTop(56).withTrimmedLeft(90);}
public:
 RoutingMatrix(OwnedArray<Strip>& s,std::array<String,B>& n):strips(s),names(n){
  for(auto* x:{&prevCh,&nextCh,&prevBus,&nextBus})addAndMakeVisible(x);
  prevCh.onClick=[this]{channelPage=std::max(0,channelPage-1);repaint();};
  nextCh.onClick=[this]{channelPage=std::min((N-1)/8,channelPage+1);repaint();};
  prevBus.onClick=[this]{busPage=std::max(0,busPage-1);repaint();};
  nextBus.onClick=[this]{busPage=std::min((B-1)/8,busPage+1);repaint();};
 }
 void resized() override {auto r=getLocalBounds().reduced(6).removeFromTop(24);for(auto* x:{&prevCh,&nextCh,&prevBus,&nextBus}){x->setBounds(r.removeFromLeft(std::min(84,r.getWidth())).reduced(1));}}
 void paint(Graphics& g) override {
  g.fillAll(Colour(0xff0c1623));auto q=grid();if(q.getWidth()<8||q.getHeight()<8)return;
  g.setFont(11.f);g.setColour(Colour(0xff99afc1));g.drawFittedText("Channel to bus | click to assign",6,28,getWidth()-12,22,Justification::centredLeft,1);
  for(int j=0;j<8;++j){int x=q.getX()+q.getWidth()*j/8,w=q.getWidth()*(j+1)/8-q.getWidth()*j/8;g.drawFittedText(names[busPage*8+j],x,q.getY()-14,w,14,Justification::centred,1);}
  for(int i=0;i<8&&channelPage*8+i<strips.size();++i){auto* s=strips[channelPage*8+i];int y=q.getY()+q.getHeight()*i/8,h=q.getHeight()*(i+1)/8-q.getHeight()*i/8;g.setColour(Colour(0xffc2d3df));g.drawFittedText(s->name.getText(),7,y,84,h,Justification::centredLeft,1);
   for(int j=0;j<8;++j){int x=q.getX()+q.getWidth()*j/8,w=q.getWidth()*(j+1)/8-q.getWidth()*j/8;auto cell=Rectangle<int>(x,y,w,h).reduced(2);bool active=s->c.bus.load()==busPage*8+j;g.setColour(active?Colour(0xff168b8e):Colour(0xff172736));g.fillRoundedRectangle(cell.toFloat(),3);if(active){g.setColour(Colours::white);g.drawText("ON",cell,Justification::centred);}}
  }
 }
 void mouseDown(const MouseEvent& m) override {auto q=grid();if(!q.contains(m.getPosition())||q.isEmpty())return;int i=channelPage*8+(m.y-q.getY())*8/q.getHeight(),b=busPage*8+(m.x-q.getX())*8/q.getWidth();if(i<strips.size()&&b<B){strips[i]->group.setSelectedId(b+1,sendNotificationSync);repaint();}}
};

class ScenePanel : public Component {
 struct Scene {bool valid=false;std::array<float,N> gain{},pan{};std::array<bool,N> mute{};std::array<float,B> bus{};float master=0.5f;};
 std::array<Scene,4> scenes;ComboBox slot;TextButton capture{"Capture mix"},recall{"Recall safely"};Label info;
public:
 std::function<void(int)> onCapture,onRecall;
 ScenePanel(){for(int i=0;i<4;++i)slot.addItem("Scene "+String(i+1),i+1);slot.setSelectedId(1);for(auto* x:std::array<Component*,4>{&slot,&capture,&recall,&info})addAndMakeVisible(x);info.setText("Session-only mix scenes. Recall disables live inputs.",dontSendNotification);capture.onClick=[this]{if(onCapture)onCapture(slot.getSelectedId()-1);};recall.onClick=[this]{if(onRecall)onRecall(slot.getSelectedId()-1);};}
 void captureMix(int i,Engine& e){if(i<0||i>=4)return;auto& s=scenes[i];s.valid=true;for(int c=0;c<N;++c){s.gain[c]=e.ch[c].gain.load();s.pan[c]=e.ch[c].pan.load();s.mute[c]=e.ch[c].mute.load();}for(int b=0;b<B;++b)s.bus[b]=e.busGain[b].load();s.master=e.master.load();info.setText("Captured Scene "+String(i+1),dontSendNotification);}
 bool recallMix(int i,OwnedArray<Strip>& strips,std::array<Slider,B>& buses,Slider& master){if(i<0||i>=4||!scenes[i].valid){info.setText("Capture a scene first.",dontSendNotification);return false;}auto& s=scenes[i];for(int c=0;c<strips.size();++c){auto* x=strips[c];x->gain.setValue(Decibels::gainToDecibels(s.gain[c],-60.f),sendNotificationSync);x->pan.setValue(s.pan[c],sendNotificationSync);x->mute.setToggleState(s.mute[c],dontSendNotification);x->c.mute=s.mute[c];x->solo.setToggleState(false,dontSendNotification);x->c.solo=false;}for(int b=0;b<B;++b)buses[b].setValue(Decibels::gainToDecibels(s.bus[b],-60.f),sendNotificationSync);master.setValue(Decibels::gainToDecibels(s.master,-60.f),sendNotificationSync);info.setText("Scene recalled; live inputs remain off.",dontSendNotification);return true;}
 void resized() override {auto r=getLocalBounds().reduced(10);slot.setBounds(r.removeFromTop(32));r.removeFromTop(8);auto row=r.removeFromTop(30);capture.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));recall.setBounds(row.reduced(2));info.setBounds(r.removeFromTop(40));}
};
