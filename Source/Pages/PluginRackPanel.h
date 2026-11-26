#pragma once
#include "../MixerStrip.h"
#include "../UI/EnterpriseTheme.h"
class PluginRackPanel : public Component {
 Engine& e;OwnedArray<Strip>& strips;std::array<String,B>& names;
 ComboBox destination,slotChoice;Label heading,note,detail;TextButton load{"Load effect"},remove{"Remove"},bypass{"Bypass"},editorButton{"Open editor"};
 std::unique_ptr<FileChooser> chooser;
 struct EditorWindow : DocumentWindow {
  explicit EditorWindow(const String& name):DocumentWindow(name,PulseTheme::panel,DocumentWindow::closeButton){setUsingNativeTitleBar(true);}
  void closeButtonPressed() override {setVisible(false);}
 };
 std::unique_ptr<EditorWindow> window;
 int rackIndex() const {return jlimit(0,N+B-1,destination.getSelectedId()-1);}
 int slotIndex() const {return jlimit(0,pulse::insertsPerRack-1,slotChoice.getSelectedId()-1);}
 pulse::InsertSlot& selected(){return e.inserts[rackIndex()].slots[slotIndex()];}
 void message(const String& error){if(error.isNotEmpty())AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon,"Plugin rack",error);}
public:
 PluginRackPanel(Engine& engine,OwnedArray<Strip>& channels,std::array<String,B>& buses):e(engine),strips(channels),names(buses){
  for(auto* c:std::array<Component*,9>{&destination,&slotChoice,&heading,&note,&detail,&load,&remove,&bypass,&editorButton})addAndMakeVisible(c);
  heading.setText("CHANNEL & BUS INSERT RACKS",dontSendNotification);PulseTheme::style(heading,16,true);
  note.setText("Two serial stereo inserts per channel or bus. Load/remove with live, playback and recording stopped. Zero-reported-latency effects only. Plugin state is session-only.",dontSendNotification);note.setJustificationType(Justification::topLeft);PulseTheme::style(note,12,false,PulseTheme::muted);detail.setJustificationType(Justification::topLeft);
  for(int i=0;i<N;++i)destination.addItem("Channel "+String(i+1),i+1);
  for(int b=0;b<B;++b)destination.addItem("Bus "+String(b+1),N+b+1);
  destination.setSelectedId(1,dontSendNotification);for(int i=0;i<pulse::insertsPerRack;++i)slotChoice.addItem("Insert "+String(i+1),i+1);slotChoice.setSelectedId(1,dontSendNotification);
  destination.onChange=[this]{refresh();};slotChoice.onChange=[this]{refresh();};
  load.onClick=[this]{if(!e.canEditInserts()){message("Stop playback and recording and disable live inputs first.");return;}int rack=rackIndex(),slot=slotIndex();chooser=std::make_unique<FileChooser>("Select stereo VST3 or Audio Unit",File{},"*.vst3;*.component");Component::SafePointer<PluginRackPanel> safe(this);chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::canSelectDirectories,[safe,rack,slot](const FileChooser& f){if(safe==nullptr||f.getResult()==File{})return;safe->window.reset();safe->message(safe->e.changeInsert(rack,slot,f.getResult()));safe->refresh();});};
  remove.onClick=[this]{if(!e.canEditInserts()){message("Stop playback and recording and disable live inputs first.");return;}window.reset();message(e.changeInsert(rackIndex(),slotIndex(),File{}));refresh();};
  bypass.setClickingTogglesState(true);bypass.setColour(TextButton::buttonOnColourId,Colour(0xff967322));bypass.onClick=[this]{selected().bypass=bypass.getToggleState();refresh();};
  editorButton.onClick=[this]{auto* p=selected().plugin.get();if(!p)return;window.reset();auto* editor=p->createEditorIfNeeded();if(!editor){message("This effect has no native editor.");return;}window=std::make_unique<EditorWindow>(p->getName());window->setContentOwned(editor,true);window->setResizable(false,false);window->centreWithSize(std::max(120,editor->getWidth()),std::max(100,editor->getHeight()));window->setVisible(true);};
  refresh();
 }
 void refresh(){for(int i=0;i<N;++i){String n="CH "+String(i+1)+" | "+(i<strips.size()?strips[i]->name.getText():String());if(destination.getItemText(i)!=n)destination.changeItemText(i+1,n);}for(int b=0;b<B;++b){String n="BUS "+String(b+1)+" | "+names[b];if(destination.getItemText(N+b)!=n)destination.changeItemText(N+b+1,n);}auto& s=selected();bool loaded=s.plugin!=nullptr;bool idle=!e.enabled.load()&&!e.transport.isPlaying();load.setEnabled(idle);remove.setEnabled(loaded&&idle);bypass.setEnabled(loaded);editorButton.setEnabled(loaded&&s.plugin->hasEditor());bypass.setToggleState(s.bypass.load(),dontSendNotification);
  String text;for(int i=0;i<pulse::insertsPerRack;++i){auto& item=e.inserts[rackIndex()].slots[i];text+="Insert "+String(i+1)+": "+(item.plugin?item.plugin->getName():"Empty");if(item.plugin){if(item.plugin->getLatencySamples()!=0)text+=" [latency guard: skipped]";else if(item.bypass.load())text+=" [bypassed]";else text+=" [enabled]";}text+="\n";}detail.setText(text,dontSendNotification);}
 void paint(Graphics& g) override {g.setColour(PulseTheme::panel);g.fillRoundedRectangle(getLocalBounds().toFloat(),6);}
 void resized() override {auto r=getLocalBounds().reduced(12);heading.setBounds(r.removeFromTop(30));auto row=r.removeFromTop(30);destination.setBounds(row.removeFromLeft(row.getWidth()*2/3).reduced(2));slotChoice.setBounds(row.reduced(2));row=r.removeFromTop(32);int remaining=4;for(auto* b:{&load,&remove,&bypass,&editorButton})b->setBounds(row.removeFromLeft(row.getWidth()/remaining--).reduced(2));detail.setBounds(r.removeFromTop(70));note.setBounds(r.removeFromTop(90));}
};
