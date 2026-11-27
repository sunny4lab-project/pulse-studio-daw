#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseTheme.h"
// User-initiated, incremental scans on the message thread only while idle.
// JUCE's dead-man file preserves a failed scan identifier across restarts.
class PluginBrowser final : public juce::Component, private juce::ListBoxModel, private juce::Timer {
 Engine& e;
 juce::KnownPluginList known;
 juce::Array<juce::PluginDescription> filtered;
 juce::StringArray favorites,failures;
 juce::Label heading,status;
 juce::TextEditor search;
 juce::ComboBox manufacturer,category;
 juce::ListBox list{"Installed effects",this};
 juce::TextButton scan{"Scan installed"},file{"Scan file"},cancel{"Stop scan"},load{"Load selected"},favorite{"Favorite"},onlyFavorites{"Favorites only"},clearBlacklist{"Retry skipped"};
 std::unique_ptr<juce::FileChooser> chooser;
 std::unique_ptr<juce::PluginDirectoryScanner> scanner;
 int formatIndex=0;bool scanning=false,allFormats=false,instruments=false;
 std::function<void(const juce::PluginDescription&)> onLoad;
 static juce::File directory(){return Engine::deviceSettingsFile().getParentDirectory();}
 static juce::File cache(){return directory().getChildFile("plugin-library.xml");}
 static juce::File pedal(){return directory().getChildFile("plugin-scan-pending.txt");}
 void persist(){
  if(!directory().createDirectory().wasOk())return;
  juce::XmlElement root("PulsePluginLibrary");root.addChildElement(known.createXml().release());
  for(auto& f:favorites)root.createNewChildElement("Favorite")->setAttribute("id",f);
  root.writeTo(cache());
 }
 void restore(){
  if(cache().existsAsFile()&&cache().getSize()<32*1024*1024)if(auto xml=juce::XmlDocument::parse(cache())){
   if(auto* x=xml->getChildByName("KNOWNPLUGINS"))known.recreateFromXml(*x);
   for(auto* x:xml->getChildWithTagNameIterator("Favorite"))favorites.addIfNotAlreadyThere(x->getStringAttribute("id"));
  }
  juce::PluginDirectoryScanner::applyBlacklistingsFromDeadMansPedal(known,pedal());
 }
 static void populate(juce::ComboBox& box,juce::StringArray values,const juce::String& all){
  auto current=box.getText();values.removeEmptyStrings();values.removeDuplicates(true);values.sort(true);
  box.clear(juce::dontSendNotification);box.addItem(all,1);for(int i=0;i<values.size();++i)box.addItem(values[i],i+2);
  int idx=values.indexOf(current);box.setSelectedId(idx>=0?idx+2:1,juce::dontSendNotification);
 }
 void updateList(bool updateFilters=false){
  if(updateFilters){juce::StringArray makers,cats;for(auto& p:known.getTypes())if(p.isInstrument==instruments){makers.add(p.manufacturerName);cats.add(p.category);}populate(manufacturer,makers,"All manufacturers");populate(category,cats,"All categories");}
  filtered.clear();auto term=search.getText().trim();
  for(auto& p:known.getTypes()){
   if(p.isInstrument!=instruments)continue;
   if(manufacturer.getSelectedId()>1&&p.manufacturerName!=manufacturer.getText())continue;
   if(category.getSelectedId()>1&&p.category!=category.getText())continue;
   if(onlyFavorites.getToggleState()&&!favorites.contains(p.createIdentifierString()))continue;
   if(term.isNotEmpty()&&!(p.name+" "+p.manufacturerName+" "+p.category+" "+p.pluginFormatName).containsIgnoreCase(term))continue;
   filtered.add(p);
  }
  struct Compare{int compareElements(const juce::PluginDescription& a,const juce::PluginDescription& b)const{return (a.manufacturerName+" "+a.name).compareNatural(b.manufacturerName+" "+b.name);}} compare;
  filtered.sort(compare);list.updateContent();list.repaint();updateButtons();
 }
 void updateButtons(){bool idle=!scanning&&e.canEditInserts(),selected=juce::isPositiveAndBelow(list.getSelectedRow(),filtered.size());scan.setEnabled(idle);file.setEnabled(idle);clearBlacklist.setEnabled(idle);cancel.setEnabled(scanning);load.setEnabled(idle&&selected);favorite.setEnabled(selected);}
 int getNumRows() override{return filtered.size();}
 void paintListBoxItem(int row,juce::Graphics& g,int w,int h,bool selected)override{
  if(!juce::isPositiveAndBelow(row,filtered.size()))return;auto& p=filtered.getReference(row);
  if(selected)g.fillAll(juce::Colour(0xff24485a));else if(row%2==0)g.fillAll(juce::Colour(0xff152431));
  g.setColour(PulseTheme::text);g.setFont(13.f);g.drawFittedText((favorites.contains(p.createIdentifierString())?juce::String("* "):juce::String())+p.name,8,2,w*55/100-8,h-4,juce::Justification::centredLeft,1);
  g.setColour(PulseTheme::muted);g.setFont(11.f);g.drawFittedText(p.manufacturerName+" | "+p.pluginFormatName+" | "+p.category,w*55/100,2,w*45/100-5,h-4,juce::Justification::centredLeft,1);
 }
 void selectedRowsChanged(int)override{updateButtons();}
 void listBoxItemDoubleClicked(int row,const juce::MouseEvent&)override{if(row>=0)loadSelected();}
 void loadSelected(){
  int row=list.getSelectedRow();if(scanning||!e.canEditInserts()||!juce::isPositiveAndBelow(row,filtered.size()))return;
  auto description=filtered[row];if(onLoad)onLoad(description);
 }
 bool nextFormat(){
  auto formats=e.pluginFormats.getFormats();if(formatIndex>=formats.size())return false;
  auto* f=formats[formatIndex++];
  scanner=std::make_unique<juce::PluginDirectoryScanner>(known,*f,f->getDefaultLocationsToSearch(),true,pedal(),false);return true;
 }
 void finish(){
  stopTimer();if(scanner){failures.addArray(scanner->getFailedFiles());scanner.reset();}
  scanning=false;known.scanFinished();persist();updateList(true);
  status.setText(juce::String(known.getNumTypes())+" discovered | "+juce::String(known.getBlacklistedFiles().size())+" skipped | "+juce::String(failures.size())+" scan failures. Showing "+(instruments?"instruments.":"effects."),juce::dontSendNotification);
 }
 void timerCallback() override{
  if(!scanning||!e.canEditInserts()){finish();return;}
  if(!scanner){if(!allFormats||!nextFormat()){finish();return;}}
  auto next=scanner->getNextPluginFileThatWillBeScanned();status.setText("Scanning "+next,juce::dontSendNotification);
  juce::String current;bool more=scanner->scanNextFile(true,current);persist();updateList(true);
  if(!more){failures.addArray(scanner->getFailedFiles());scanner.reset();if(!allFormats)finish();}
 }
 void startDefaultScan(){if(scanning||!e.canEditInserts())return;directory().createDirectory();formatIndex=0;failures.clear();allFormats=true;scanning=true;updateButtons();startTimer(20);}
 void chooseFile(){
  if(scanning||!e.canEditInserts())return;
  chooser=std::make_unique<juce::FileChooser>("Scan VST3 or Audio Unit bundle",juce::File{},"*.vst3;*.component");
  juce::Component::SafePointer<PluginBrowser> safe(this);
  chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::canSelectDirectories,[safe](const juce::FileChooser& f){
   if(!safe||f.getResult()==juce::File{}||!safe->e.canEditInserts())return;auto path=f.getResult().getFullPathName();
   for(auto* format:safe->e.pluginFormats.getFormats())if(format->fileMightContainThisPluginType(path)){
    directory().createDirectory();safe->scanner=std::make_unique<juce::PluginDirectoryScanner>(safe->known,*format,juce::FileSearchPath{},false,pedal(),false);
    safe->scanner->setFilesOrIdentifiersToScan(juce::StringArray{path});safe->allFormats=false;safe->scanning=true;safe->failures.clear();safe->updateButtons();safe->startTimer(20);return;
   }
   safe->status.setText("No compatible format for the selected file.",juce::dontSendNotification);
  });
 }
