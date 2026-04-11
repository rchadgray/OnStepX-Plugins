// Axis1LimitDisablePlugin plugin
#pragma once

class Axis1LimitDisablePlugin {
public:
  void init();
  void slowTimer();
  bool command(char reply[], char command[], char parameter[], bool *suppressFrame, bool *numericReply, CommandError *commandError);
};

extern Axis1LimitDisablePlugin axis1LimitDisablePlugin;
