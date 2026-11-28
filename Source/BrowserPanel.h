#pragma once
#include "AudioEngine.h"
class BrowserPanel : public Component, private FileBrowserListener {
 WildcardFileFilter filter{"*.wav;*.aiff;*.aif;*.flac;*.mp3","*","Audio files"};
 TimeSliceThread thread{"Browser scan"};
 DirectoryContentsList directory{&filter,thread};
 FileListComponent list{directory};
 Label title,location;
 TextButton up{"Up"},home{"Home"};
public:
 std::function<void(const File&)> onOpen;
 BrowserPanel(){
  title.setText("FILE BROWSER",dontSendNotification);title.setFont(Font(11.f,Font::bold));
  location.setFont(Font(10.f));
  for(auto* x:std::array<Component*,5>{&title,&location,&up,&home,&list})addAndMakeVisible(x);
  list.addListener(this);thread.startThread();
  setFolder(File::getSpecialLocation(File::userMusicDirectory));
  up.onClick=[this]{setFolder(directory.getDirectory().getParentDirectory());};
  home.onClick=[this]{setFolder(File::getSpecialLocation(File::userHomeDirectory));};
 }
 ~BrowserPanel() override {list.removeListener(this);directory.clear();thread.stopThread(2000);}
 void setFolder(File f){if(!f.isDirectory())f=File::getSpecialLocation(File::userHomeDirectory);directory.setDirectory(f,true,true);location.setText(f.getFileName().isEmpty()?f.getFullPathName():f.getFileName(),dontSendNotification);location.setTooltip(f.getFullPathName());}
 void selectionChanged() override {}
 void fileClicked(const File&,const MouseEvent&) override {}
 void fileDoubleClicked(const File& f) override {if(f.isDirectory())setFolder(f);else if(onOpen)onOpen(f);}
 void browserRootChanged(const File&) override {}
 void resized() override {auto r=getLocalBounds().reduced(4);title.setBounds(r.removeFromTop(18));location.setBounds(r.removeFromTop(18));auto row=r.removeFromTop(22);up.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(1));home.setBounds(row.reduced(1));list.setBounds(r);}
};
