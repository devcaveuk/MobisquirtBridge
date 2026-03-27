#pragma once

#include <Arduino.h>

#include "bridge_types.h"

namespace ble_bridge {

void setupBle(const BridgeConfig &config);
void shutdownBle();
void serviceBle(HardwareSerial &uart);
bool isBleConnected();

}  // namespace ble_bridge
