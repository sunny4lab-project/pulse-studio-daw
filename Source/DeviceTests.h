#pragma once
#include "DeviceMonitor.h"
namespace pulse {
inline bool testDevicePolicy(){
 juce::AudioDeviceManager::AudioDeviceSetup a,b;
 a.inputDeviceName=a.outputDeviceName="Test interface";a.inputChannels.setRange(0,64,true);a.outputChannels.setRange(0,2,true);b=a;
 if(!sameDeviceRouting("ASIO",a,"ASIO",b)||sameDeviceRouting("ASIO",a,"Windows Audio",b))return false;
 b.inputChannels.clearBit(32);if(sameDeviceRouting("ASIO",a,"ASIO",b))return false;
 b=a;b.outputDeviceName="Other interface";if(sameDeviceRouting("ASIO",a,"ASIO",b))return false;
 Engine e;if(e.openSelectedDevice("ASIO","","",48000,256,2,2).isEmpty()||e.openSelectedDevice("ASIO","test","test",48000,256,2,1).isEmpty())return false;if(!e.addChannel())return false;auto* first=&e.ch[0];std::vector<int> inputs;
 for(int i=0;i<64;++i)inputs.push_back(i);
 if(!e.addChannels(inputs)||e.channelCount()!=65||&e.ch[0]!=first||e.rackCount()!=75)return false;
 for(int i=0;i<64;++i)if(e.ch[i+1].source.load()!=i||!e.ch[i+1].mute.load()||e.ch[i+1].rightSource.load()!=-1)return false;
 e.enabled=true;if(e.addChannels(inputs))return false;
 e.audioDeviceError("Simulated driver fault");if(e.enabled.load()||!e.deviceFault.load()||!e.deviceFaultNotice.load())return false;
 constexpr int n=16;juce::AudioBuffer<float> out(2,n);for(int k=0;k<2;++k)for(int i=0;i<n;++i)out.setSample(k,i,1.f);
 float* outputs[]={out.getWritePointer(0),out.getWritePointer(1)};
 e.audioDeviceIOCallbackWithContext(nullptr,0,outputs,2,n,{});
 return out.getMagnitude(0,n)==0.f;
}
}
