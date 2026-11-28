#pragma once
#include <array>
#include <algorithm>
enum class Workspace { Dashboard, Live, Mixer, Routing, AudioDevices, SoundGrid, Broadcast, Recording, AI, Setlist, Snapshots, Settings, Analytics, Plugins, Instruments, Mastering };
class WorkspaceManager {
 Workspace active=Workspace::Dashboard;
public:
 static constexpr int count=16;
 inline static constexpr std::array<const char*,count> names{"Dashboard","Live Performance","Mixer","Routing Matrix","Audio Devices","External Audio","Broadcast Control","Arrangement","AI Engineer","Setlist Manager","Snapshots & Scenes","Settings","Analytics","Plugin Manager","Instruments","Mastering"};
 inline static constexpr std::array<const char*,count> descriptions{"System overview & service health","Performance and playback","Channel and bus mixing","Channel-to-bus assignments","Universal audio device discovery and configuration","External processing integration","Stereo output monitoring","Audio playback and recording","Assisted and automatic level matching","Song library and playback order","Session mix snapshots","Audio device and project configuration","Live engine metrics","Native plugin library and insert rack","MIDI and instrument bounce","Master processing and analysis"};
 void select(int i){active=static_cast<Workspace>(std::clamp(i,0,count-1));}
 int index() const{return static_cast<int>(active);}
 Workspace current() const{return active;}
 bool is(Workspace w) const{return active==w;}
};
