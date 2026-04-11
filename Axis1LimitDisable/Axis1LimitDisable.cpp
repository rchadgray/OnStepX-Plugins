// Axis1LimitDisablePlugin plugin
#include "Axis1LimitDisable.h"
#include "../../Common.h"
#include "../../lib/serial/Serial_Local.h"
#include "../../lib/serial/Commands.h"
#include "../../lib/settings/Settings.h"
#include "../../lib/mount/Mount.h"

// The states for our state machine
enum AldState {
  ALD_IDLE,
  ALD_WAIT_FOR_RESPONSE,
  ALD_PROCESS_RESPONSE,
  ALD_GET_AXIS1_POS,
  ALD_MOVE_TO_SAFE,
  ALD_WAIT_FOR_MOVE
};

// A pointer to the response from our serial command
char *aldResponse = NULL;

// This function is called when a serial command we've sent gets a response
void aldCommandCallback(char *response) {
  if (response != NULL) {
    int len = strlen(response);
    if (len > 0) {
      aldResponse = (char *)malloc(len + 1);
      strcpy(aldResponse, response);
    }
  }
}

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
      sprintf(cmd, ":SXA1,2,%.3f#", settings.get(AXIS1_LIMIT_MIN));
      SERIAL_LOCAL.transmit(cmd);
      sprintf(cmd, ":SXA1,3,%.3f#", settings.get(AXIS1_LIMIT_MAX));
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

void Axis1LimitDisablePlugin::slowTimer() {
  static AldState aldState = ALD_IDLE;
  static long timeout = 0;
  static bool limitsDisabled = false;
  
  switch (aldState) {
    case ALD_IDLE:
      // Send the :GU# command every 5 seconds
      if (SERIAL_LOCAL.isIdle(5000)) {
        commands.sendCommand("GU", "", aldCommandCallback);
        timeout = millis() + 2000; // 2 second timeout
        aldState = ALD_WAIT_FOR_RESPONSE;
      }
      break;

    case ALD_WAIT_FOR_RESPONSE:
      // If we got a response, process it
      if (aldResponse != NULL) {
        aldState = ALD_PROCESS_RESPONSE;
      } else if (millis() > timeout) {
        // If we timed out, go back to idle
        aldState = ALD_IDLE;
      }
      break;

    case ALD_PROCESS_RESPONSE: {
      // Check for 'I' (parking) in the response
      bool parking = strstr(aldResponse, "I");

      if (parking) {
        if (!limitsDisabled) {
          SERIAL_LOCAL.transmit(":SXA1,2,-360#"); // Set Axis1 min limit
          SERIAL_LOCAL.transmit(":SXA1,3,360#");  // Set Axis1 max limit
          VLF("MSG: Axis1LimitDisable, limits disabled for parking");
          limitsDisabled = true;
        }
      } else {
        if (limitsDisabled) {
          // Un-parking, get current position before enabling limits
          aldState = ALD_GET_AXIS1_POS;
        }
      }
      
      free(aldResponse);
      aldResponse = NULL;
      if (aldState == ALD_PROCESS_RESPONSE) aldState = ALD_IDLE;
      break;
    }

    case ALD_GET_AXIS1_POS:
      if (SERIAL_LOCAL.isIdle(1000)) {
        if (mount.isAxis1AltAz()) {
          commands.sendCommand("GZ", "", aldCommandCallback);
        } else {
          commands.sendCommand("GR", "", aldCommandCallback);
        }
        timeout = millis() + 2000;
        aldState = ALD_MOVE_TO_SAFE;
      }
      break;

    case ALD_MOVE_TO_SAFE: {
      if (aldResponse == NULL) {
        if (millis() > timeout) aldState = ALD_IDLE;
        break;
      }

      double currentPos;
      if (mount.isAxis1AltAz()) {
        currentPos = Mount::convertAzmToDouble(aldResponse);
      } else {
        currentPos = Mount::convertRaToDouble(aldResponse);
      }
      free(aldResponse);
      aldResponse = NULL;

      double minLimit = settings.get(AXIS1_LIMIT_MIN);
      double maxLimit = settings.get(AXIS1_LIMIT_MAX);
      
      if (currentPos < minLimit || currentPos > maxLimit) {
        if (SERIAL_LOCAL.isIdle(1000)) {
          if (mount.isAxis1AltAz()) {
            SERIAL_LOCAL.transmit(":Sz000*00:00#");
          } else {
            SERIAL_LOCAL.transmit(":Sr00:00:00#");
          }
          SERIAL_LOCAL.transmit(":MS#");
          VLF("MSG: Axis1LimitDisable, moving to safe position");
          aldState = ALD_WAIT_FOR_MOVE;
        }
      } else {
        // Already in a safe position, just enable limits
        char cmd[20];
        sprintf(cmd, ":SXA1,2,%.3f#", minLimit);
        SERIAL_LOCAL.transmit(cmd);
        sprintf(cmd, ":SXA1,3,%.3f#", maxLimit);
        SERIAL_LOCAL.transmit(cmd);
        VLF("MSG: Axis1LimitDisable, limits re-enabled after parking");
        limitsDisabled = false;
        aldState = ALD_IDLE;
      }
      break;
    }

    case ALD_WAIT_FOR_MOVE:
       if (SERIAL_LOCAL.isIdle(1000)) {
        commands.sendCommand("GU", "", aldCommandCallback);
        timeout = millis() + 2000;
        
        if (aldResponse != NULL) {
          // If response does not contain N, we're slewing
          if (strstr(aldResponse, "N")) {
            char cmd[20];
            sprintf(cmd, ":SXA1,2,%.3f#", settings.get(AXIS1_LIMIT_MIN));
            SERIAL_LOCAL.transmit(cmd);
            sprintf(cmd, ":SXA1,3,%.3f#", settings.get(AXIS1_LIMIT_MAX));
            SERIAL_LOCAL.transmit(cmd);
            VLF("MSG: Axis1LimitDisable, limits re-enabled after parking");
            limitsDisabled = false;
            aldState = ALD_IDLE;
          }
          free(aldResponse);
          aldResponse = NULL;
        } else if (millis() > timeout) {
          aldState = ALD_IDLE;
        }
      }
      break;
  }
}

void Axis1LimitDisablePlugin::init() {
  VLF("MSG: Plugins, starting: Axis1LimitDisable");
}

Axis1LimitDisablePlugin axis1LimitDisablePlugin;
