#pragma once
#include "../MixerStrip.h"
#include "../DelayAlignment.h"
#include "PluginBrowser.h"
#include "../UI/EnterpriseTheme.h"
class PluginRackPanel : public Component {
 Engine& e;OwnedArray<Strip>& strips;std::array<String,B>& names;
 ComboBox destination,slotChoice;Label heading,note,latency;TextEditor detail;TextButton load{"Load effect"},remove{"Remove"},bypass{"Bypass"},editorButton{"Open editor"},savePreset{"Save preset"},loadPreset{"Recall preset"},addSlot{"+ Insert slot"},compensate{"Compensated mode"};Component::SafePointer<DialogWindow> browserDialog;
 std::unique_ptr<FileChooser> chooser;
 struct EditorWindow : DocumentWindow {
  explicit EditorWindow(const String& name):DocumentWindow(name,PulseTheme::panel,DocumentWindow::closeButton){setUsingNativeTitleBar(true);}
  void closeButtonPressed() override {setVisible(false);}
 };
 std::unique_ptr<EditorWindow> window;
 int rackIndex() const {return jlimit(0,e.rackCount(),destination.getSelectedId()-1);}
 int slotIndex() const {return jlimit(0,pulse::insertsPerRack-1,slotChoice.getSelectedId()-1);}
 pulse::InsertSlot& selected(){return e.rackAt(rackIndex()).slots[slotIndex()];}
 void message(const String& error){if(error.isNotEmpty())AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Plugin rack",error);}
public:
 void closeEditor(){chooser.reset();window.reset();if(browserDialog!=nullptr){if(auto* p=dynamic_cast<PluginBrowser*>(browserDialog->getContentComponent()))p->shutdown();browserDialog->exitModalState(0);browserDialog=nullptr;}}
 ~PluginRackPanel()override{closeEditor();}
 void openBrowserForMaster(){refresh();destination.setSelectedId(e.rackCount()+1,dontSendNotification);refresh();auto& rack=e.masterInserts;int slot=0;while(slot<pulse::insertsPerRack&&rack.slots[slot].plugin)++slot;if(slot>=pulse::insertsPerRack){message("Master rack is full. Remove an effect first.");return;}rack.visibleSlots=std::max(rack.visibleSlots,slot+1);refresh();slotChoice.setSelectedId(slot+1,dontSendNotification);openBrowser(true);}
 void openBrowser(bool master=false){if(!e.canEditInserts()){message("Stop live audio, playback and recording before opening the plugin library.");return;}if(browserDialog!=nullptr){browserDialog->toFront(true);return;}int rack=rackIndex(),slot=slotIndex();Component::SafePointer<PluginRackPanel> safe(this);DialogWindow::LaunchOptions o;o.content.setOwned(new PluginBrowser(e,[safe,master,rack,slot](const PluginDescription& desc){if(!safe)return;safe->window.reset();auto error=safe->e.changeInsertDescription(rack,slot,&desc);if(error.isEmpty()){if(safe->browserDialog!=nullptr)safe->browserDialog->exitModalState(0);}else safe->message(error);safe->refresh();}));o.dialogTitle=master?"Master plugin library":"Insert plugin library";o.dialogBackgroundColour=PulseTheme::panel;browserDialog=o.launchAsync();}
 PluginRackPanel(Engine& engine,OwnedArray<Strip>& channels,std::array<String,B>& buses):e(engine),strips(channels),names(buses){
  for(auto* c:std::array<Component*,14>{&destination,&slotChoice,&heading,&note,&detail,&load,&remove,&bypass,&editorButton,&savePreset,&loadPreset,&latency,&addSlot,&compensate})addAndMakeVisible(c);
  heading.setText("CHANNEL / BUS / MASTER INSERT RACKS",dontSendNotification);PulseTheme::style(heading,16,true);
  note.setText("Start with two slots and add up to sixteen per channel, bus or master. Load effects from the searchable library. Low-latency mode skips delayed effects; compensated mode aligns channel and bus paths up to 8192 samples. Stop audio before changes.",dontSendNotification);note.setJustificationType(Justification::topLeft);PulseTheme::style(note,12,false,PulseTheme::muted);detail.setMultiLine(true);detail.setReadOnly(true);detail.setScrollbarsShown(true);latency.setJustificationType(Justification::topLeft);PulseTheme::style(latency,12,false,PulseTheme::muted);
  
  destination.setSelectedId(1,dontSendNotification);for(int i=0;i<pulse::initialInsertSlots;++i)slotChoice.addItem("Insert "+String(i+1),i+1);slotChoice.setSelectedId(1,dontSendNotification);
  destination.onChange=[this]{refresh();};slotChoice.onChange=[this]{refresh();};
  load.onClick=[this]{openBrowser();};
  addSlot.onClick=[this]{if(!e.canEditInserts())return;auto& rack=e.rackAt(rackIndex());if(rack.visibleSlots<pulse::insertsPerRack){++rack.visibleSlots;refresh();slotChoice.setSelectedId(rack.visibleSlots,dontSendNotification);refresh();}};
  compensate.setClickingTogglesState(true);compensate.onClick=[this]{if(!e.setCompensated(compensate.getToggleState()))message("Stop audio before changing compensation mode.");refresh();};
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
 void refresh(){int count=e.channelCount();if(destination.getNumItems()!=count+B+1){destination.clear(dontSendNotification);for(int i=0;i<count;++i)destination.addItem("Channel "+String(i+1),i+1);for(int b=0;b<B;++b)destination.addItem("Bus "+String(b+1),count+b+1);destination.addItem("MASTER",count+B+1);destination.setSelectedId(1,dontSendNotification);}for(int i=0;i<count;++i){String n="CH "+String(i+1)+" | "+(i<strips.size()?strips[i]->name.getText():String());if(destination.getItemText(i)!=n)destination.changeItemText(i+1,n);}for(int b=0;b<B;++b){String n="BUS "+String(b+1)+" | "+names[b];if(destination.getItemText(count+b)!=n)destination.changeItemText(count+b+1,n);}auto& rack=e.rackAt(rackIndex());if(slotChoice.getNumItems()!=rack.visibleSlots){int current=slotChoice.getSelectedId();slotChoice.clear(dontSendNotification);for(int i=0;i<rack.visibleSlots;++i)slotChoice.addItem("Insert "+String(i+1),i+1);slotChoice.setSelectedId(jlimit(1,rack.visibleSlots,current),dontSendNotification);}auto& s=selected();bool loaded=s.plugin!=nullptr;bool idle=!e.enabled.load()&&!e.transport.isPlaying()&&!e.multitrack.active();addSlot.setEnabled(idle&&rack.visibleSlots<pulse::insertsPerRack);compensate.setEnabled(idle);compensate.setToggleState(e.compensated.load(),dontSendNotification);load.setEnabled(idle);remove.setEnabled(loaded&&idle);bypass.setEnabled(loaded&&(!e.compensated.load()||idle));editorButton.setEnabled(loaded&&s.plugin->hasEditor());savePreset.setEnabled(loaded&&idle);loadPreset.setEnabled(loaded&&idle);bypass.setToggleState(s.bypass.load(),dontSendNotification);
  String text;for(int i=0;i<rack.visibleSlots;++i){auto& item=rack.slots[i];text+="Insert "+String(i+1)+": "+(item.plugin?item.plugin->getName():String("Empty"));if(item.plugin){int l=item.plugin->getLatencySamples();text+=" ("+String(l)+" samples)";if(item.bypass.load())text+=" [bypassed]";else if(l!=0&&!e.compensated.load()&&rackIndex()!=e.rackCount())text+=" [skipped: low latency]";else text+=" [enabled]";}text+="\n";}if(detail.getText()!=text)detail.setText(text,false);
  int64 m=0;for(auto& item:e.masterInserts.slots)if(item.plugin&&!item.bypass.load())m+=std::max(0,item.plugin->getLatencySamples());
  latency.setText((e.alignmentFault.load()?String("Alignment budget exceeded: outputs silenced"):e.compensated.load()?String("Active path alignment: ")+String(e.alignmentSamples.load())+" samples":String("Low-latency mode: delayed channel/bus effects skipped"))+" | Master rack: "+String(m)+" samples",dontSendNotification);
 }

 void paint(Graphics& g) override {g.setColour(PulseTheme::panel);g.fillRoundedRectangle(getLocalBounds().toFloat(),6);}
 void resized() override {auto r=getLocalBounds().reduced(12);heading.setBounds(r.removeFromTop(30));auto row=r.removeFromTop(30);destination.setBounds(row.removeFromLeft(row.getWidth()*2/3).reduced(2));slotChoice.setBounds(row.reduced(2));row=r.removeFromTop(32);int remaining=4;for(auto* b:{&load,&remove,&bypass,&editorButton})b->setBounds(row.removeFromLeft(row.getWidth()/remaining--).reduced(2));row=r.removeFromTop(30);savePreset.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));loadPreset.setBounds(row.reduced(2));row=r.removeFromTop(30);addSlot.setBounds(row.removeFromLeft(row.getWidth()/2).reduced(2));compensate.setBounds(row.reduced(2));note.setBounds(r.removeFromBottom(56));latency.setBounds(r.removeFromBottom(42));detail.setBounds(r.reduced(2));}
};
