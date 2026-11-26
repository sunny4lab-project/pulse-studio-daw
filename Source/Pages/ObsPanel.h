#pragma once
#include "../ObsStatus.h"
#include "../UI/EnterpriseTheme.h"
class ObsPanel : public Component {
 pulse::ObsStatus obs;TextEditor password,port;Label heading,status;TextButton check{"Check OBS"};
public:
 ObsPanel(){for(auto* c:std::array<Component*,5>{&password,&port,&heading,&status,&check})addAndMakeVisible(c);
 heading.setText("OBS / LOCAL STATUS",dontSendNotification);PulseTheme::style(heading,12,true);
 password.setPasswordCharacter(0x2022);password.setTextToShowWhenEmpty("WebSocket password",PulseTheme::muted);
 port.setInputRestrictions(5,"0123456789");port.setText("4455",dontSendNotification);
 check.onClick=[this]{if(obs.check(port.getText().getIntValue(),password.getText())){password.clear();refresh();}else status.setText("Invalid port or check in progress.",dontSendNotification);};
 refresh();}
 void refresh(){int state=obs.state.load();check.setEnabled(state!=1);String text;
 if(state==0)text="Not checked";else if(state==1)text="Checking local OBS...";else if(state==3)text="Unavailable / authentication failed";else{auto age=Time::getCurrentTime().toMilliseconds()-obs.checkedAt.load();text="Stream "+String(obs.streaming.load()==1?"active":"idle")+" | Record "+String(obs.recording.load()==1?"active":"idle")+" | "+String((int)std::max<juce::int64>(0,age/1000))+"s ago";}
 status.setText(text,dontSendNotification);}
 void resized() override{auto r=getLocalBounds().reduced(5);heading.setBounds(r.removeFromTop(22));auto row=r.removeFromTop(26);port.setBounds(row.removeFromLeft(54).reduced(1));check.setBounds(row.removeFromRight(100).reduced(1));password.setBounds(row.reduced(1));status.setBounds(r.removeFromTop(24));}
 void paint(Graphics& g) override{g.fillAll(PulseTheme::panel);}
};
