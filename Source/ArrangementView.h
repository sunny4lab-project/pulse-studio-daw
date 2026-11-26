#pragma once
#include "AudioEngine.h"
class ArrangementView : public Component {
 Engine& engine;AudioThumbnailCache cache{8};std::array<std::unique_ptr<AudioThumbnail>,pulse::stemCount> waves;Array<File> files;TextButton add{"Add stem"},remove{"Remove last"};std::array<Slider,pulse::stemCount> trim;std::array<TextButton,pulse::stemCount> mute;std::unique_ptr<FileChooser> chooser;
 double span() const {double d=10;for(auto& w:waves)if(w)d=std::max(d,w->getTotalLength());return d;}
 int labelWidth() const{return getWidth()>500?146:100;}
 Rectangle<int> track() const{return getLocalBounds().reduced(5).withTrimmedTop(46).withTrimmedLeft(labelWidth());}
public:
 explicit ArrangementView(Engine& e):engine(e){
  addAndMakeVisible(add);addAndMakeVisible(remove);add.onClick=[this]{chooser=std::make_unique<FileChooser>("Add aligned mono/stereo stem (same sample rate)",File{},"*.wav;*.aiff;*.flac;*.mp3");chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles,[this](const FileChooser& f){if(f.getResult()==File{})return;if(engine.addStem(f.getResult()))sync();else AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Stem not added","Use up to four mono/stereo files with the same sample rate.");});};remove.onClick=[this]{if(engine.removeStem())sync();};
  for(int i=0;i<pulse::stemCount;++i){waves[i]=std::make_unique<AudioThumbnail>(512,e.formats,cache);trim[i].setSliderStyle(Slider::LinearHorizontal);trim[i].setRange(-60,0,.1);trim[i].setValue(0);trim[i].setTextBoxStyle(Slider::NoTextBox,false,0,0);trim[i].setTooltip("Stem trim (read-ahead delay applies)");trim[i].onValueChange=[this,i]{engine.stemTrims[i]=Decibels::decibelsToGain((float)trim[i].getValue(),-60.f);};mute[i].setButtonText("M");mute[i].setClickingTogglesState(true);mute[i].setColour(TextButton::buttonOnColourId,Colour(0xffa83240));mute[i].onClick=[this,i]{engine.stemMutes[i]=mute[i].getToggleState();};addChildComponent(trim[i]);addChildComponent(mute[i]);}sync();
 }
 void sync(){files=engine.stemFiles;for(int i=0;i<pulse::stemCount;++i){waves[i]->clear();if(i<files.size())waves[i]->setSource(new FileInputSource(files[i]));trim[i].setValue(Decibels::gainToDecibels(engine.stemTrims[i].load(),-60.f),dontSendNotification);mute[i].setToggleState(engine.stemMutes[i].load(),dontSendNotification);}add.setEnabled(files.size()<pulse::stemCount);remove.setEnabled(files.size()>1);resized();repaint();}
 void openFile(const File&){sync();}
 void resized() override {auto top=getLocalBounds().reduced(4).removeFromTop(23);remove.setBounds(top.removeFromRight(88).reduced(1));add.setBounds(top.removeFromRight(80).reduced(1));auto r=track();int h=std::max(1,r.getHeight()/pulse::stemCount);for(int i=0;i<pulse::stemCount;++i){bool visible=i<files.size()&&h>=35;trim[i].setVisible(visible);mute[i].setVisible(visible);auto row=Rectangle<int>(6,r.getY()+i*h+18,labelWidth()-8,16);mute[i].setBounds(row.removeFromRight(24));trim[i].setBounds(row);}}
 void paint(Graphics& g) override {
  g.fillAll(Colour(0xff131e28));auto r=track();if(r.isEmpty())return;g.setColour(Colour(0xff243440));g.fillRect(0,0,getWidth(),25);g.setFont(Font(12.f,Font::bold));g.setColour(Colour(0xffd1dce5));g.drawFittedText("STEMS / ALIGNED PLAYBACK",8,0,std::max(0,getWidth()-180),25,Justification::centredLeft,1);
  g.setColour(Colour(0xff0f1720));g.fillRect(r);double duration=span();int ticks=std::max(2,r.getWidth()/65);
  for(int i=0;i<=ticks;++i){int x=r.getX()+r.getWidth()*i/ticks;g.setColour(Colour(0xff30404f));g.drawVerticalLine(x,28,(float)r.getBottom());if(i<ticks){g.setColour(Colour(0xff9eb4c5));g.setFont(10.f);g.drawText(String(duration*i/ticks,1)+"s",x+3,27,55,17,Justification::centredLeft);}}
  const Colour colours[]={Colour(0xff276f83),Colour(0xff4e6f4a),Colour(0xff775b9d),Colour(0xff957044)};int h=std::max(1,r.getHeight()/pulse::stemCount);
  for(int i=0;i<pulse::stemCount;++i){auto lane=r.withY(r.getY()+i*h).withHeight(h).reduced(1);g.setColour(Colour(0xff22313d));g.fillRect(5,lane.getY(),labelWidth()-1,lane.getHeight());g.setColour(colours[i]);g.fillRect(5,lane.getY(),3,lane.getHeight());g.setColour(Colour(0xffd1dce5));g.drawFittedText(i<files.size()?files[i].getFileName():"Stem "+String(i+1)+" / empty",11,lane.getY()+1,labelWidth()-10,std::min(17,h),Justification::centredLeft,1);
   double length=waves[i]->getTotalLength();if(length>0){auto clip=lane.withWidth(std::max(1,(int)(lane.getWidth()*length/duration)));g.setColour(colours[i]);g.fillRoundedRectangle(clip.toFloat(),3);g.setColour(Colour(0xffc4e0e7));waves[i]->drawChannels(g,clip.reduced(2),0,length,1.f);}g.setColour(Colour(0xff30404f));g.drawHorizontalLine(lane.getBottom(),(float)r.getX(),(float)r.getRight());
  }
  int x=r.getX()+(int)(r.getWidth()*jlimit(0.0,1.0,engine.transport.getCurrentPosition()/duration));g.setColour(Colour(0xff36d4cf));g.drawVerticalLine(x,28,(float)r.getBottom());
 }
 void mouseDown(const MouseEvent& e) override {auto r=track();if(r.contains(e.getPosition())&&!files.isEmpty())engine.transport.setPosition(jlimit(0.0,engine.transport.getLengthInSeconds(),(double)(e.x-r.getX())/std::max(1,r.getWidth())*span()));repaint();}
};
