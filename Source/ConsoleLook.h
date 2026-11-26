#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
using namespace juce;
class ConsoleLook : public LookAndFeel_V4 {
public:
 void drawLinearSlider(Graphics& g,int x,int y,int w,int h,float pos,float minPos,float maxPos,const Slider::SliderStyle style,Slider& slider) override {
  if(style!=Slider::LinearVertical){LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,pos,minPos,maxPos,style,slider);return;}
  const float cx=x+w*0.48f;
  g.setColour(Colour(0xff10151a));g.fillRoundedRectangle(cx-3,y,6,(float)h,3);
  g.setColour(Colour(0xff56616a));g.drawVerticalLine((int)cx,y,y+h);
  for(float db: {3.f,0.f,-5.f,-10.f,-20.f,-30.f,-40.f,-60.f}){
   if(db<slider.getMinimum() || db>slider.getMaximum())continue;
   float yy=(float)slider.getPositionOfValue(db);
   g.setColour(db==0?Colour(0xffe0e6e9):Colour(0xff87939d));
   g.drawHorizontalLine((int)yy,cx+7,cx+12);
   g.setFont(9.f);g.drawText(String((int)db),(int)cx+13,(int)yy-6,20,12,Justification::centredLeft,false);
  }
  float cw=std::min(30.f,w*0.55f),ch=27.f;
  Rectangle<float> cap(cx-cw/2,pos-ch/2,cw,ch);
  g.setColour(Colour(0xff080b0e));g.fillRoundedRectangle(cap.translated(1,2),3);
  g.setGradientFill(ColourGradient(Colour(0xffe1e5e8),cap.getX(),cap.getY(),Colour(0xff7c8791),cap.getX(),cap.getBottom(),false));
  g.fillRoundedRectangle(cap,3);
  g.setColour(Colour(0xff424b54));g.drawRoundedRectangle(cap,3,1);
  for(int i=-2;i<=2;++i){g.setColour(Colour(0xff56616a));g.drawHorizontalLine((int)pos+i*3,cap.getX()+4,cap.getRight()-4);}
  g.setColour(Colour(0xfff6f8fa));g.drawHorizontalLine((int)pos,cap.getX()+2,cap.getRight()-2);
 }
 int getSliderThumbRadius(Slider& s) override {return s.getSliderStyle()==Slider::LinearVertical?14:LookAndFeel_V4::getSliderThumbRadius(s);}
};
