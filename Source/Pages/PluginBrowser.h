#pragma once
#include "../AudioEngine.h"
#include "../UI/EnterpriseTheme.h"
#include "../PluginScanHelper.h"
#include <vector>
// The UI polls child processes; third-party plugin discovery never executes in the host.
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
 struct Candidate{juce::String format,path;};
 std::vector<Candidate> queue;size_t nextCandidate=0;
 std::unique_ptr<juce::ChildProcess> child;juce::File requestDir;
 juce::String currentPath;double startedAt=0;bool discovering=false,scanning=false,instruments=false;
 inline static PluginBrowser* scanOwner=nullptr;
 static constexpr double scanTimeoutMs=90000,discoveryTimeoutMs=120000;
 std::function<void(const juce::PluginDescription&)> onLoad;
 static juce::File directory(){return Engine::deviceSettingsFile().getParentDirectory();}
 static juce::File cache(){return directory().getChildFile("plugin-library.xml");}
 static juce::File pedal(){return directory().getChildFile("plugin-scan-pending.txt");}
 void persist(){
  if(!directory().createDirectory().wasOk())return;
  juce::XmlElement root("PulsePluginLibrary");root.addChildElement(known.createXml().release());
  for(auto& f:favorites)root.createNewChildElement("Favorite")->setAttribute("id",f);
  juce::TemporaryFile temp(cache());if(root.writeTo(temp.getFile()))temp.overwriteTargetFileWithTemporary();
 }
 void restore(){
  if(cache().existsAsFile()&&cache().getSize()<32*1024*1024)if(auto xml=juce::XmlDocument::parse(cache())){
   if(auto* x=xml->getChildByName("KNOWNPLUGINS"))known.recreateFromXml(*x);
   for(auto* x:xml->getChildWithTagNameIterator("Favorite"))favorites.addIfNotAlreadyThere(x->getStringAttribute("id"));
  }
  if(scanOwner==nullptr){juce::PluginDirectoryScanner::applyBlacklistingsFromDeadMansPedal(known,pedal());persist();pedal().deleteFile();}
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
 void updateButtons(){bool idle=!scanning&&scanOwner==nullptr&&e.canEditInserts(),selected=juce::isPositiveAndBelow(list.getSelectedRow(),filtered.size());scan.setEnabled(idle);file.setEnabled(idle);clearBlacklist.setEnabled(idle);cancel.setEnabled(scanning);load.setEnabled(idle&&selected);favorite.setEnabled(selected);}
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
 bool launch(const juce::String& action,const Candidate& item){
  using namespace juce;
  requestDir=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("pulse-plugin-scan","");
  if(!requestDir.createDirectory().wasOk())return false;
  auto request=requestDir.getChildFile("request.xml");XmlElement xml("PluginScan");
  xml.setAttribute("action",action);xml.setAttribute("format",item.format);xml.setAttribute("path",item.path);
  if(!xml.writeTo(request))return false;
  child=std::make_unique<ChildProcess>();
  if(!child->start(StringArray{File::getSpecialLocation(File::currentExecutableFile).getFullPathName(),"--scan-plugin-helper",request.getFullPathName()},0)){child.reset();return false;}
  startedAt=Time::getMillisecondCounterHiRes();return true;
 }
 void cleanupChild(){if(child&&child->isRunning())child->kill();child.reset();if(requestDir!=juce::File{})requestDir.deleteRecursively();requestDir=juce::File{};}
 void markFailed(const juce::String& reason){
  if(currentPath.isNotEmpty()){known.addToBlacklist(currentPath);failures.add(currentPath+" | "+reason);persist();pedal().deleteFile();}
  else failures.add("Discovery | "+reason);
 }
 void finish(bool cancelled=false){
  stopTimer();cleanupChild();if(scanOwner==this){pedal().deleteFile();scanOwner=nullptr;}currentPath.clear();scanning=false;discovering=false;known.scanFinished();persist();updateList(true);
  if(!failures.isEmpty())directory().getChildFile("plugin-scan-failures.txt").appendText(failures.joinIntoString("\n")+"\n");
  status.setText((cancelled?juce::String("Stopped. "):juce::String("Finished. "))+juce::String(known.getNumTypes())+" discovered | "+juce::String(known.getBlacklistedFiles().size())+" skipped | "+juce::String(failures.size())+" failures. See plugin-scan-failures.txt.",juce::dontSendNotification);
 }
 void beginNext(){
  while(nextCandidate<queue.size()){
   auto item=queue[nextCandidate++];if(known.getBlacklistedFiles().contains(item.path))continue;
   // Existing VST3 entries with the same filesystem timestamp need no rescan.
   bool cached=false,changed=false;
   if(item.format=="VST3"&&juce::File::isAbsolutePath(item.path)){
    juce::File f(item.path);if(f.exists())for(auto& d:known.getTypes())if(d.fileOrIdentifier==item.path&&d.pluginFormatName==item.format){cached=true;if(d.lastFileModTime!=f.getLastModificationTime())changed=true;}
   }
   if(cached&&!changed)continue;
   currentPath=item.path;status.setText("Scanning "+juce::String((int)nextCandidate)+"/"+juce::String((int)queue.size())+" | "+item.path,juce::dontSendNotification);
   if(!pedal().replaceWithText(currentPath+"\n")){failures.add("Cannot write scan recovery marker.");finish();return;}
   if(launch("scan",item))return;
   failures.add("Could not launch scan helper: "+currentPath);finish();return;
  }finish();
 }
 void timerCallback() override{
  using namespace juce;
  if(!scanning){stopTimer();return;}if(!e.canEditInserts()){finish(true);return;}
  if(!child){beginNext();return;}
  if(child->isRunning()){
   if(Time::getMillisecondCounterHiRes()-startedAt>(discovering?discoveryTimeoutMs:scanTimeoutMs)){
    cleanupChild();markFailed("Timed out");if(discovering){finish();return;}currentPath.clear();updateList(true);
   }return;
  }
  auto code=child->getExitCode();auto resultFile=requestDir.getChildFile("result.xml");
  auto xml=(code==0&&resultFile.existsAsFile()&&resultFile.getSize()<=32*1024*1024)?XmlDocument::parse(resultFile):nullptr;
  bool ok=xml&&xml->hasTagName("PluginScanResult");
  if(discovering){
   if(ok){for(auto* x:xml->getChildWithTagNameIterator("Candidate")){Candidate item{x->getStringAttribute("format"),x->getStringAttribute("path")};if(item.path.isNotEmpty()&&queue.size()<20000)queue.push_back(item);}}
   cleanupChild();discovering=false;if(!ok){markFailed("Discovery helper failed");finish();return;}
   status.setText("Found "+String((int)queue.size())+" candidates. Scanning individually...",dontSendNotification);return;
  }
  Array<PluginDescription> found;
  if(ok){for(auto* x:xml->getChildWithTagNameIterator("PLUGIN")){PluginDescription d;if(d.loadFromXml(*x))found.add(d);}}
  cleanupChild();
  if(!ok||found.isEmpty())markFailed("Plugin failed, crashed, or returned no supported types");
  else{
   for(auto& d:known.getTypes())if(d.fileOrIdentifier==currentPath)known.removeType(d);
   for(auto& d:found)known.addType(d);
   persist();pedal().deleteFile();
  }
  currentPath.clear();updateList(true);
 }
 bool beginScan(){
  if(scanning||scanOwner!=nullptr||!e.canEditInserts())return false;
  if(!directory().createDirectory().wasOk())return false;
  scanOwner=this;scanning=true;discovering=false;nextCandidate=0;queue.clear();failures.clear();currentPath.clear();updateButtons();startTimer(100);return true;
 }
 void startDefaultScan(){
  if(!beginScan())return;discovering=true;status.setText("Finding installed plugins in isolated helper...",juce::dontSendNotification);
  if(!launch("discover",{})){markFailed("Could not start discovery helper");finish();}
 }
 void chooseFile(){
  if(scanning||scanOwner!=nullptr||!e.canEditInserts())return;
  chooser=std::make_unique<juce::FileChooser>("Scan VST3 or Audio Unit bundle",juce::File{},"*.vst3;*.component");
  juce::Component::SafePointer<PluginBrowser> safe(this);
  chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::canSelectDirectories,[safe](const juce::FileChooser& f){
   if(!safe||f.getResult()==juce::File{}||!safe->beginScan())return;
   auto path=f.getResult().getFullPathName();safe->known.removeFromBlacklist(path);safe->queue.push_back({"",path});
  });
 }

