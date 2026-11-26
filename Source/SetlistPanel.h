#pragma once
#include "AudioEngine.h"
class SetlistPanel : public Component, private ListBoxModel {
 StringArray files;
 ListBox list{"Setlist",this};
 TextButton add{"+ Add"},remove{"Remove"},load{"Load"},next{"Next"},up{"Up"};
 std::unique_ptr<FileChooser> chooser;
 Label title;
public:
 std::function<void(const File&)> onOpen;
 SetlistPanel(){
  title.setText("SETLIST",dontSendNotification);title.setFont(Font(13.f,Font::bold));
  for(auto* c:std::array<Component*,7>{&title,&list,&add,&remove,&load,&next,&up})addAndMakeVisible(c);
  list.setRowHeight(26);list.setColour(ListBox::backgroundColourId,Colour(0xff0b1420));
  add.onClick=[this]{chooser=std::make_unique<FileChooser>("Add setlist audio",File{},"*.wav;*.aiff;*.aif;*.flac;*.mp3");chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::canSelectMultipleItems,[this](const FileChooser& f){for(const auto& item:f.getResults())if(item.existsAsFile())files.add(item.getFullPathName());list.updateContent();if(list.getSelectedRow()<0&&files.size()>0)list.selectRow(0);repaint();});};
  remove.onClick=[this]{int i=list.getSelectedRow();if(i>=0&&i<files.size()){files.remove(i);list.updateContent();list.selectRow(std::min(i,files.size()-1));repaint();}};
  load.onClick=[this]{activate(list.getSelectedRow());};
  next.onClick=[this]{int i=list.getSelectedRow()+1;if(i<files.size()){list.selectRow(i);activate(i);}};
  up.onClick=[this]{int i=list.getSelectedRow();if(i>0&&i<files.size()){files.swap(i,i-1);list.updateContent();list.selectRow(i-1);repaint();}};
 }
 ~SetlistPanel() override {list.setModel(nullptr);}
 int getNumRows() override {return files.size();}
 void paintListBoxItem(int row,Graphics& g,int w,int h,bool selected) override {
  if(row<0||row>=files.size())return;
  auto q=Rectangle<int>(0,0,w,h).reduced(1,2);g.setColour(selected?Colour(0xff164b53):Colour(0xff152230));g.fillRoundedRectangle(q.toFloat(),4);
  g.setColour(selected?Colour(0xff45d5d0):Colour(0xff95aabd));g.setFont(11.f);g.drawText(String(row+1),q.removeFromLeft(21),Justification::centred);
  g.setColour(Colour(0xffe0ebf5));g.drawFittedText(File(files[row]).getFileNameWithoutExtension(),q.reduced(3,1),Justification::centredLeft,1);
 }
 void listBoxItemDoubleClicked(int row,const MouseEvent&) override {activate(row);}
 void activate(int i){if(i>=0&&i<files.size()&&onOpen)onOpen(File(files[i]));}
 void save(XmlElement& root) const {root.setAttribute("setlistCount",files.size());for(int i=0;i<files.size();++i)root.setAttribute("setlist"+String(i),files[i]);}
 void restore(const XmlElement& root){files.clear();for(int i=0;i<jlimit(0,1000,root.getIntAttribute("setlistCount"));++i){auto s=root.getStringAttribute("setlist"+String(i));if(s.isNotEmpty()&&File::isAbsolutePath(s))files.add(s);}list.updateContent();if(files.size()>0)list.selectRow(0);repaint();}
 void resized() override {auto r=getLocalBounds().reduced(6);title.setBounds(r.removeFromTop(22));auto row=r.removeFromTop(24);add.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(1));remove.setBounds(row.reduced(1));auto bottom=r.removeFromBottom(24);load.setBounds(bottom.removeFromLeft(bottom.getWidth()/3).reduced(1));next.setBounds(bottom.removeFromLeft(bottom.getWidth()/2).reduced(1));up.setBounds(bottom.reduced(1));list.setBounds(r.reduced(0,3));}
};
