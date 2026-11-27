#pragma once
#include "../AudioEngine.h"
#include "../TimelineExport.h"
#include "MusicPanel.h"
#include <cstdlib>
#include "../UI/EnterpriseTheme.h"
class TimelineEditor final : public juce::Component,private juce::Timer {
 using Session=pulse::TimelineSession;using Frame=pulse::Frame;
 Engine& e;Session model;std::vector<Session> undoStack,redoStack;
 int selectedTrack=-1,selectedClip=-1;double pixelsPerSecond=55;
 juce::AudioThumbnailCache cache{64};
 struct Wave {juce::File file;std::unique_ptr<juce::AudioThumbnail> thumbnail;};
 std::vector<Wave> waves;
 juce::Component::SafePointer<juce::DialogWindow> musicDialog;juce::TextButton launch{"Launch clip"},music{"MIDI / Automation"},addTrack{"Add track"},import{"Import files"},split{"Split at cursor"},duplicate{"Duplicate"},erase{"Delete clip"},eraseTrack{"Delete track"},undo{"Undo"},redo{"Redo"},play{"Play"},stop{"Stop"},exportWav{"Export WAV"},apply{"Apply settings"};
 juce::Label status,trackLabel,clipLabel;juce::TextEditor name,start,offset,length,fadeIn,fadeOut,gain,pan,clipGain;
 std::array<juce::Label,9> fieldLabels;juce::ToggleButton mute{"Mute track"},solo{"Solo track"};
 juce::ComboBox destination;juce::Slider zoom;juce::Viewport viewport;std::unique_ptr<juce::FileChooser> chooser;std::unique_ptr<pulse::TimelineExportTask> exportTask;
 static constexpr int labelWidth=150,rowHeight=76,rulerHeight=28;
 class Canvas final : public juce::Component {
  TimelineEditor& owner;Frame dragStart=0;int dragTrack=-1,dragClip=-1;bool dragging=false;
 public:
  explicit Canvas(TimelineEditor& o):owner(o){}
  void paint(juce::Graphics& g)override{owner.draw(g);}
  void mouseDown(const juce::MouseEvent& ev)override{
   auto& o=owner;dragTrack=dragClip=-1;dragging=false;
   int t=(ev.y-rulerHeight)/rowHeight;
   if(ev.y>=rulerHeight&&t>=0&&t<(int)o.model.tracks.size()){
    o.selectedTrack=t;o.selectedClip=-1;Frame at=o.frameAt(ev.x);
    if(ev.x>=labelWidth){auto& clips=o.model.tracks[t].clips;for(int i=(int)clips.size()-1;i>=0;--i)if(at>=clips[i].start&&at<clips[i].start+clips[i].length){o.selectedClip=i;dragTrack=t;dragClip=i;dragStart=clips[i].start;break;}}
    o.syncFields();
   }
   if(ev.x>=labelWidth&&!o.e.trackRecorder.active()&&!o.e.multitrack.active())o.e.transport.setPosition(juce::jlimit(0.0,o.e.transport.getLengthInSeconds(),(double)o.frameAt(ev.x)/pulse::timelineRate));
   repaint();
  }
  void mouseDrag(const juce::MouseEvent& ev)override{if(dragClip>=0&&std::abs(ev.getDistanceFromDragStartX())>3)dragging=true;}
  void mouseUp(const juce::MouseEvent& ev)override{
   if(!dragging||dragTrack<0||dragClip<0||!owner.e.canEditInserts())return;auto& o=owner;auto next=o.model;
   auto& c=next.tracks[dragTrack].clips[dragClip];auto delta=(Frame)std::llround(ev.getDistanceFromDragStartX()/o.pixelsPerSecond*pulse::timelineRate);
   c.start=juce::jlimit((Frame)0,pulse::timelineLimit-c.length,dragStart+delta);o.commit(std::move(next));
  }
 } canvas{*this};
 bool hasTrack()const{return selectedTrack>=0&&selectedTrack<(int)model.tracks.size();}
 bool hasClip()const{return hasTrack()&&selectedClip>=0&&selectedClip<(int)model.tracks[selectedTrack].clips.size();}
 Frame frameAt(int x)const{return std::max<Frame>(0,(Frame)std::llround((x-labelWidth)/pixelsPerSecond*pulse::timelineRate));}
 int xAt(Frame f)const{return labelWidth+(int)std::min(16000000.0,(double)f/pulse::timelineRate*pixelsPerSecond);}
 static juce::String seconds(Frame f){return juce::String((double)f/pulse::timelineRate,6);}
 static bool parse(const juce::TextEditor& ed,double lo,double hi,double& value){
  auto s=ed.getText().trim();if(s.isEmpty()||s.containsOnly("0123456789.-+eE")==false)return false;
  auto str=s.toStdString();char* end=nullptr;value=std::strtod(str.c_str(),&end);return end==str.c_str()+str.size()&&std::isfinite(value)&&value>=lo&&value<=hi;
 }
 void message(const juce::String& s){status.setText(s,juce::dontSendNotification);}
 void refreshWaves(){
  std::vector<Wave> next;
  for(auto& t:model.tracks)for(auto& c:t.clips){
   if(std::any_of(next.begin(),next.end(),[&](auto& w){return w.file==c.file;}))continue;
   auto old=std::find_if(waves.begin(),waves.end(),[&](auto& w){return w.file==c.file;});
   if(old!=waves.end())next.push_back({old->file,std::move(old->thumbnail)});
   else {auto thumb=std::make_unique<juce::AudioThumbnail>(512,e.formats,cache);thumb->setSource(new juce::FileInputSource(c.file));next.push_back({c.file,std::move(thumb)});}
  }waves.swap(next);
 }
 void resizeCanvas(){canvas.setSize(std::max(viewport.getWidth()-18,xAt(std::max<Frame>(model.length()+480000,480000))),std::max(viewport.getHeight()-18,rulerHeight+rowHeight*(int)model.tracks.size()));canvas.repaint();}
 void refresh(){if(!hasTrack()){selectedTrack=model.tracks.empty()?-1:juce::jlimit(0,(int)model.tracks.size()-1,selectedTrack);selectedClip=-1;}if(!hasClip())selectedClip=-1;refreshWaves();syncFields();resizeCanvas();}
 bool commit(Session next,bool history=true){
  double cursor=e.transport.getCurrentPosition();auto error=e.setTimeline(next,cursor);
  if(error.isNotEmpty()){message(error);return false;}
  if(history){undoStack.push_back(model);if(undoStack.size()>32)undoStack.erase(undoStack.begin());redoStack.clear();}
  model=std::move(next);refresh();message("Arrangement updated.");return true;
 }
 void history(bool back){auto& from=back?undoStack:redoStack;auto& to=back?redoStack:undoStack;if(from.empty())return;auto next=from.back(),previous=model;if(commit(std::move(next),false)){from.pop_back();to.push_back(std::move(previous));}}
 void syncFields(){
  auto set=[](juce::TextEditor& f,const juce::String& s){f.setText(s,false);};
  if(hasTrack()){auto& t=model.tracks[selectedTrack];destination.setSelectedId(t.bus+2,juce::dontSendNotification);set(name,t.name);set(gain,juce::String(t.gain,3));set(pan,juce::String(t.pan,3));mute.setToggleState(t.mute,juce::dontSendNotification);solo.setToggleState(t.solo,juce::dontSendNotification);trackLabel.setText("Track "+juce::String(selectedTrack+1),juce::dontSendNotification);}else{destination.setSelectedId(1,juce::dontSendNotification);set(name,{});set(gain,{});set(pan,{});mute.setToggleState(false,juce::dontSendNotification);solo.setToggleState(false,juce::dontSendNotification);trackLabel.setText("Select a track",juce::dontSendNotification);}
  if(hasClip()){auto& c=model.tracks[selectedTrack].clips[selectedClip];set(start,seconds(c.start));set(offset,seconds(c.offset));set(length,seconds(c.length));set(fadeIn,seconds(c.fadeIn));set(fadeOut,seconds(c.fadeOut));set(clipGain,juce::String(c.gain,3));clipLabel.setText(c.file.getFileName(),juce::dontSendNotification);}else{for(auto* f:{&start,&offset,&length,&fadeIn,&fadeOut,&clipGain})set(*f,{});clipLabel.setText("Select a clip",juce::dontSendNotification);}
 }
 void applyFields(){
  if(!hasTrack())return;auto next=model;auto& t=next.tracks[selectedTrack];double g=0,p=0;
  if(!parse(gain,0,4,g)||!parse(pan,-1,1,p)){message("Track gain must be 0–4; pan -1 to 1.");return;}
  t.bus=destination.getSelectedId()-2;t.name=name.getText().trim().substring(0,100);if(t.name.isEmpty())t.name="Audio";t.gain=(float)g;t.pan=(float)p;t.mute=mute.getToggleState();t.solo=solo.getToggleState();
  if(hasClip()){auto& c=t.clips[selectedClip];double values[6]{};juce::TextEditor* fields[]={&start,&offset,&length,&fadeIn,&fadeOut,&clipGain};for(int i=0;i<6;++i)if(!parse(*fields[i],0,i==5?4.0:172800.0,values[i])){message("Invalid clip setting. Enter non-negative seconds and gain 0–4.");return;}
   c.start=(Frame)std::llround(values[0]*pulse::timelineRate);c.offset=(Frame)std::llround(values[1]*pulse::timelineRate);c.length=(Frame)std::llround(values[2]*pulse::timelineRate);c.fadeIn=(Frame)std::llround(values[3]*pulse::timelineRate);c.fadeOut=(Frame)std::llround(values[4]*pulse::timelineRate);c.gain=(float)values[5];
  }commit(std::move(next));
 }
 void importFiles(){
  if(!e.canEditInserts()){message("Stop audio first.");return;}chooser=std::make_unique<juce::FileChooser>("Import mono or stereo audio files",juce::File{},"*.wav;*.aiff;*.flac;*.mp3");
  juce::Component::SafePointer<TimelineEditor> safe(this);
  chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::canSelectMultipleItems,[safe](const juce::FileChooser& f){
   if(!safe||f.getResults().isEmpty())return;auto& o=*safe.getComponent();auto next=o.model;Frame cursor=(Frame)std::llround(o.e.transport.getCurrentPosition()*pulse::timelineRate);
   int t=o.hasTrack()?o.selectedTrack:-1;bool separate=f.getResults().size()>1;
   for(auto& file:f.getResults()){std::unique_ptr<juce::AudioFormatReader> r(o.e.formats.createReaderFor(file));if(!r||r->numChannels<1||r->numChannels>2||r->sampleRate<8000||r->sampleRate>384000){o.message("Unsupported file: "+file.getFileName());return;}
    if(t<0||separate){pulse::TimelineTrack track;track.name=file.getFileNameWithoutExtension();next.tracks.push_back(std::move(track));t=(int)next.tracks.size()-1;}
    pulse::AudioClip c;c.file=file;c.start=separate?0:cursor;c.length=pulse::TimelineSession::duration(*r);next.tracks[t].clips.push_back(c);
   }if(o.commit(std::move(next))){o.selectedTrack=t;o.selectedClip=(int)o.model.tracks[t].clips.size()-1;o.syncFields();}
  });
 }
 void exportFile(){
  if(exportTask&&!exportTask->completed){message("Export already running.");return;}if(model.length()<=0||!e.canEditInserts()){message("Stop audio and add clips first.");return;}
  chooser=std::make_unique<juce::FileChooser>("Export arrangement only (no master FX)",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getNonexistentChildFile("Pulse arrangement",".wav"),"*.wav");
  juce::Component::SafePointer<TimelineEditor> safe(this);
  chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles,[safe](const juce::FileChooser& f){
   if(!safe||f.getResult()==juce::File{}||!safe->e.canEditInserts())return;juce::String error;auto source=pulse::TimelineSource::create(safe->e.formats,safe->model,error);
   if(!source){safe->message(error);return;}safe->exportTask=std::make_unique<pulse::TimelineExportTask>(std::move(source),f.getResult().withFileExtension("wav"));safe->exportTask->launchThread();
  });
 }
 void draw(juce::Graphics& g){
  g.fillAll(PulseTheme::bgPrimary);auto area=canvas.getLocalBounds();g.setColour(PulseTheme::header);g.fillRect(0,0,area.getWidth(),rulerHeight);
  auto visible=viewport.getViewArea();double step=pixelsPerSecond>=80?1:pixelsPerSecond>=20?5:15;
  int first=std::max(0,(int)((visible.getX()-labelWidth)/(pixelsPerSecond*step)));
  for(int i=first;;++i){int x=labelWidth+(int)(i*step*pixelsPerSecond);if(x>visible.getRight()+100)break;g.setColour(PulseTheme::border);g.drawVerticalLine(x,0,(float)area.getHeight());g.setColour(PulseTheme::muted);g.setFont(11.f);g.drawText(juce::String(i*step,0)+"s",x+3,0,65,rulerHeight,juce::Justification::centredLeft);}
  for(int t=0;t<(int)model.tracks.size();++t){int y=rulerHeight+t*rowHeight;if(y+rowHeight<visible.getY()||y>visible.getBottom())continue;auto& track=model.tracks[t];
   g.setColour(t==selectedTrack?PulseTheme::active:PulseTheme::panel);g.fillRect(0,y,labelWidth-1,rowHeight-1);g.setColour(PulseTheme::text);g.setFont(13.f);g.drawFittedText(track.name,8,y+5,labelWidth-16,25,juce::Justification::centredLeft,1);g.setColour(PulseTheme::muted);g.setFont(11.f);g.drawText((track.mute?juce::String("MUTED "):juce::String())+(track.solo?"SOLO ":"")+juce::String((int)track.clips.size())+" clips / "+juce::String((int)track.notes.size())+" notes",8,y+33,labelWidth-16,20,juce::Justification::centredLeft);
   for(int i=0;i<(int)track.clips.size();++i){auto& c=track.clips[i];auto rect=juce::Rectangle<int>(xAt(c.start),y+3,std::max(3,xAt(c.start+c.length)-xAt(c.start)),rowHeight-6);if(!rect.intersects(visible))continue;
    g.setColour(t==selectedTrack&&i==selectedClip?juce::Colour(0xff277f91):juce::Colour(0xff24465d));g.fillRoundedRectangle(rect.toFloat(),4);g.setColour(PulseTheme::text);g.drawFittedText(c.file.getFileName(),rect.reduced(4).removeFromTop(18),juce::Justification::centredLeft,1);
    auto wave=std::find_if(waves.begin(),waves.end(),[&](auto& w){return w.file==c.file;});if(wave!=waves.end()&&wave->thumbnail){auto full=rect.reduced(3).withTrimmedTop(18);auto shown=full.getIntersection(visible);if(!shown.isEmpty()&&full.getWidth()>0){double a=(double)(shown.getX()-full.getX())/full.getWidth(),b=(double)(shown.getRight()-full.getX())/full.getWidth();g.setColour(juce::Colour(0xff9bd5db));wave->thumbnail->drawChannels(g,shown,(c.offset+a*c.length)/pulse::timelineRate,(c.offset+b*c.length)/pulse::timelineRate,1.f);}}
    g.setColour(PulseTheme::cyan);if(c.fadeIn>0)g.drawLine((float)rect.getX(),(float)rect.getBottom(),(float)xAt(c.start+c.fadeIn),(float)rect.getY(),1.2f);if(c.fadeOut>0)g.drawLine((float)xAt(c.start+c.length-c.fadeOut),(float)rect.getY(),(float)rect.getRight(),(float)rect.getBottom(),1.2f);
   }for(auto& note:track.notes){auto rect=juce::Rectangle<int>(xAt(note.start),y+5+(127-note.pitch)*(rowHeight-14)/128,std::max(2,xAt(note.start+note.length)-xAt(note.start)),4);if(rect.intersects(visible)){g.setColour(juce::Colour(0xffbfa0ef));g.fillRect(rect);}}g.setColour(PulseTheme::border);g.drawHorizontalLine(y+rowHeight-1,0,(float)area.getWidth());
  }
  g.setColour(PulseTheme::cyan);g.drawVerticalLine(xAt((Frame)(e.transport.getCurrentPosition()*pulse::timelineRate)),0,(float)area.getHeight());
 }
 void timerCallback()override{if(!isShowing())return;if(exportTask&&exportTask->completed){message(exportTask->cancelled?juce::String("Export cancelled."):exportTask->error.isEmpty()?juce::String("Arrangement exported (pre-master)."):exportTask->error);exportTask.reset();}bool idle=e.canEditInserts()&&!exportTask&&musicDialog==nullptr;music.setEnabled(idle&&hasTrack());launch.setEnabled(hasClip()&&!exportTask&&musicDialog==nullptr&&!e.trackRecorder.active()&&!e.multitrack.active());for(auto* b:{&addTrack,&import,&split,&duplicate,&erase,&eraseTrack,&undo,&redo,&apply,&exportWav})b->setEnabled(idle);split.setEnabled(idle&&hasClip());duplicate.setEnabled(idle&&hasClip());erase.setEnabled(idle&&hasClip());eraseTrack.setEnabled(idle&&hasTrack());apply.setEnabled(idle&&hasTrack());undo.setEnabled(idle&&!undoStack.empty());redo.setEnabled(idle&&!redoStack.empty());exportWav.setEnabled(idle&&model.length()>0);canvas.repaint();}
