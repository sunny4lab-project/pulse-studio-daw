#pragma once
#include <juce_core/juce_core.h>
namespace pulse {
inline juce::File testLog(){return juce::File::getCurrentWorkingDirectory().getChildFile("PulseStudio-native-tests.log");}
inline void testLogLine(const juce::String& line){testLog().appendText(line+"\n");}
inline bool testFailure(const char* file,int line){testLogLine("Assertion failed: "+juce::File::createFileWithoutCheckingPath(file).getFileName()+":"+juce::String(line));return false;}
template<class F> bool runNativeTest(const char* name,F fn){testLogLine("RUN "+juce::String(name));bool ok=fn();testLogLine((ok?"PASS ":"FAIL ")+juce::String(name));return ok;}
}