public:
 PluginBrowser(Engine& engine,std::function<void(const juce::PluginDescription&)> callback,bool showInstruments=false):e(engine),instruments(showInstruments),onLoad(std::move(callback)){
  for(auto* c:std::array<juce::Component*,13>{&heading,&status,&search,&manufacturer,&category,&list,&scan,&file,&cancel,&load,&favorite,&onlyFavorites,&clearBlacklist})addAndMakeVisible(c);
  heading.setText(instruments?"PLUGIN LIBRARY / INSTRUMENTS":"PLUGIN LIBRARY / STEREO EFFECTS",juce::dontSendNotification);PulseTheme::style(heading,17,true);PulseTheme::style(status,11,false,PulseTheme::muted);
  search.setTextToShowWhenEmpty("Search plugin, manufacturer, category...",PulseTheme::muted);search.onTextChange=[this]{updateList();};
  manufacturer.onChange=[this]{updateList();};category.onChange=[this]{updateList();};
  list.setRowHeight(28);list.setMultipleSelectionEnabled(false);list.setColour(juce::ListBox::backgroundColourId,PulseTheme::panel);
  scan.onClick=[this]{startDefaultScan();};file.onClick=[this]{chooseFile();};cancel.onClick=[this]{finish(true);};load.onClick=[this]{loadSelected();};
  favorite.onClick=[this]{int row=list.getSelectedRow();if(!juce::isPositiveAndBelow(row,filtered.size()))return;auto id=filtered[row].createIdentifierString();if(favorites.contains(id))favorites.removeString(id);else favorites.add(id);persist();updateList();};
  onlyFavorites.setClickingTogglesState(true);onlyFavorites.onClick=[this]{updateList();};
  clearBlacklist.onClick=[this]{if(scanning||scanOwner!=nullptr||!e.canEditInserts())return;known.clearBlacklistedFiles();pedal().deleteFile();persist();status.setText("Skipped plugins cleared. Scan again only if safe.",juce::dontSendNotification);};
  restore();updateList(true);status.setText(juce::String(known.getNumTypes())+" cached plugins. Scan installed effects or a specific bundle.",juce::dontSendNotification);setSize(740,520);
 }
 void shutdown(){if(scanning)finish(true);else stopTimer();}
 ~PluginBrowser()override{list.setModel(nullptr);shutdown();}
 void resized()override{
  auto r=getLocalBounds().reduced(10);heading.setBounds(r.removeFromTop(29));auto row=r.removeFromTop(30);search.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));manufacturer.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));category.setBounds(row.reduced(2));
  row=r.removeFromTop(32);int n=4;for(auto* b:{&scan,&file,&cancel,&clearBlacklist})b->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2));
  status.setBounds(r.removeFromBottom(28));row=r.removeFromBottom(32);n=3;for(auto* b:{&onlyFavorites,&favorite,&load})b->setBounds(row.removeFromLeft(row.getWidth()/n--).reduced(2));list.setBounds(r.reduced(2));
 }
};
