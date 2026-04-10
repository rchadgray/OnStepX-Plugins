// Axis1LimitControllerPlugin plugin
#pragma once

class Axis1LimitControllerPlugin {
public:
  void init();
  bool command(char reply[], char command[], char parameter[], bool *suppressFrame, bool *numericReply, CommandError *commandError);
};

extern Axis1LimitControllerPlugin axis1LimitControllerPlugin;
