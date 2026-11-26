#include "Dsp.h"
#include "ToneDSP.h"
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
int main(){testTone();pulse::StripDSP d; float y=0; for(int i=0;i<48000;i++)y=d.process(0.5f,1,0,1,1,.01f,.001f,.02f); assert(std::abs(y-.5f)<.001f); for(int i=0;i<48000;i++)y=d.process(.5f,1,pulse::coefficient(80,48000),1,1,.01f,.001f,.02f); assert(std::abs(y)<.001f); d.reset(); for(int i=0;i<48000;i++)y=d.process(.8f,1,0,.2f,4,.01f,.001f,.02f); assert(y<.4f && y>.2f); std::cout<<"DSP tests passed\n";}
