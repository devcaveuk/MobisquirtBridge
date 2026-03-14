#pragma once

#include <Arduino.h>

namespace cfg {

constexpr uint32_t kDefaultBaud = 115200;
constexpr uint32_t kBaudMin = 1200;
constexpr uint32_t kBaudMax = 2000000;
constexpr uint32_t kSupportedBaudRates[] = {4800, 9600, 19200, 38400, 57600, 115200};
constexpr size_t kSupportedBaudRateCount = sizeof(kSupportedBaudRates) / sizeof(kSupportedBaudRates[0]);
constexpr uint16_t kTcpPort = 9001;
constexpr uint16_t kWebPort = 80;
constexpr uint32_t kStaConnectTimeoutMs = 15000;

constexpr size_t kDeviceNameMaxLen = 32;
constexpr size_t kSsidMaxLen = 32;
constexpr size_t kPasswordMaxLen = 63;
constexpr size_t kMinApPasswordLen = 8;

constexpr const char kPrefsNamespace[] = "bridge";
constexpr const char kPrefsBaudKey[] = "baud";
constexpr const char kPrefsWifiModeKey[] = "wmode";
constexpr const char kPrefsDeviceNameKey[] = "devname";
constexpr const char kPrefsStaSsidKey[] = "stassid";
constexpr const char kPrefsStaPasswordKey[] = "stapass";
constexpr const char kPrefsApSsidKey[] = "apssid";
constexpr const char kPrefsApPasswordKey[] = "appass";

constexpr const char kDefaultDeviceName[] = "MobisquirtBridge";
constexpr const char kDefaultApSsid[] = "MobisquirtBridge";
constexpr const char kDefaultApPassword[] = "mobisquirt123";

// Pins can be overridden per PlatformIO environment with BRIDGE_RX_PIN/BRIDGE_TX_PIN.
#if defined(BRIDGE_RX_PIN) && defined(BRIDGE_TX_PIN)
constexpr int kBridgeRxPin = BRIDGE_RX_PIN;
constexpr int kBridgeTxPin = BRIDGE_TX_PIN;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
constexpr int kBridgeRxPin = 17;
constexpr int kBridgeTxPin = 18;
#else
constexpr int kBridgeRxPin = 20;
constexpr int kBridgeTxPin = 21;
#endif

}  // namespace cfg
