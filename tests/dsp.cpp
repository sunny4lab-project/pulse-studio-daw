#include "Dsp.h"
#include "ToneDSP.h"
#include "MasterDSP.h"
#include "DelayAlignment.h"
#include "CallbackDeadline.h"
#include "AutoMixControl.h"
#include "ChannelColours.h"
#undef NDEBUG
#include <cassert>
#include <iostream>
static void testTone(){
 pulse::ToneDSP dsp;pulse::ToneSettings t;constexpr double rate=48000;
 dsp.prepare(t,rate);assert(dsp.process(.37f)==.37f);assert(dsp.process(INFINITY)==0.f);
 auto measure=[&](float frequency,float amplitude){double power=0;for(int i=0;i<48000;++i){float y=dsp.process(amplitude*std::sin(6.28318530718*frequency*i/rate));assert(std::isfinite(y));if(i>=24000)power+=y*y;}return std::sqrt(power/24000);};
 t.frequency[1]=1000;t.gain[1]=6;dsp.prepare(t,rate);double boosted=measure(1000,.1f);assert(boosted>.135&&boosted<.15);
 t.gain[1]=-6;dsp.prepare(t,rate);double cut=measure(1000,.1f);assert(cut>.03&&cut<.04);
 t.gain[1]=0;t.gate=-30;dsp.prepare(t,rate);assert(measure(1000,.001f)<.0001);
 assert(measure(1000,.5f)>.30);
 t.gate=-80;t.deess=-32;dsp.prepare(t,rate);assert(measure(10000,.5f)<.30);
 t.bypass=true;dsp.prepare(t,rate);assert(dsp.process(.37f)==.37f);
}
static void testMaster(){
 pulse::MasterLimiter limiter;float l[4]={2.f,.5f,-3.f,0.f},r[4]={1.f,.25f,-1.5f,0.f};
 limiter.process(l,r,4,48000,-1,100,true);
 for(int i=0;i<4;++i){assert(std::abs(l[i])<=.89126f);assert(std::abs(r[i])<=.89126f);assert(std::abs(l[i]-2*r[i])<.0001f);}assert(limiter.reduction>10);
 pulse::LoudnessMeter meter;meter.reset(48000);std::array<float,480> a{};
 for(int block=0;block<500;++block){for(int i=0;i<480;++i)a[i]=.1f*std::sin(6.28318530718*1000*(block*480+i)/48000.0);meter.process(a.data(),a.data(),480);}
 assert(meter.getIntegrated()>-21.0&&meter.getIntegrated()<-19.0);
 assert(meter.getMomentary()>-21&&meter.getShortTerm()<-19);
 assert(meter.getHeldPeak()>=.0999f&&meter.getHeldPeak()<.12f);
 meter.reset(48000);a.fill(0);meter.process(a.data(),a.data(),480);assert(meter.getIntegrated()==-100);
}
static void testDelayPlan(){
 auto p=pulse::DelayPlan<3,2>::calculate({10,40,8},{0,0,1},{20,5},0,100);
 assert(p.valid&&p.total==60&&p.channelDelay[0]==30&&p.channelDelay[1]==0&&p.busDelay[1]==47&&p.playbackDelay==60);
 assert((!pulse::DelayPlan<3,2>::calculate({10,40,8},{0,0,1},{20,5},0,50).valid));
 pulse::StereoAlignDelay d;d.prepare(64);assert(d.configure(3));float l[]={1,0,0,0,0},r[]={.5f,0,0,0,0};d.process(l,r,5);assert(l[0]==0&&l[3]==1&&r[3]==.5f);assert(!d.configure(65));
}
static void testCallbackDeadline(){
 pulse::CallbackDeadline meter;
 // 480 frames at 48 kHz have a 10 ms buffer deadline.
 meter.record(.002,480,48000);assert(meter.latestPermille==200&&meter.peakPermille==200&&meter.missedDeadlines==0);
 meter.record(.012,480,48000);assert(meter.latestPermille==1200&&meter.peakPermille==1200&&meter.missedDeadlines==1);
 meter.record(.001,480,48000);assert(meter.latestPermille==100&&meter.peakPermille==1200&&meter.missedDeadlines==1);
 meter.record(-1,480,48000);meter.record(.1,0,48000);meter.record(.1,480,0);assert(meter.missedDeadlines==1);
 meter.record(1e6,480,48000);assert(meter.peakPermille==100000&&meter.missedDeadlines==2);
 meter.reset();assert(meter.latestPermille==0&&meter.peakPermille==0&&meter.missedDeadlines==0);
}
static void testAutoMixControl(){
 using pulse::AutoMixControl;
 assert(AutoMixControl::targetDb(3,4,false)==-17.f);
 assert(AutoMixControl::targetDb(7,2,true)==AutoMixControl::targetDb(7,2,false)-3.f);
 assert(AutoMixControl::targetDb(2,2,true)==AutoMixControl::targetDb(2,2,false));
 // No signal must never cause gain to rise; all proposals stay near the human anchor.
 assert(AutoMixControl::propose(.5f,0.f,3,4,false)==.5f);
 float down=AutoMixControl::propose(.5f,.9f,7,4,true);
 float up=AutoMixControl::propose(.5f,.0033f,3,4,false);
 assert(down>=.5f*AutoMixControl::gainForDb(-6.f)-.00001f&&down<=.5f);
 assert(up<=.5f*AutoMixControl::gainForDb(3.f)+.00001f);
 assert(AutoMixControl::ride(.5f,1.f,true)==.5f);
 assert(AutoMixControl::ride(.5f,.1f,false)<.5f);
 assert(AutoMixControl::ride(.5f,1.f,false)>.5f);
 assert(AutoMixControl::ride(.5f,.5f,false)==.5f);
}
static void testChannelColours(){
 assert(pulse::channelPaletteSize==12);
 for(auto colour:pulse::channelPalette)assert(pulse::validChannelColour(colour));
 assert(pulse::defaultChannelColour(3)==pulse::channelPalette[0]);
 assert(pulse::defaultChannelColour(2)==pulse::channelPalette[1]);
 assert(pulse::defaultChannelColour(4)==pulse::channelPalette[2]);
 assert(pulse::defaultChannelColour(5)==pulse::channelPalette[6]);
 assert(pulse::defaultChannelColour(1,0)==pulse::channelPalette[6]);
 assert(pulse::defaultChannelColour(1,1)==pulse::channelPalette[7]);
 assert(!pulse::validChannelColour(0x0055cb92u));
}
int main(){testTone();testMaster();testDelayPlan();testCallbackDeadline();testAutoMixControl();testChannelColours();pulse::StripDSP d; float y=0; for(int i=0;i<48000;i++)y=d.process(0.5f,1,0,1,1,.01f,.001f,.02f); assert(std::abs(y-.5f)<.001f); for(int i=0;i<48000;i++)y=d.process(.5f,1,pulse::coefficient(80,48000),1,1,.01f,.001f,.02f); assert(std::abs(y)<.001f); d.reset(); for(int i=0;i<48000;i++)y=d.process(.8f,1,0,.2f,4,.01f,.001f,.02f); assert(y<.4f && y>.2f); std::cout<<"DSP tests passed\n";}
