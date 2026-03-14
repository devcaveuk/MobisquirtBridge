#pragma once

#include <Arduino.h>

#include "bridge_types.h"

namespace wifi_manager {

bool setupWifi(BridgeConfig &config);
String currentIp();

}  // namespace wifi_manager
