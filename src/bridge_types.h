#pragma once

#include <Arduino.h>

enum class WifiMode : uint8_t { AP = 0, STA = 1 };

struct BridgeConfig {
  uint32_t baudRate;
  WifiMode wifiMode;
  String deviceName;
  String staSsid;
  String staPassword;
  String apSsid;
  String apPassword;
};
