#pragma once
#include "../MixerStrip.h"
#include "../UI/EnterpriseTheme.h"
class TrackControlPanel final : public Component,private Timer {
 Engine& e;OwnedArray<Strip>& strips;ComboBox track,group;TextButton arm{"Record arm"},armAll{"Arm mapped"},clear{"Disarm all"},start{"Record armed WAVs"},stop{"Stop recording"};Label title,status;
 std::array<Slider,Engine::dcaCount> levels;std::array<TextButton,Engine::dcaCount> mutes;std::array<Label,Engine::dcaCount> labels;
 std::unique_ptr<FileChooser> chooser;
 Channel* current(){int i=track.getSelectedId()-1;return i>=0&&i<strips.size()?&strips[i]->c:nullptr;}
 bool recording(){return e.trackRecorder.active()||e.multitrack.active();}
 void timerCallback()override{
  if(!isShowing())return;
  StringArray names;for(auto* s:strips)names.add(s->name.getText());bool rebuild=names.size()!=track.getNumItems();if(!rebuild)for(int i=0;i<names.size();++i)if(names[i]!=track.getItemText(i)){rebuild=true;break;}
  if(rebuild){int prev=track.getSelectedId();track.clear(dontSendNotification);for(int i=0;i<names.size();++i)track.addItem(names[i].isEmpty()?String("Channel ")+String(i+1):names[i],i+1);track.setSelectedId(names.isEmpty()?0:jlimit(1,names.size(),prev),dontSendNotification);}
  auto* c=current();int armed=0;for(auto& ch:e.ch)if(ch.recordArm.load())++armed;
  group.setEnabled(c!=nullptr);arm.setEnabled(c&&!recording());armAll.setEnabled(!recording());clear.setEnabled(!recording());start.setEnabled(!recording()&&armed>0);stop.setEnabled(e.trackRecorder.active()||e.trackRecorder.dropped.load()>0);
  group.setSelectedId(c?c->dca.load()+2:1,dontSendNotification);arm.setToggleState(c&&c->recordArm.load(),dontSendNotification);
  for(int i=0;i<Engine::dcaCount;++i){if(!levels[i].isMouseButtonDown())levels[i].setValue(Decibels::gainToDecibels(e.dcaGain[i].load(),-60.f),dontSendNotification);mutes[i].setToggleState(e.dcaMute[i].load(),dontSendNotification);}
  status.setText(e.trackRecorder.active()?String("Recording ")+String(e.trackRecorder.count.load())+" tracks | "+String((double)e.trackRecorder.frames.load()/e.rate.load(),1)+" s":String(armed)+" armed | "+(e.trackRecorder.dropped.load()?"Capture interrupted: check disk/input; last block may be incomplete.":"Raw input capture; independent of fader and mute."),dontSendNotification);
 }
 void begin(){
  if(recording())return;
  chooser=std::make_unique<FileChooser>("Choose a folder for separate WAV tracks",File::getSpecialLocation(File::userDocumentsDirectory));
  Component::SafePointer<TrackControlPanel> safe(this);
  chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectDirectories,[safe](const FileChooser& f){
   if(!safe||f.getResult()==File{}||safe->recording())return;auto& e=safe->e;auto* d=e.devices.getCurrentAudioDevice();if(!d||!d->isPlaying()){AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Recording","Connect an audio device first.");return;}
   std::vector<pulse::TrackRecorder::Request> req;for(auto* s:safe->strips)if(s->c.recordArm.load())req.push_back({s->name.getText(),s->c.source.load(),s->c.rightSource.load()});
   auto folder=f.getResult().getNonexistentChildFile("Pulse-Take-"+Time::getCurrentTime().formatted("%Y%m%d-%H%M%S"),"");
   auto error=e.trackRecorder.start(folder,req,d->getCurrentSampleRate(),d->getActiveInputChannels().countNumberOfSetBits(),d->getCurrentBufferSizeSamples());
   if(error.isNotEmpty())AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Recording",error);safe->timerCallback();
  });
 }
public:
 TrackControlPanel(Engine& engine,OwnedArray<Strip>& s):e(engine),strips(s){
  for(auto* c:std::array<Component*,9>{&track,&group,&arm,&armAll,&clear,&start,&stop,&title,&status})addAndMakeVisible(c);
  title.setText("DCA GROUPS / TRACK RECORDING",dontSendNotification);PulseTheme::style(title,16,true);PulseTheme::style(status,12,false,PulseTheme::muted);
  group.addItem("No DCA",1);for(int i=0;i<Engine::dcaCount;++i){group.addItem("DCA "+String(i+1),i+2);addAndMakeVisible(levels[i]);addAndMakeVisible(mutes[i]);addAndMakeVisible(labels[i]);labels[i].setText("DCA "+String(i+1),dontSendNotification);levels[i].setSliderStyle(Slider::LinearHorizontal);levels[i].setRange(-60,6,.1);levels[i].setTextValueSuffix(" dB");levels[i].setTextBoxStyle(Slider::TextBoxRight,false,60,22);levels[i].onValueChange=[this,i]{e.dcaGain[i]=Decibels::decibelsToGain((float)levels[i].getValue(),-60.f);};mutes[i].setButtonText("Mute");mutes[i].setClickingTogglesState(true);mutes[i].onClick=[this,i]{e.dcaMute[i]=mutes[i].getToggleState();};}
  track.onChange=[this]{timerCallback();};group.onChange=[this]{if(auto* c=current())c->dca=group.getSelectedId()-2;};arm.setClickingTogglesState(true);arm.onClick=[this]{if(auto* c=current();c&&!recording())c->recordArm=arm.getToggleState();timerCallback();};armAll.onClick=[this]{if(!recording()){auto n=e.activeChannelNames(true).size();for(auto& c:e.ch)c.recordArm=c.source.load()>=0&&c.source.load()<n&&(c.rightSource.load()<0||c.rightSource.load()<n);}timerCallback();};clear.onClick=[this]{if(!recording())for(auto& c:e.ch)c.recordArm=false;timerCallback();};start.onClick=[this]{begin();};stop.onClick=[this]{e.trackRecorder.stop();timerCallback();};
  setSize(650,455);startTimerHz(5);
 }
 void shutdown(){stopTimer();chooser.reset();}
 ~TrackControlPanel()override{shutdown();}
 void resized()override{auto r=getLocalBounds().reduced(10);title.setBounds(r.removeFromTop(30));auto row=r.removeFromTop(32);track.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));group.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));arm.setBounds(row.reduced(2));row=r.removeFromTop(32);int n=4;for(auto* b:{&armAll,&clear,&start,&stop})b->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2));status.setBounds(r.removeFromBottom(36));for(int i=0;i<Engine::dcaCount;++i){auto line=r.removeFromTop(36);labels[i].setBounds(line.removeFromLeft(64));mutes[i].setBounds(line.removeFromRight(74).reduced(2));levels[i].setBounds(line.reduced(2));}}
};