public:
 PluginBrowser(Engine& engine,std::function<void(const juce::PluginDescription&)> callback,bool showInstruments=false):e(engine),instruments(showInstruments),onLoad(std::move(callback)){
  for(auto* c:std::array<juce::Component*,13>{&heading,&status,&search,&manufacturer,&category,&list,&scan,&file,&cancel,&load,&favorite,&onlyFavorites,&clearBlacklist})addAndMakeVisible(c);
  heading.setText(instruments?"PLUGIN LIBRARY / INSTRUMENTS":"PLUGIN LIBRARY / STEREO EFFECTS",juce::dontSendNotification);PulseTheme::style(heading,17,true);PulseTheme::style(status,11,false,PulseTheme::muted);
  search.setTextToShowWhenEmpty("Search plugin, manufacturer, category...",PulseTheme::muted);search.onTextChange=[this]{updateList();};
  manufacturer.onChange=[this]{updateList();};category.onChange=[this]{updateList();};
  list.setRowHeight(28);list.setMultipleSelectionEnabled(false);list.setColour(juce::ListBox::backgroundColourId,PulseTheme::panel);
  scan.onClick=[this]{startDefaultScan();};file.onClick=[this]{chooseFile();};cancel.onClick=[this]{finish();};load.onClick=[this]{loadSelected();};
  favorite.onClick=[this]{int row=list.getSelectedRow();if(!juce::isPositiveAndBelow(row,filtered.size()))return;auto id=filtered[row].createIdentifierString();if(favorites.contains(id))favorites.removeString(id);else favorites.add(id);persist();updateList();};
  onlyFavorites.setClickingTogglesState(true);onlyFavorites.onClick=[this]{updateList();};
  clearBlacklist.onClick=[this]{if(scanning||!e.canEditInserts())return;known.clearBlacklistedFiles();pedal().deleteFile();persist();status.setText("Skipped plugins cleared. Scan again only if safe.",juce::dontSendNotification);};
  restore();updateList(true);status.setText(juce::String(known.getNumTypes())+" cached plugins. Scan installed effects or a specific bundle.",juce::dontSendNotification);setSize(740,520);
 }
 void shutdown(){stopTimer();scanner.reset();scanning=false;persist();}
 ~PluginBrowser()override{list.setModel(nullptr);shutdown();}
 void resized()override{
  auto r=getLocalBounds().reduced(10);heading.setBounds(r.removeFromTop(29));auto row=r.removeFromTop(30);search.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));manufacturer.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));category.setBounds(row.reduced(2));
  row=r.removeFromTop(32);int n=4;for(auto* b:{&scan,&file,&cancel,&clearBlacklist})b->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2));
  status.setBounds(r.removeFromBottom(28));row=r.removeFromBottom(32);n=3;for(auto* b:{&onlyFavorites,&favorite,&load})b->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2));list.setBounds(r.reduced(2));
 }
};
