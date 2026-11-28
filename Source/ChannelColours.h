#pragma once
#include <cstdint>
namespace pulse {
inline constexpr std::uint32_t channelPalette[] = {
  0xffe4ba62u, 0xffe96c76u, 0xffaa83e8u, 0xff52a7f5u,
  0xff55cb92u, 0xffef9a54u, 0xffd75a6bu, 0xff4acfc9u,
  0xffb6c8d9u, 0xfff2df86u, 0xffd486c8u, 0xff82c8f0u
};
inline constexpr int channelPaletteSize=sizeof(channelPalette)/sizeof(channelPalette[0]);
inline constexpr std::uint32_t defaultChannelColour(int role,int bus=3) noexcept {
 switch(role){case 2:return channelPalette[1];case 3:return channelPalette[0];
 case 4:return channelPalette[2];case 5:return channelPalette[6];
 case 6:return channelPalette[4];case 7:return channelPalette[3];
 default:return channelPalette[bus==0?6:bus==1?7:bus==2?2:8];}
}
inline bool validChannelColour(std::uint32_t c) noexcept {return (c&0xff000000u)==0xff000000u;}
}
