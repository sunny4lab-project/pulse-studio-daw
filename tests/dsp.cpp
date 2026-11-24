#include "Dsp.h"
#undef NDEBUG
#include <cassert>
#include <iostream>
int main(){pulse::StripDSP d; float y=0; for(int i=0;i<48000;i++)y=d.process(0.5f,1,0,1,1,.01f,.001f,.02f); assert(std::abs(y-.5f)<.001f); for(int i=0;i<48000;i++)y=d.process(.5f,1,pulse::coefficient(80,48000),1,1,.01f,.001f,.02f); assert(std::abs(y)<.001f); d.reset(); for(int i=0;i<48000;i++)y=d.process(.8f,1,0,.2f,4,.01f,.001f,.02f); assert(y<.4f && y>.2f); std::cout<<"DSP tests passed\n";}