public:
 explicit TimelineEditor(Engine& engine):e(engine),model(e.timelineSession()){
  for(auto* b:{&addTrack,&import,&split,&duplicate,&erase,&eraseTrack,&undo,&redo,&play,&stop,&exportWav,&apply,&music,&launch})addAndMakeVisible(*b);
  for(auto* c:std::array<juce::Component*,6>{&status,&trackLabel,&clipLabel,&zoom,&mute,&solo})addAndMakeVisible(c);
  juce::TextEditor* fields[]={&name,&gain,&pan,&start,&offset,&length,&fadeIn,&fadeOut,&clipGain};const char* labels[]={"Track name","Gain (linear)","Pan (-1..1)","Start (s)","Source in (s)","Length (s)","Fade in (s)","Fade out (s)","Clip gain"};
  for(int i=0;i<9;++i){addAndMakeVisible(*fields[i]);fieldLabels[i].setText(labels[i],juce::dontSendNotification);addAndMakeVisible(fieldLabels[i]);}
  addAndMakeVisible(destination);destination.addItem("Direct master",1);for(int b=0;b<B;++b)destination.addItem("Bus "+juce::String(b+1),b+2);destination.setSelectedId(1,juce::dontSendNotification);viewport.setViewedComponent(&canvas,false);addAndMakeVisible(viewport);
  zoom.setRange(5,240,1);zoom.setValue(pixelsPerSecond);zoom.setTextBoxStyle(juce::Slider::TextBoxRight,false,58,20);zoom.setTextValueSuffix(" px/s");zoom.onValueChange=[this]{pixelsPerSecond=zoom.getValue();resizeCanvas();};
  addTrack.onClick=[this]{auto next=model;pulse::TimelineTrack t;t.name="Audio "+juce::String((int)next.tracks.size()+1);next.tracks.push_back(std::move(t));if(commit(std::move(next))){selectedTrack=(int)model.tracks.size()-1;selectedClip=-1;syncFields();}};
  import.onClick=[this]{importFiles();};split.onClick=[this]{auto next=model;auto at=(Frame)std::llround(e.transport.getCurrentPosition()*pulse::timelineRate);if(next.split(selectedTrack,selectedClip,at))commit(std::move(next));else message("Place playhead inside the selected clip.");};
  duplicate.onClick=[this]{if(!hasClip())return;auto next=model;auto& v=next.tracks[selectedTrack].clips;auto c=v[selectedClip];c.start+=c.length;v.push_back(c);if(commit(std::move(next))){selectedClip=(int)model.tracks[selectedTrack].clips.size()-1;syncFields();}};
  erase.onClick=[this]{if(!hasClip())return;auto next=model;auto& v=next.tracks[selectedTrack].clips;v.erase(v.begin()+selectedClip);commit(std::move(next));};
  eraseTrack.onClick=[this]{if(!hasTrack())return;auto next=model;next.tracks.erase(next.tracks.begin()+selectedTrack);commit(std::move(next));};
  undo.onClick=[this]{history(true);};redo.onClick=[this]{history(false);};apply.onClick=[this]{applyFields();};
  play.onClick=[this]{auto* d=e.devices.getCurrentAudioDevice();if(!d||!d->isPlaying()){message("Connect an audio device for playback; offline export does not need hardware.");return;}if(!e.timeline){if(!commit(model,false))return;}e.transport.start();};stop.onClick=[this]{e.transport.stop();};
  launch.onClick=[this]{if(!hasClip()||e.trackRecorder.active()||e.multitrack.active())return;auto* d=e.devices.getCurrentAudioDevice();if(!d||!d->isPlaying()){message("Connect audio first.");return;}if(!e.timeline&&!commit(model,false))return;e.transport.setPosition((double)model.tracks[selectedTrack].clips[selectedClip].start/pulse::timelineRate);e.transport.start();message("Launched arrangement from selected clip start (all tracks).");};
  music.onClick=[this]{if(!hasTrack()||!e.canEditInserts()||musicDialog!=nullptr)return;int index=selectedTrack;juce::Component::SafePointer<TimelineEditor> safe(this);juce::DialogWindow::LaunchOptions o;o.content.setOwned(new MusicPanel(model.tracks[index],[safe,index](const pulse::TimelineTrack& t){if(!safe||index<0||index>=(int)safe->model.tracks.size())return false;auto next=safe->model;next.tracks[index]=t;return safe->commit(std::move(next));}));o.dialogTitle="MIDI and automation";o.dialogBackgroundColour=PulseTheme::panel;o.resizable=true;musicDialog=o.launchAsync();};
  exportWav.onClick=[this]{exportFile();};setSize(1050,650);refresh();message("Select clips; drag to move, edit fields and Apply. Multiple-file import creates aligned tracks.");startTimerHz(10);
 }
 void shutdown(){stopTimer();chooser.reset();exportTask.reset();if(musicDialog!=nullptr){musicDialog->exitModalState(0);musicDialog=nullptr;}}
 ~TimelineEditor()override{shutdown();viewport.setViewedComponent(nullptr,false);}
 void resized()override{
  auto r=getLocalBounds().reduced(8);auto row=r.removeFromTop(32);int n=7;for(auto* b:{&addTrack,&import,&split,&duplicate,&erase,&eraseTrack,&exportWav})b->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2));
  row=r.removeFromTop(30);for(auto* b:{&undo,&redo,&play,&stop,&music,&launch})b->setBounds(row.removeFromLeft(b==&music?140:b==&launch?95:65).reduced(2));zoom.setBounds(row.removeFromRight(190));status.setBounds(r.removeFromBottom(28));auto inspector=r.removeFromRight(std::min(260,r.getWidth()/3));trackLabel.setBounds(inspector.removeFromTop(25));destination.setBounds(inspector.removeFromTop(26).reduced(2));juce::TextEditor* fields[]={&name,&gain,&pan,&start,&offset,&length,&fadeIn,&fadeOut,&clipGain};
  for(int i=0;i<9;++i){if(i==3){auto toggles=inspector.removeFromTop(26);mute.setBounds(toggles.removeFromLeft(toggles.getWidth()/2));solo.setBounds(toggles);clipLabel.setBounds(inspector.removeFromTop(26));}auto line=inspector.removeFromTop(29);fieldLabels[i].setBounds(line.removeFromLeft(100));fields[i]->setBounds(line.reduced(2));}apply.setBounds(inspector.removeFromTop(30).reduced(2));viewport.setBounds(r.reduced(2));resizeCanvas();
 }
};
