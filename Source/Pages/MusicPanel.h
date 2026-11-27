#pragma once
#include "../TimelineSource.h"
#include "../UI/EnterpriseTheme.h"
#include <array>
class MusicPanel final : public juce::Component,private juce::MidiInputCallback,private juce::Timer {
 using Frame=pulse::Frame;
 pulse::TimelineTrack track;std::function<bool(const pulse::TimelineTrack&)> applyTrack;
 juce::ComboBox instrument,mode,device,octave;
 juce::Slider length,velocity,grid;
 juce::TextButton apply{"Apply to track"},clear{"Clear lane"},record{"Record MIDI"},stop{"Stop MIDI"};
 juce::Label title,status;juce::Array<juce::MidiDeviceInfo> devices;
 std::unique_ptr<juce::MidiInput> input;juce::CriticalSection mutex;
 struct Pending{Frame start=-1;float velocity=0;};std::array<Pending,16*128> held{};
 std::vector<pulse::MidiNote> recorded;double began=0;Frame recordBase=0;bool capturing=false;
 Frame pageStart=0;juce::TextButton prev{"< 8 sec"},next{"> 8 sec"};
 static constexpr Frame pageFrames=384000;static constexpr int rows=36;
 class Grid final:public juce::Component{
  MusicPanel& p;
 public:
  explicit Grid(MusicPanel& panel):p(panel){}
  void paint(juce::Graphics& g)override{p.draw(g,getLocalBounds());}
  void mouseDown(const juce::MouseEvent& e)override{p.click(e,getLocalBounds());}
 } canvas{*this};
 int bottomNote()const{return (octave.getSelectedId()-1)*12;}
 bool automation()const{return mode.getSelectedId()>1;}
 std::vector<pulse::AutomationPoint>& points(){return mode.getSelectedId()==2?track.gainAutomation:track.panAutomation;}
 void message(juce::String s){status.setText(s,juce::dontSendNotification);}
 void draw(juce::Graphics& g,juce::Rectangle<int> r){
  g.fillAll(PulseTheme::bgPrimary);auto plot=r.withTrimmedLeft(52).reduced(1);
  auto xAt=[&](Frame f){return (float)(plot.getX()+(double)(f-pageStart)/pageFrames*plot.getWidth());};
  if(!automation()){for(int i=0;i<rows;++i){int pitch=bottomNote()+rows-1-i;float y=plot.getY()+i*plot.getHeight()/(float)rows,h=plot.getHeight()/(float)rows;
   bool black=pitch%12==1||pitch%12==3||pitch%12==6||pitch%12==8||pitch%12==10;g.setColour(black?juce::Colour(0xff192733):juce::Colour(0xff263747));g.fillRect(juce::Rectangle<float>(0,y,51,h-1));g.setColour(PulseTheme::muted);g.setFont(10.f);g.drawText(juce::MidiMessage::getMidiNoteName(pitch,true,true,3),2,(int)y,49,(int)h,juce::Justification::centredLeft);g.setColour(PulseTheme::border);g.drawHorizontalLine((int)y,(float)plot.getX(),(float)plot.getRight());}}
  for(int i=0;i<=16;++i){float x=plot.getX()+plot.getWidth()*i/16.f;g.setColour(PulseTheme::border);g.drawVerticalLine((int)x,(float)plot.getY(),(float)plot.getBottom());}
  if(automation()){auto& list=points();float low=mode.getSelectedId()==2?0.f:-1.f,high=mode.getSelectedId()==2?4.f:1.f;
   auto yAt=[&](float v){return plot.getBottom()-(v-low)/(high-low)*plot.getHeight();};
   g.setColour(PulseTheme::cyan);float y=yAt(pulse::automationAt(list,pageStart,mode.getSelectedId()==2?track.gain:track.pan));float x=(float)plot.getX();
   for(auto& a:list)if(a.frame>=pageStart&&a.frame<=pageStart+pageFrames){float xx=xAt(a.frame),yy=yAt(a.value);g.drawLine(x,y,xx,yy,2);g.fillEllipse(xx-3,yy-3,6,6);x=xx;y=yy;}
   g.drawLine(x,y,(float)plot.getRight(),yAt(pulse::automationAt(list,pageStart+pageFrames,mode.getSelectedId()==2?track.gain:track.pan)),2);
   g.setColour(PulseTheme::muted);g.drawText(juce::String(high,0),2,0,48,20,juce::Justification::centred);g.drawText(juce::String(low,0),2,r.getBottom()-20,48,20,juce::Justification::centred);
  }else{g.setColour(PulseTheme::cyan);for(auto& n:track.notes){if(n.pitch<bottomNote()||n.pitch>=bottomNote()+rows||n.start+n.length<=pageStart||n.start>=pageStart+pageFrames)continue;
   float y=plot.getY()+(bottomNote()+rows-1-n.pitch)*plot.getHeight()/(float)rows,h=plot.getHeight()/(float)rows;
   float a=std::max((float)plot.getX(),xAt(n.start)),b=std::min((float)plot.getRight(),xAt(n.start+n.length));g.fillRoundedRectangle(a,y+1,std::max(1.f,b-a),std::max(1.f,h-2),2);}}
 }
 void click(const juce::MouseEvent& ev,juce::Rectangle<int> r){
  if(capturing)return;auto plot=r.withTrimmedLeft(52).reduced(1);if(!plot.contains(ev.getPosition()))return;
  Frame at=pageStart+(Frame)((double)(ev.x-plot.getX())/plot.getWidth()*pageFrames);
  Frame step=(Frame)std::llround(grid.getValue()*48000);if(step>0)at=(at/step)*step;
  if(automation()){auto& a=points();if(ev.mods.isRightButtonDown()){auto it=std::min_element(a.begin(),a.end(),[&](auto& x,auto& y){return std::abs(x.frame-at)<std::abs(y.frame-at);});if(it!=a.end())a.erase(it);}else{float v=1.f-(float)(ev.y-plot.getY())/plot.getHeight();pulse::putAutomation(a,at,mode.getSelectedId()==2?v*4.f:2*v-1);}}
  else{int pitch=bottomNote()+rows-1-juce::jlimit(0,rows-1,(ev.y-plot.getY())*rows/plot.getHeight());auto& notes=track.notes;
   if(ev.mods.isRightButtonDown()){auto it=std::find_if(notes.begin(),notes.end(),[&](auto& n){return n.pitch==pitch&&n.start<=at&&at<n.start+n.length;});if(it!=notes.end())notes.erase(it);}
   else if(at<pulse::timelineLimit){Frame n=std::min((Frame)(length.getValue()*48000),pulse::timelineLimit-at);if(n>0)notes.push_back({at,n,pitch,(float)velocity.getValue()});}}
  canvas.repaint();
 }
 void handleIncomingMidiMessage(juce::MidiInput*,const juce::MidiMessage& m)override{
  const juce::ScopedLock lock(mutex);if(!capturing||(!m.isNoteOn()&&!m.isNoteOff()))return;
  Frame at=recordBase+std::max<Frame>(0,(Frame)((juce::Time::getMillisecondCounterHiRes()-began)*48.0));at=std::min(at,pulse::timelineLimit-1);
  int index=(m.getChannel()-1)*128+m.getNoteNumber();auto& h=held[(size_t)index];
  if(h.start>=0&&at>h.start)recorded.push_back({h.start,at-h.start,m.getNoteNumber(),h.velocity});
  h.start=-1;if(m.isNoteOn()){h.start=at;h.velocity=m.getFloatVelocity();}
 }
 void finishCapture(){
  if(input){input->stop();input.reset();}
  {const juce::ScopedLock lock(mutex);if(!capturing)return;Frame at=std::min(pulse::timelineLimit,recordBase+std::max<Frame>(1,(Frame)((juce::Time::getMillisecondCounterHiRes()-began)*48.0)));
   for(int i=0;i<(int)held.size();++i)if(held[i].start>=0&&at>held[i].start)recorded.push_back({held[i].start,at-held[i].start,i%128,held[i].velocity});
   track.notes.insert(track.notes.end(),recorded.begin(),recorded.end());recorded.clear();capturing=false;}
  canvas.repaint();message("MIDI captured. Apply to track to hear it in arrangement playback.");
 }
 void beginCapture(){
  if(capturing)return;int id=device.getSelectedId()-1;if(!juce::isPositiveAndBelow(id,devices.size())){message("Select a MIDI input.");return;}
  input=juce::MidiInput::openDevice(devices[id].identifier,this);if(!input){message("Could not open MIDI input.");return;}
  {const juce::ScopedLock lock(mutex);for(auto& h:held)h=Pending{};recorded.clear();began=juce::Time::getMillisecondCounterHiRes();recordBase=pageStart;capturing=true;}input->start();message("Recording note events from page start (no live monitoring).");
 }
 void timerCallback()override{record.setEnabled(!capturing);stop.setEnabled(capturing);apply.setEnabled(!capturing);clear.setEnabled(!capturing);prev.setEnabled(!capturing);next.setEnabled(!capturing);}
public:
 MusicPanel(pulse::TimelineTrack t,std::function<bool(const pulse::TimelineTrack&)> fn):track(std::move(t)),applyTrack(std::move(fn)){
  for(auto* c:std::array<juce::Component*,15>{&instrument,&mode,&device,&octave,&length,&velocity,&grid,&apply,&clear,&record,&stop,&title,&status,&prev,&next})addAndMakeVisible(c);addAndMakeVisible(canvas);
  title.setText("PIANO ROLL / TRACK AUTOMATION",juce::dontSendNotification);PulseTheme::style(title,17,true);PulseTheme::style(status,12,false,PulseTheme::muted);
  int id=1;for(auto* s:{"Sine","Electric keys","Organ","Pluck"})instrument.addItem(s,id++);instrument.setSelectedId(track.instrument+1,juce::dontSendNotification);instrument.onChange=[this]{track.instrument=instrument.getSelectedId()-1;};
  mode.addItem("Piano roll",1);mode.addItem("Gain automation",2);mode.addItem("Pan automation",3);mode.setSelectedId(1,juce::dontSendNotification);mode.onChange=[this]{canvas.repaint();};
  for(int i=0;i<=7;++i)octave.addItem("Pitch "+juce::String(i*12)+"–"+juce::String(i*12+35),i+1);octave.setSelectedId(5,juce::dontSendNotification);octave.onChange=[this]{canvas.repaint();};
  devices=juce::MidiInput::getAvailableDevices();for(int i=0;i<devices.size();++i)device.addItem(devices[i].name,i+1);device.setTextWhenNothingSelected("MIDI input");if(!devices.isEmpty())device.setSelectedId(1,juce::dontSendNotification);
  auto setup=[](juce::Slider& s,double lo,double hi,double step,double value,const char* suffix){s.setSliderStyle(juce::Slider::LinearHorizontal);s.setRange(lo,hi,step);s.setValue(value);s.setTextValueSuffix(suffix);s.setTextBoxStyle(juce::Slider::TextBoxRight,false,75,22);};
  setup(length,.0625,4,.0625,.5," s note");setup(velocity,.05,1,.05,.8," vel");setup(grid,.0625,1,.0625,.125," s grid");
  prev.onClick=[this]{pageStart=std::max<Frame>(0,pageStart-pageFrames);canvas.repaint();message("Page starts at "+juce::String(pageStart/48000)+" s");};
  next.onClick=[this]{pageStart=std::min(pulse::timelineLimit-pageFrames,pageStart+pageFrames);canvas.repaint();message("Page starts at "+juce::String(pageStart/48000)+" s");};
  clear.onClick=[this]{if(automation())points().clear();else track.notes.clear();canvas.repaint();};
  apply.onClick=[this]{if(applyTrack&&applyTrack(track))message("Applied to arrangement. Play in Timeline.");else message("Stop audio before applying changes.");};
  record.onClick=[this]{beginCapture();};stop.onClick=[this]{finishCapture();};
  message("Left-click add; right-click remove. Apply changes, then play. 8-second pages.");setSize(850,650);startTimerHz(5);
 }
 ~MusicPanel()override{stopTimer();finishCapture();}
 void resized()override{auto r=getLocalBounds().reduced(8);title.setBounds(r.removeFromTop(30));auto row=r.removeFromTop(32);int n=4;for(auto* b:{&mode,&instrument,&octave,&device})b->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2));
  row=r.removeFromTop(30);n=3;for(auto* s:{&length,&velocity,&grid})s->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2));
  row=r.removeFromTop(32);n=6;for(auto* b:{&prev,&next,&record,&stop,&clear,&apply})b->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2));status.setBounds(r.removeFromBottom(26));canvas.setBounds(r);}
};
