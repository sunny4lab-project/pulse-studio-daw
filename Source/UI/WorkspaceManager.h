#pragma once
#include <array>
#include <algorithm>
enum class Workspace { Dashboard, Live, Mixer, Routing, Dante, SoundGrid, Broadcast, Recording, AI, Setlist, Snapshots, Settings, Analytics };
class WorkspaceManager {
 Workspace active=Workspace::Dashboard;
public:
 static constexpr int count=13;
 inline static constexpr std::array<const char*,count> names{"Dashboard","Live Performance","Mixer","Routing Matrix","Dante Center","SoundGrid / SuperRack","Broadcast Control","Recording Studio","AI Engineer","Setlist Manager","Snapshots & Scenes","Settings","Analytics"};
 inline static constexpr std::array<const char*,count> descriptions{"System overview & service health","Performance and playback","Channel and bus mixing","Channel-to-bus assignments","Network audio device integration","External processing integration","Stereo output monitoring","Audio playback and recording","Assisted and automatic level matching","Song library and playback order","Session mix snapshots","Audio device and project configuration","Live engine metrics"};
 void select(int i){active=static_cast<Workspace>(std::clamp(i,0,count-1));}
 int index() const{return static_cast<int>(active);}
 Workspace current() const{return active;}
 bool is(Workspace w) const{return active==w;}
};
