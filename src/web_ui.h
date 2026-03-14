#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "bridge_types.h"

namespace webui {

const char *modeToString(WifiMode mode);
String renderPage(const BridgeConfig &config, const String &currentIp);
bool parseConfigFromRequest(WebServer &web, const BridgeConfig &current,
                            BridgeConfig &next, String &errorMessage);

}  // namespace webui
