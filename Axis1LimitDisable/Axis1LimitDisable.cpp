// Axis1LimitDisablePlugin plugin
#include "Axis1LimitDisable.h"
#include "Config.h"
#include "../../Common.h"
#include "../../lib/serial/Serial_Local.h"

// The command handler for the plugin
bool Axis1LimitDisablePlugin::command(char reply[], char command[], char parameter[], bool *suppressFrame, bool *numericReply, CommandError *commandError) {
  if (command[0] == 'X' && command[1] == 'A' && command[2] == 'D') {
    if (parameter[0] == 'D') { // Disable
      SERIAL_LOCAL.transmit(":SXA1,2,-360#"); // Set Axis1 min limit
      SERIAL_LOCAL.transmit(":SXA1,3,360#");  // Set Axis1 max limit
      strcpy(reply, "Axis1 limits disabled (-360 to 360)");
      VLF("MSG: Axis1LimitDisable, limits disabled");
    } else if (parameter[0] == 'E') { // Enable
      char cmd[20];
      sprintf(cmd, ":SXA1,2,%d#", POLAR_ALIGN_LIMIT_MIN_DEFAULT);
      SERIAL_LOCAL.transmit(cmd);
      sprintf(cmd, ":SXA1,3,%d#", POLAR_ALIGN_LIMIT_MAX_DEFAULT);
      SERIAL_LOCAL.transmit(cmd);
      strcpy(reply, "Axis1 limits enabled");
      VLF("MSG: Axis1LimitDisable, limits enabled");
    } else {
      strcpy(reply, "Usage: :XADD# (disable), :XADE# (enable)");
    }
    *suppressFrame = false;
    *numericReply = false;
    return true;
  }
  return false;
}

void Axis1LimitDisablePlugin::init() {
  VLF("MSG: Plugins, starting: Axis1LimitDisable");
}

Axis1LimitDisablePlugin axis1LimitDisablePlugin;
