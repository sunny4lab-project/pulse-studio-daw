#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include <algorithm>
#include "Dsp.h"
#include <atomic>
#include <cmath>
using namespace juce;
constexpr int N=32;
struct Channel { std::atomic<float> gain{0.25f}, pan{0}, peak{0}, postPeak{0}, hp{0}, threshold{-18}, ratio{1}; pulse::StripDSP dsp; std::atomic<bool> mute{true}, solo{false}; };
class Engine : public AudioIODeviceCallback {
public:
 std::array<Channel,N> ch; std::atomic<float> master{0.5f}, peak{0}; std::atomic<bool> enabled{false};
 AudioDeviceManager devices; AudioFormatManager formats; AudioTransportSource transport; TimeSliceThread disk{"Recording disk"};
 std::unique_ptr<AudioFormatReaderSource> reader; std::unique_ptr<AudioFormatWriter::ThreadedWriter> writer; CriticalSection recordLock; std::atomic<unsigned> missedWrites{0}; std::atomic<double> rate{48000}; AudioBuffer<float> playback;
 String deviceError; bool started=false;
 Engine(){formats.registerBasicFormats(); disk.startThread();}
 void startAudio(){if(started)return; started=true; deviceError=devices.initialise(0,2,nullptr,true); devices.addAudioCallback(this); }
 ~Engine() override {devices.removeAudioCallback(this); stopRecord(); transport.setSource(nullptr); disk.stopThread(2000);}
 void audioDeviceAboutToStart(AudioIODevice* d) override {rate=d->getCurrentSampleRate(); for(auto& c:ch)c.dsp.reset(); playback.setSize(2,d->getCurrentBufferSizeSamples()); transport.prepareToPlay(d->getCurrentBufferSizeSamples(),rate);}
 void audioDeviceStopped() override {transport.releaseResources();}
 void audioDeviceIOCallbackWithContext(const float* const* in,int ni,float* const* out,int no,int n,const AudioIODeviceCallbackContext&) override {
  for(int c=0;c<no;++c) if(out[c]) FloatVectorOperations::clear(out[c],n);
  if(no<2 || !out[0] || !out[1] || n>playback.getNumSamples()) return;
  bool solo=false; for(auto& c:ch) solo|=c.solo.load();
  for(int c=0;c<N;++c){auto& s=ch[c]; float p=0, post=0; if(c<ni && in[c]) {auto* x=in[c]; for(int i=0;i<n;++i) p=std::max(p,std::abs(x[i])); {float g=(enabled && !s.mute && (!solo || s.solo))?s.gain.load():0.f, a=s.pan; float l=std::sqrt((1-a)*0.5f),r=std::sqrt((1+a)*0.5f); float hp=pulse::coefficient(s.hp.load(),rate), th=pulse::dbGain(s.threshold), ratio=s.ratio, attack=pulse::coefficient(30,rate), release=pulse::coefficient(2,rate), smooth=pulse::coefficient(100,rate); for(int i=0;i<n;++i){float y=s.dsp.process(x[i],g,hp,th,ratio,attack,release,smooth);post=std::max(post,std::abs(y));out[0][i]+=y*l;out[1][i]+=y*r;}}} s.peak=p;s.postPeak=post;}
  playback.clear(); AudioSourceChannelInfo info(&playback,0,n); transport.getNextAudioBlock(info);
  float p=0; for(int c=0;c<2;++c){FloatVectorOperations::add(out[c],playback.getReadPointer(c),n); FloatVectorOperations::multiply(out[c],master.load(),n); for(int i=0;i<n;++i){p=std::max(p,std::abs(out[c][i])); out[c][i]=jlimit(-1.f,1.f,out[c][i]);}} peak=p;
  const ScopedTryLock lock(recordLock); if(lock.isLocked() && writer && !writer->write(out,n)) ++missedWrites;
 }
 bool load(File f){auto* r=formats.createReaderFor(f); if(!r)return false; transport.stop(); transport.setSource(nullptr); reader=std::make_unique<AudioFormatReaderSource>(r,true); transport.setSource(reader.get(),32768,&disk,r->sampleRate); return true;}
 bool record(File f){stopRecord(); auto stream=f.createOutputStream(); if(!stream)return false; WavAudioFormat wav; auto* w=wav.createWriterFor(stream.get(),rate,2,24,{},0); if(!w)return false; stream.release(); auto next=std::make_unique<AudioFormatWriter::ThreadedWriter>(w,disk,65536); const ScopedLock lock(recordLock); writer=std::move(next); return true;}
 void stopRecord(){std::unique_ptr<AudioFormatWriter::ThreadedWriter> old; {const ScopedLock lock(recordLock); old=std::move(writer);} }
};
class Strip : public Component {
public:
 Channel& c; Label name; Slider gain,pan,hp,ratio; TextButton lock{"Lock"}, apply{"Apply"}; Label advice; float anchor=0.25f, envelope=0; TextButton mute{"M"},solo{"S"};
 Strip(Channel& s,int i):c(s){name.setText("IN "+String(i+1),dontSendNotification); name.setEditable(false,true,false); name.setJustificationType(Justification::centred); addAndMakeVisible(name); for(auto* b:{&mute,&solo}){addAndMakeVisible(b);b->setClickingTogglesState(true);} mute.setColour(TextButton::buttonOnColourId,Colour(0xffa83240)); solo.setColour(TextButton::buttonOnColourId,Colour(0xff967322)); mute.setToggleState(true,dontSendNotification); mute.onClick=[this]{c.mute=mute.getToggleState();}; solo.onClick=[this]{c.solo=solo.getToggleState();}; gain.setSliderStyle(Slider::LinearVertical); gain.setRange(-60,3.52,0.01);gain.setSkewFactorFromMidPoint(-12);gain.setValue(Decibels::gainToDecibels(c.gain.load()));gain.setTextValueSuffix(" dB");gain.setTextBoxStyle(Slider::TextBoxBelow,false,56,20);gain.onValueChange=[this]{c.gain=Decibels::decibelsToGain((float)gain.getValue(),-60.f); anchor=c.gain.load();}; lock.setClickingTogglesState(true); lock.setColour(TextButton::buttonOnColourId,Colour(0xff967322)); addAndMakeVisible(lock); addAndMakeVisible(apply); addAndMakeVisible(advice); advice.setFont(Font(11.f)); advice.setJustificationType(Justification::centred); apply.onClick=[this]{if(apply.isEnabled() && anchor>=0.001f) setAuto(suggest());}; pan.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);pan.setRange(-1,1,0.01);pan.setValue(0);pan.setTextBoxStyle(Slider::NoTextBox,false,0,0);pan.onValueChange=[this]{c.pan=(float)pan.getValue();};addAndMakeVisible(gain);addAndMakeVisible(pan); for(auto* k:{&hp,&ratio}){k->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);k->setTextBoxStyle(Slider::TextBoxBelow,false,65,16);addAndMakeVisible(k);} hp.setRange(0,250,1);hp.setTextValueSuffix(" Hz");hp.onValueChange=[this]{c.hp=(float)hp.getValue();};ratio.setRange(1,8,0.1);ratio.setValue(1);ratio.setTextValueSuffix(":1");ratio.onValueChange=[this]{c.ratio=(float)ratio.getValue();};}
 float suggest() const {float base=std::max(0.001f,anchor); return std::clamp(0.0631f/std::max(envelope,0.0032f),base*0.5012f,std::min(1.5f,base*1.9953f));}
 void setAuto(float v){c.gain=v; gain.setValue(Decibels::gainToDecibels(v,-60.f),dontSendNotification);}
 void updateAuto(int mode,bool active){envelope=0.85f*envelope+0.15f*c.peak.load(); bool eligible=active && !c.mute.load() && !lock.getToggleState() && anchor>=0.001f && envelope>0.0032f && c.peak.load()>0.0032f; apply.setEnabled(mode==2 && eligible); if(!eligible || mode==1){advice.setText(lock.getToggleState()?"Locked":c.mute.load()?"Muted":!active?"Paused":mode==1?"Manual":anchor<0.001f?"Fader off":"No signal",dontSendNotification);return;} float target=suggest(), current=std::max(0.001f,c.gain.load()); float delta=20.f*std::log10(target/current); advice.setText((mode==3?String("Auto "):String())+String(delta,1)+" dB",dontSendNotification); if(mode==3 && !gain.isMouseButtonDown() && std::abs(delta)>0.5f)setAuto(std::clamp(current*std::pow(10.f,std::clamp(delta,-0.067f,0.033f)/20.f),0.f,1.5f));}
 void paint(Graphics& g) override {g.fillAll(Colour(0xff202832));auto r=Rectangle<int>(getWidth()-17,322,6,std::max(1,getHeight()-350));g.setColour(Colour(0xff10151b));g.fillRect(r);g.setColour(c.peak>0.99f?Colours::orange:Colour(0xff27d6ad));auto post=r.translated(8,0);g.fillRect(r.removeFromBottom((int)(r.getHeight()*jlimit(0.f,1.f,std::sqrt(c.peak.load())))));g.setColour(Colour(0xff10151b));g.fillRect(post);g.setColour(Colour(0xff7caaff));g.fillRect(post.removeFromBottom((int)(post.getHeight()*jlimit(0.f,1.f,std::sqrt(c.postPeak.load())))));}
 void resized() override {auto r=getLocalBounds().reduced(4);name.setBounds(r.removeFromTop(24));pan.setBounds(r.removeFromTop(48));auto b=r.removeFromTop(25);mute.setBounds(b.removeFromLeft(b.getWidth()/2));solo.setBounds(b);hp.setBounds(r.removeFromTop(60));ratio.setBounds(r.removeFromTop(60));lock.setBounds(r.removeFromTop(23)); apply.setBounds(r.removeFromTop(23)); advice.setBounds(r.removeFromTop(18)); r.removeFromRight(18);gain.setBounds(r);}
};
class Main : public Component, private Timer {
 Engine e; OwnedArray<Strip> strips; Component bank; Viewport view; Label title,status,routeStatus; TextButton setup{"Audio setup"},open{"Open audio"},play{"Play"},stop{"Stop"},rec{"Record stereo"},end{"Stop recording"},live{"Enable live inputs"}; Slider master; ComboBox mode; Label modeLabel; std::unique_ptr<FileChooser> chooser;
public:
 Main(){title.setText("PULSE STUDIO  /  ALPHA 0.3.2",dontSendNotification);title.setFont(Font(24.f));addAndMakeVisible(title);addAndMakeVisible(status);addAndMakeVisible(routeStatus); mode.addItem("Manual",1); mode.addItem("Assisted",2); mode.addItem("Auto level",3); mode.setSelectedId(1); addAndMakeVisible(mode); modeLabel.setText("Mix mode",dontSendNotification); addAndMakeVisible(modeLabel);for(auto* b:{&setup,&open,&play,&stop,&rec,&end,&live})addAndMakeVisible(b);live.setClickingTogglesState(true);live.setColour(TextButton::buttonOnColourId,Colour(0xff17665b));live.onClick=[this]{e.enabled=live.getToggleState();live.setButtonText(e.enabled.load()?"Live inputs ON":"Enable live inputs");};setup.onClick=[this]{e.startAudio(); DialogWindow::LaunchOptions o;o.content.setOwned(new AudioDeviceSelectorComponent(e.devices,0,N,2,2,false,false,true,false));o.content->setSize(560,480);o.dialogTitle="Audio devices";o.dialogBackgroundColour=Colour(0xff202832);o.launchAsync();};open.onClick=[this]{chooser=std::make_unique<FileChooser>("Open audio",File{},"*.wav;*.aiff;*.flac;*.mp3");chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles,[this](const FileChooser& f){if(f.getResult().existsAsFile())e.load(f.getResult());});};play.onClick=[this]{e.transport.start();};stop.onClick=[this]{e.transport.stop();e.transport.setPosition(0);};rec.onClick=[this]{chooser=std::make_unique<FileChooser>("Record stereo WAV",File::getSpecialLocation(File::userDocumentsDirectory).getNonexistentChildFile("Pulse recording",".wav"),"*.wav");chooser->launchAsync(FileBrowserComponent::saveMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::warnAboutOverwriting,[this](const FileChooser& f){auto file=f.getResult();if(file!=File{} && !file.withFileExtension("wav").existsAsFile())e.record(file.withFileExtension("wav"));});};end.onClick=[this]{e.stopRecord();};master.setRange(0,1,0.001);master.setValue(0.5);master.onValueChange=[this]{e.master=(float)master.getValue();};addAndMakeVisible(master);view.setViewedComponent(&bank,false);addAndMakeVisible(view);for(int i=0;i<N;++i){auto* s=strips.add(new Strip(e.ch[i],i));bank.addAndMakeVisible(s);}setSize(1200,680);startTimerHz(15);}
 void paint(Graphics& g) override {g.fillAll(Colour(0xff131a22));g.setColour(Colour(0xff32d8bd));g.drawText("MASTER",12,132,75,25,Justification::centredLeft);}
 void resized() override {auto r=getLocalBounds().reduced(12);title.setBounds(r.removeFromTop(42));auto row=r.removeFromTop(38);for(auto* b:{&setup,&open,&play,&stop,&rec,&end,&live}){b->setBounds(row.removeFromLeft(145).reduced(2));}auto controls=r.removeFromTop(32); modeLabel.setBounds(controls.removeFromLeft(90)); mode.setBounds(controls.removeFromLeft(180)); auto m=r.removeFromTop(45);m.removeFromLeft(80);master.setBounds(m.removeFromLeft(280));status.setBounds(m);routeStatus.setBounds(r.removeFromTop(25));view.setBounds(r);bank.setSize(N*108,std::max(180,r.getHeight()-18));for(int i=0;i<strips.size();++i)strips[i]->setBounds(i*108,0,104,bank.getHeight());}
 void timerCallback() override {bool active=e.enabled.load() && e.devices.getCurrentAudioDevice()!=nullptr && e.peak.load()<0.9f; bool anySolo=false; for(auto& c:e.ch) anySolo|=c.solo.load(); for(auto* s:strips){s->updateAuto(mode.getSelectedId(),active && !anySolo);s->repaint();}int unmuted=0; for(auto& c:e.ch) if(!c.mute.load()) ++unmuted; routeStatus.setText(!e.started?"Open Audio setup first":!e.enabled.load()?"Live inputs OFF - enable monitoring with headphones":unmuted==0?"All channels muted - unmute one input":anySolo?"Auto paused: solo active":e.peak.load()>=0.9f?"Auto paused: high master peak":mode.getSelectedId()==1?"Manual mixing":"Auto ready | Green meter: input / Blue: post fader",dontSendNotification);status.setText((!e.started ? String("Audio not started - open Audio setup | ") : e.deviceError.isNotEmpty() && e.devices.getCurrentAudioDevice()==nullptr ? e.deviceError+" | " : String())+"Device CPU: "+String(e.devices.getCpuUsage()*100,1)+"%  |  Master peak: "+String(Decibels::gainToDecibels(e.peak.load()),1)+" dBFS | Write failures: "+String((int)e.missedWrites.load()),dontSendNotification);}
};
class ConsoleLook : public LookAndFeel_V4 {
public:
 void drawLinearSlider(Graphics& g,int x,int y,int w,int h,float pos,float minPos,float maxPos,const Slider::SliderStyle style,Slider& slider) override {
  if(style!=Slider::LinearVertical){LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,pos,minPos,maxPos,style,slider);return;}
  const float cx=x+w*0.48f;
  g.setColour(Colour(0xff10151a));g.fillRoundedRectangle(cx-3,y,6,(float)h,3);
  g.setColour(Colour(0xff56616a));g.drawVerticalLine((int)cx,y,y+h);
  for(float db: {3.f,0.f,-5.f,-10.f,-20.f,-30.f,-40.f,-60.f}){
   if(db<slider.getMinimum() || db>slider.getMaximum())continue;
   float yy=(float)slider.getPositionOfValue(db);
   g.setColour(db==0?Colour(0xffe0e6e9):Colour(0xff87939d));
   g.drawHorizontalLine((int)yy,cx+7,cx+12);
   g.setFont(9.f);g.drawText(String((int)db),(int)cx+13,(int)yy-6,20,12,Justification::centredLeft,false);
  }
  float cw=std::min(30.f,w*0.55f),ch=27.f;
  Rectangle<float> cap(cx-cw/2,pos-ch/2,cw,ch);
  g.setColour(Colour(0xff080b0e));g.fillRoundedRectangle(cap.translated(1,2),3);
  g.setGradientFill(ColourGradient(Colour(0xffe1e5e8),cap.getX(),cap.getY(),Colour(0xff7c8791),cap.getX(),cap.getBottom(),false));
  g.fillRoundedRectangle(cap,3);
  g.setColour(Colour(0xff424b54));g.drawRoundedRectangle(cap,3,1);
  for(int i=-2;i<=2;++i){g.setColour(Colour(0xff56616a));g.drawHorizontalLine((int)pos+i*3,cap.getX()+4,cap.getRight()-4);}
  g.setColour(Colour(0xfff6f8fa));g.drawHorizontalLine((int)pos,cap.getX()+2,cap.getRight()-2);
 }
 int getSliderThumbRadius(Slider& s) override {return s.getSliderStyle()==Slider::LinearVertical?14:LookAndFeel_V4::getSliderThumbRadius(s);}
};
class App : public JUCEApplication {std::unique_ptr<DocumentWindow> window; ConsoleLook look; public: const String getApplicationName() override{return "Pulse Studio";} const String getApplicationVersion() override{return "0.3.2";} void initialise(const String&) override {look.setColour(Slider::thumbColourId,Colour(0xff32d8bd));LookAndFeel::setDefaultLookAndFeel(&look);struct W:DocumentWindow{W():DocumentWindow("Pulse Studio",Colour(0xff131a22),allButtons){setUsingNativeTitleBar(true);setContentOwned(new Main,true);setResizable(true,true);centreWithSize(1200,680);setVisible(true);}void closeButtonPressed()override{JUCEApplication::getInstance()->systemRequestedQuit();}};window=std::make_unique<W>();}void shutdown()override{window.reset();LookAndFeel::setDefaultLookAndFeel(nullptr);}};
START_JUCE_APPLICATION(App)
