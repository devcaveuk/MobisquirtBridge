#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "bridge_types.h"

namespace bridge_runtime {

void setupBridge(WiFiServer &tcpServer, HardwareSerial &uart, const BridgeConfig &config);
void serviceTcpBridge(WiFiServer &tcpServer, WiFiClient &tcpClient, HardwareSerial &uart);
void updateBridgeStatusLed(bool connected);

}  // namespace bridge_runtime
