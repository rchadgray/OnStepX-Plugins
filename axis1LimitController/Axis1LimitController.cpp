// Axis1LimitControllerPlugin plugin
#include "Axis1LimitController.h"
#include "Config.h"
#include "../../Common.h"
#include "../../lib/serial/Serial_Local.h"

// The command handler for the plugin
bool Axis1LimitControllerPlugin::command(char reply[], char command[], char parameter[], bool *suppressFrame, bool *numericReply, CommandError *commandError) {
  if (command[0] == 'X' && command[1] == 'A' && command[2] == 'L') {
    if (parameter[0] == 'D') { // Disable
      SERIAL_LOCAL.transmit(":SXA1,2,-360#"); // Set Axis1 min limit
      SERIAL_LOCAL.transmit(":SXA1,3,360#");  // Set Axis1 max limit
      strcpy(reply, "Axis1 limits disabled (-360 to 360)");
    } else if (parameter[0] == 'E') { // Enable
      char cmd[20];
      sprintf(cmd, ":SXA1,2,%d#", POLAR_ALIGN_LIMIT_MIN_DEFAULT);
      SERIAL_LOCAL.transmit(cmd);
      sprintf(cmd, ":SXA1,3,%d#", POLAR_ALIGN_LIMIT_MAX_DEFAULT);
      SERIAL_LOCAL.transmit(cmd);
      strcpy(reply, "Axis1 limits enabled");
    } else {
      strcpy(reply, "Usage: :XALD# (disable), :XALE# (enable)");
    }
    *suppressFrame = false;
    *numericReply = false;
    return true;
  }
  return false;
}

void Axis1LimitControllerPlugin::init() {
  VLF("MSG: Plugins, starting: Axis1LimitController");
}

Axis1LimitControllerPlugin axis1LimitControllerPlugin;
