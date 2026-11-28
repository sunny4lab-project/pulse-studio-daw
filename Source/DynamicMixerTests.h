#pragma once
#include "TestDiagnostics.h"
#include "PluginProject.h"
namespace pulse {
inline bool testDynamicMixer(){
 Engine e;
 if(e.channelCount()!=0||B!=10||e.rackCount()!=10)return pulse::testFailure(__FILE__,__LINE__);
 if(!e.addChannel())return pulse::testFailure(__FILE__,__LINE__);
 auto* first=&e.ch[0];auto* rack=&e.rackAt(0);auto* bus=&e.rackAt(e.channelCount()+9);
 e.ch[0].gain=.37f;e.ch[0].bus=1;
 for(int i=1;i<128;++i)if(!e.addChannel())return pulse::testFailure(__FILE__,__LINE__);
 if(e.channelCount()!=128||&e.ch[0]!=first||&e.rackAt(0)!=rack||&e.rackAt(e.channelCount()+9)!=bus)return pulse::testFailure(__FILE__,__LINE__);
 if(!e.removeChannel(63)||e.channelCount()!=127||&e.ch[0]!=first||&e.rackAt(e.channelCount()+9)!=bus||std::abs(e.ch[0].gain.load()-.37f)>.0001f)return pulse::testFailure(__FILE__,__LINE__);
 auto& c=e.ch[126];c.source=0;c.rightSource=1;c.bus=9;c.gain=1;c.mute=false;
 constexpr int n=2048;
 e.playback.setSize(2,n);e.pluginBuffer.setSize(2,n);for(auto& b:e.buses)b.setSize(2,n);
 juce::AudioBuffer<float> in(2,n),out(2,n);for(int i=0;i<n;++i){in.setSample(0,i,.2f);in.setSample(1,i,.4f);}
 const float* input[]={in.getReadPointer(0),in.getReadPointer(1)};
 float* output[]={out.getWritePointer(0),out.getWritePointer(1)};
 e.enabled=true;e.master=.5f;
 if(e.addChannel()||e.removeChannel(0))return pulse::testFailure(__FILE__,__LINE__);
 e.audioDeviceIOCallbackWithContext(input,2,output,2,n,{});
 if(std::abs(out.getSample(0,n-1)-.1f)>.001f||std::abs(out.getSample(1,n-1)-.2f)>.001f||e.busPeak[9].load()<.39f)return pulse::testFailure(__FILE__,__LINE__);
 e.enabled=false;
 juce::XmlElement root("PulseProject");root.setAttribute("version",3);root.setAttribute("channelCount",e.channelCount());
 if(PluginProject::save(e,root).isNotEmpty())return pulse::testFailure(__FILE__,__LINE__);
 if(PluginProject::restore(e,root,127).isNotEmpty()||e.channelCount()!=127||e.rackCount()!=137)return pulse::testFailure(__FILE__,__LINE__);
 while(e.channelCount())if(!e.removeChannel(e.channelCount()-1))return pulse::testFailure(__FILE__,__LINE__);
 return e.channelCount()==0&&e.rackCount()==10;
}
}
