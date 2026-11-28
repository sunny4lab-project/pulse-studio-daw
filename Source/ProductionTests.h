#pragma once
#include "TestDiagnostics.h"
#include "MultitrackRecorder.h"
namespace pulse {
inline bool testProduction(){
 using namespace juce;
 auto root=File::getSpecialLocation(File::tempDirectory).getNonexistentChildFile("pulse-production-test","");
 if(!root.createDirectory().wasOk())return pulse::testFailure(__FILE__,__LINE__);
 struct Cleanup{File f;~Cleanup(){f.deleteRecursively();}} cleanup{root};
 TimeSliceThread disk("test recording");disk.startThread();
 MultitrackRecorder recorder(disk);auto file=root.getChildFile("four-inputs.wav");
 bool ok=recorder.start(file,48000,4,256);
 if(ok){AudioBuffer<float> buffer(4,256);const float* ptrs[4];for(int c=0;c<4;++c){for(int i=0;i<256;++i)buffer.setSample(c,i,.1f*(c+1));ptrs[c]=buffer.getReadPointer(c);}recorder.push(ptrs,4,256);recorder.stop();ok=recorder.missedBlocks.load()==0;}
 disk.stopThread(2000);if(!ok)return pulse::testFailure(__FILE__,__LINE__);
 WavAudioFormat wav;std::unique_ptr<AudioFormatReader> reader(wav.createReaderFor(file.createInputStream().release(),true));
 if(!reader||reader->numChannels!=4||reader->lengthInSamples!=256)return pulse::testFailure(__FILE__,__LINE__);
 AudioBuffer<float> buffer(4,256);if(!reader->read(&buffer,0,256,0,true,true))return pulse::testFailure(__FILE__,__LINE__);
 for(int c=0;c<4;++c)if(std::abs(buffer.getSample(c,128)-.1f*(c+1))>.001f)return pulse::testFailure(__FILE__,__LINE__);
 return true;
}
}
