#pragma once
#include "TestDiagnostics.h"
#include "AudioEngine.h"
namespace pulse {
inline bool testOutputMixes(){
 Engine engine;if(!engine.addChannel())return pulse::testFailure(__FILE__,__LINE__);
 constexpr int samples=2048;
 engine.playback.setSize(2,samples);
 for(auto& b:engine.buses)b.setSize(2,samples);
 for(auto& c:engine.ch)c.source=-1;
 auto& ch=engine.ch[0];ch.source=0;ch.rightSource=1;ch.gain=1;ch.mute=false;ch.bus=3;
 engine.enabled=true;engine.master=.5f;
 engine.auxSend[3*auxMixCount]=.5f;engine.auxGain[0]=.25f;engine.auxArmed[0]=true;
 juce::AudioBuffer<float> input(2,samples),output(defaultOutputChannels,samples);
 for(int i=0;i<samples;++i){input.setSample(0,i,.2f);input.setSample(1,i,.4f);}
 const float* inputs[]={input.getReadPointer(0),input.getReadPointer(1)};
 float* outputs[defaultOutputChannels];for(int c=0;c<defaultOutputChannels;++c)outputs[c]=output.getWritePointer(c);
 juce::AudioIODeviceCallbackContext context{};
 engine.audioDeviceIOCallbackWithContext(inputs,2,outputs,defaultOutputChannels,samples,context);
 auto close=[](float a,float b){return std::abs(a-b)<.001f;};
 if(!engine.auxAvailable[0].load()||!close(output.getSample(0,samples-1),.1f)||!close(output.getSample(1,samples-1),.2f)||!close(output.getSample(2,samples-1),.025f)||!close(output.getSample(3,samples-1),.05f)||!close(output.getSample(4,samples-1),0.f))return pulse::testFailure(__FILE__,__LINE__);
 engine.disarmAux();engine.audioDeviceIOCallbackWithContext(inputs,2,outputs,defaultOutputChannels,samples,context);
 if(!close(output.getSample(2,samples-1),0.f)||!close(output.getSample(3,samples-1),0.f))return pulse::testFailure(__FILE__,__LINE__);
 engine.auxArmed[0]=true;engine.audioDeviceIOCallbackWithContext(inputs,2,outputs,2,samples,context);
 if(engine.auxAvailable[0].load()||!close(engine.auxPeak[0].load(),0.f))return pulse::testFailure(__FILE__,__LINE__);
 // Route master to physical callback slots 15/16 and auxiliary to 31/32.
 engine.masterPair=7;engine.auxPair[0]=15;engine.auxPair[1]=-1;engine.auxPair[2]=-1;
 juce::AudioBuffer<float> wide(32,samples);float* wideOut[32];for(int i=0;i<32;++i)wideOut[i]=wide.getWritePointer(i);
 engine.audioDeviceIOCallbackWithContext(inputs,2,wideOut,32,samples,context);
 if(!close(wide.getSample(14,samples-1),.1f)||!close(wide.getSample(15,samples-1),.2f)||!close(wide.getSample(30,samples-1),.025f)||!close(wide.getSample(31,samples-1),.05f)||!close(wide.getSample(0,samples-1),0.f))return pulse::testFailure(__FILE__,__LINE__);
 engine.auxPair[0]=7;engine.audioDeviceIOCallbackWithContext(inputs,2,wideOut,32,samples,context);
 return !engine.auxAvailable[0].load()&&close(wide.getSample(14,samples-1),.1f);
}
}
