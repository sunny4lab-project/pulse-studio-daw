#pragma once
#include "../MixerStrip.h"
#include "../DelayAlignment.h"
#include "../UI/EnterpriseTheme.h"
class PluginRackPanel : public Component {
 Engine& e;OwnedArray<Strip>& strips;std::array<String,B>& names;
 ComboBox destination,slotChoice;Label heading,note,detail,latency;TextButton load{"Load effect"},remove{"Remove"},bypass{"Bypass"},editorButton{"Open editor"},savePreset{"Save preset"},loadPreset{"Recall preset"};
 std::unique_ptr<FileChooser> chooser;
 struct EditorWindow : DocumentWindow {
  explicit EditorWindow(const String& name):DocumentWindow(name,PulseTheme::panel,DocumentWindow::closeButton){setUsingNativeTitleBar(true);}
  void closeButtonPressed() override {setVisible(false);}
 };
 std::unique_ptr<EditorWindow> window;
 int rackIndex() const {return jlimit(0,e.rackCount()-1,destination.getSelectedId()-1);}
 int slotIndex() const {return jlimit(0,pulse::insertsPerRack-1,slotChoice.getSelectedId()-1);}
 pulse::InsertSlot& selected(){return e.rackAt(rackIndex()).slots[slotIndex()];}
 void message(const String& error){if(error.isNotEmpty())AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Plugin rack",error);}
public:
 void closeEditor(){chooser.reset();window.reset();}
 PluginRackPanel(Engine& engine,OwnedArray<Strip>& channels,std::array<String,B>& buses):e(engine),strips(channels),names(buses){
  for(auto* c:std::array<Component*,12>{&destination,&slotChoice,&heading,&note,&detail,&load,&remove,&bypass,&editorButton,&savePreset,&loadPreset,&latency})addAndMakeVisible(c);
  heading.setText("CHANNEL & BUS INSERT RACKS",dontSendNotification);PulseTheme::style(heading,16,true);
  note.setText("Two serial stereo inserts per channel or bus. Load/remove with live, playback and recording stopped. Zero-reported-latency effects only. Save/recall .pulsefx state for the same installed effect. Rack assignment and effect state are saved in projects.",dontSendNotification);note.setJustificationType(Justification::topLeft);PulseTheme::style(note,12,false,PulseTheme::muted);detail.setJustificationType(Justification::topLeft);latency.setJustificationType(Justification::topLeft);PulseTheme::style(latency,12,false,PulseTheme::muted);
  
  destination.setSelectedId(1,dontSendNotification);for(int i=0;i<pulse::insertsPerRack;++i)slotChoice.addItem("Insert "+String(i+1),i+1);slotChoice.setSelectedId(1,dontSendNotification);
  destination.onChange=[this]{refresh();};slotChoice.onChange=[this]{refresh();};
  load.onClick=[this]{if(!e.canEditInserts()){message("Stop playback and recording and disable live inputs first.");return;}int rack=rackIndex(),slot=slotIndex();chooser=std::make_unique<FileChooser>("Select stereo VST3 or Audio Unit",File{},"*.vst3;*.component");Component::SafePointer<PluginRackPanel> safe(this);chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::canSelectDirectories,[safe,rack,slot](const FileChooser& f){if(safe==nullptr||f.getResult()==File{})return;safe->window.reset();safe->message(safe->e.changeInsert(rack,slot,f.getResult()));safe->refresh();});};
  remove.onClick=[this]{if(!e.canEditInserts()){message("Stop playback and recording and disable live inputs first.");return;}window.reset();message(e.changeInsert(rackIndex(),slotIndex(),File{}));refresh();};
  bypass.setClickingTogglesState(true);bypass.setColour(TextButton::buttonOnColourId,Colour(0xff967322));bypass.onClick=[this]{selected().bypass=bypass.getToggleState();refresh();};
  editorButton.onClick=[this]{auto* p=selected().plugin.get();if(!p)return;window.reset();auto* editor=p->createEditorIfNeeded();if(!editor){message("This effect has no native editor.");return;}window=std::make_unique<EditorWindow>(p->getName());window->setContentOwned(editor,true);window->setResizable(false,false);window->centreWithSize(std::max(120,editor->getWidth()),std::max(100,editor->getHeight()));window->setVisible(true);};
  savePreset.onClick=[this]{preset(true);};loadPreset.onClick=[this]{preset(false);};
  refresh();
 }
 void preset(bool save){
  if(!selected().plugin||!e.canEditInserts()){message("Select an installed effect and stop live audio, playback and recording.");return;}
  int rack=rackIndex(),slot=slotIndex();
  chooser=std::make_unique<FileChooser>(save?"Save effect state":"Recall effect state",File::getSpecialLocation(File::userDocumentsDirectory),"*.pulsefx");
  Component::SafePointer<PluginRackPanel> safe(this);
  chooser->launchAsync((save?FileBrowserComponent::saveMode|FileBrowserComponent::warnAboutOverwriting:FileBrowserComponent::openMode)|FileBrowserComponent::canSelectFiles,[safe,rack,slot,save](const FileChooser& f){
   if(safe==nullptr||f.getResult()==File{})return;auto& e=safe->e;auto& s=e.rackAt(rack).slots[slot];
   if(!e.canEditInserts()||!s.plugin){safe->message("Effect missing or audio is active.");return;}
   auto desc=s.plugin->getPluginDescription().createIdentifierString();auto file=f.getResult();MemoryBlock state;
   std::unique_ptr<XmlElement> xml;
   if(!save){if(file.getSize()>24*1024*1024){safe->message("Preset exceeds 24 MB limit.");return;}xml=XmlDocument::parse(file);if(!xml||!xml->hasTagName("PulseFX")||xml->getStringAttribute("plugin")!=desc||!state.fromBase64Encoding(xml->getStringAttribute("state"))||state.getSize()>16*1024*1024){safe->message("Invalid preset or different effect.");return;}}
   bool attached=e.started;if(attached)e.devices.removeAudioCallback(&e);
   if(save)s.plugin->getStateInformation(state);else{s.plugin->setStateInformation(state.getData(),(int)state.getSize());s.bypass=xml->getBoolAttribute("bypass",false);}
   if(attached)e.devices.addAudioCallback(&e);
   if(save){XmlElement doc("PulseFX");doc.setAttribute("plugin",desc);doc.setAttribute("state",state.toBase64Encoding());doc.setAttribute("bypass",s.bypass.load());if(state.getSize()>16*1024*1024||!doc.writeTo(file.withFileExtension("pulsefx")))safe->message("Preset could not be saved (16 MB state limit).");}
   safe->refresh();
  });
 }
 void refresh(){int count=e.channelCount();if(destination.getNumItems()!=count+B){destination.clear(dontSendNotification);for(int i=0;i<count;++i)destination.addItem("Channel "+String(i+1),i+1);for(int b=0;b<B;++b)destination.addItem("Bus "+String(b+1),count+b+1);destination.setSelectedId(1,dontSendNotification);}for(int i=0;i<count;++i){String n="CH "+String(i+1)+" | "+(i<strips.size()?strips[i]->name.getText():String());if(destination.getItemText(i)!=n)destination.changeItemText(i+1,n);}for(int b=0;b<B;++b){String n="BUS "+String(b+1)+" | "+names[b];if(destination.getItemText(count+b)!=n)destination.changeItemText(count+b+1,n);}auto& s=selected();bool loaded=s.plugin!=nullptr;bool idle=!e.enabled.load()&&!e.transport.isPlaying();load.setEnabled(idle);remove.setEnabled(loaded&&idle);bypass.setEnabled(loaded);editorButton.setEnabled(loaded&&s.plugin->hasEditor());savePreset.setEnabled(loaded&&idle);loadPreset.setEnabled(loaded&&idle);bypass.setToggleState(s.bypass.load(),dontSendNotification);
  String text;for(int i=0;i<pulse::insertsPerRack;++i){auto& item=e.rackAt(rackIndex()).slots[i];text+="Insert "+String(i+1)+": "+(item.plugin?item.plugin->getName():"Empty");if(item.plugin){if(item.plugin->getLatencySamples()!=0)text+=" [latency guard: skipped]";else if(item.bypass.load())text+=" [bypassed]";else text+=" [enabled]";}text+="\n";}detail.setText(text,dontSendNotification);std::array<int,B> maxInput{};std::array<bool,B> used{};auto sum=[&](int rack){int total=0;for(auto& item:e.rackAt(rack).slots)if(item.plugin&&!item.bypass.load())total+=std::max(0,item.plugin->getLatencySamples());return total;};for(int i=0;i<count;++i){int b=jlimit(0,B-1,e.ch[i].bus.load());maxInput[b]=std::max(maxInput[b],sum(i));used[b]=true;}int total=0;for(int b=0;b<B;++b)if(used[b])total=std::max(total,maxInput[b]+sum(count+b));int m=e.masterPlugin&&!e.pluginBypass.load()?std::max(0,e.masterPlugin->getLatencySamples()):0;latency.setText((total<=8192?"Proposed insert alignment: "+String(total)+" samples":"Insert alignment exceeds 8192 samples")+String("\nMaster effect: ")+String(m)+" samples | compensation not active. Nonzero-latency channel/bus inserts are skipped.",dontSendNotification);}
 void paint(Graphics& g) override {g.setColour(PulseTheme::panel);g.fillRoundedRectangle(getLocalBounds().toFloat(),6);}
 void resized() override {auto r=getLocalBounds().reduced(12);heading.setBounds(r.removeFromTop(30));auto row=r.removeFromTop(30);destination.setBounds(row.removeFromLeft(row.getWidth()*2/3).reduced(2));slotChoice.setBounds(row.reduced(2));row=r.removeFromTop(32);int remaining=4;for(auto* b:{&load,&remove,&bypass,&editorButton})b->setBounds(row.removeFromLeft(row.getWidth()/remaining--).reduced(2));row=r.removeFromTop(30);savePreset.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));loadPreset.setBounds(row.reduced(2));detail.setBounds(r.removeFromTop(70));latency.setBounds(r.removeFromTop(56));note.setBounds(r.removeFromTop(90));}
};
