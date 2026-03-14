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
constexpr uint32_t kStatusLedBlinkIntervalMs = 350;

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

// UART bridge pins are defined here by board family.
#if defined(CONFIG_IDF_TARGET_ESP32S3)
constexpr int kBridgeRxPin = 17;
constexpr int kBridgeTxPin = 18;
#else
constexpr int kBridgeRxPin = 20;
constexpr int kBridgeTxPin = 21;
#endif

// Status LED can be overridden with STATUS_LED_PIN and STATUS_LED_ACTIVE_LOW.
#if defined(STATUS_LED_PIN)
constexpr int kStatusLedPin = STATUS_LED_PIN;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
constexpr int kStatusLedPin = 21;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
constexpr int kStatusLedPin = 8;
#elif defined(PIN_LED)
constexpr int kStatusLedPin = PIN_LED;
#elif defined(LED_BUILTIN)
constexpr int kStatusLedPin = LED_BUILTIN;
#else
constexpr int kStatusLedPin = -1;
#endif

#if defined(STATUS_LED_IS_NEOPIXEL)
constexpr bool kStatusLedIsNeoPixel = (STATUS_LED_IS_NEOPIXEL != 0);
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
constexpr bool kStatusLedIsNeoPixel = true;
#else
constexpr bool kStatusLedIsNeoPixel = false;
#endif

#if defined(STATUS_LED_ACTIVE_LOW)
constexpr bool kStatusLedActiveLow = (STATUS_LED_ACTIVE_LOW != 0);
#elif defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3)
constexpr bool kStatusLedActiveLow = true;
#else
constexpr bool kStatusLedActiveLow = false;
#endif

}  // namespace cfg
