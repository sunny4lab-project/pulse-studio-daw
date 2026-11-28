#pragma once
#include <juce_core/juce_core.h>
namespace pulse {
// Save to a temporary file first; retain one previous valid project.
inline juce::String writeProjectSafely(const juce::XmlElement& root,const juce::File& file,bool backup){
 if(!file.getParentDirectory().createDirectory().wasOk())return "Cannot create project directory.";
 juce::TemporaryFile tmp(file);
 if(!root.writeTo(tmp.getFile())||tmp.getFile().getSize()>64*1024*1024)return "Project write failed or exceeds 64 MB.";
 if(backup&&file.existsAsFile()){auto previous=file.getSiblingFile(file.getFileName()+".bak");juce::TemporaryFile copy(previous);
  if(!file.copyFileTo(copy.getFile())||!copy.overwriteTargetFileWithTemporary())return "Cannot preserve previous project; original left unchanged.";
 }
 if(!tmp.overwriteTargetFileWithTemporary())return "Cannot finalize project file.";return {};
}
}
