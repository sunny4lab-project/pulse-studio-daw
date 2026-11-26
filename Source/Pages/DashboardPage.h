#pragma once
#include "../Dashboard.h"
class DashboardPage : public juce::Component {
 Dashboard content;
public:
 explicit DashboardPage(Engine& e):content(e){addAndMakeVisible(content);}
 void refresh(){content.refresh();}
 void resized() override {content.setBounds(getLocalBounds());}
};
