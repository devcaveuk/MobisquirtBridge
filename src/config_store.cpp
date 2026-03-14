#include "config_store.h"

#include "config.h"

namespace {

String truncateTo(const String &value, size_t maxLen) {
  if (value.length() <= maxLen) {
    return value;
  }
  return value.substring(0, maxLen);
}

}  // namespace

namespace config_store {

void loadConfig(Preferences &prefs, BridgeConfig &config) {
  prefs.begin(cfg::kPrefsNamespace, true);

  config.baudRate = prefs.getULong(cfg::kPrefsBaudKey, cfg::kDefaultBaud);
  config.wifiMode = static_cast<WifiMode>(
      prefs.getUChar(cfg::kPrefsWifiModeKey, static_cast<uint8_t>(WifiMode::AP)));

  config.deviceName = prefs.getString(cfg::kPrefsDeviceNameKey, cfg::kDefaultDeviceName);
  config.staSsid = prefs.getString(cfg::kPrefsStaSsidKey, "");
  config.staPassword = prefs.getString(cfg::kPrefsStaPasswordKey, "");
  config.apSsid = prefs.getString(cfg::kPrefsApSsidKey, cfg::kDefaultApSsid);
  config.apPassword = prefs.getString(cfg::kPrefsApPasswordKey, cfg::kDefaultApPassword);

  prefs.end();

  if (config.baudRate < cfg::kBaudMin || config.baudRate > cfg::kBaudMax) {
    config.baudRate = cfg::kDefaultBaud;
  }

  if (config.wifiMode != WifiMode::AP && config.wifiMode != WifiMode::STA) {
    config.wifiMode = WifiMode::AP;
  }

  config.deviceName = truncateTo(config.deviceName, cfg::kDeviceNameMaxLen);
  config.staSsid = truncateTo(config.staSsid, cfg::kSsidMaxLen);
  config.staPassword = truncateTo(config.staPassword, cfg::kPasswordMaxLen);
  config.apSsid = truncateTo(config.apSsid, cfg::kSsidMaxLen);
  config.apPassword = truncateTo(config.apPassword, cfg::kPasswordMaxLen);

  if (config.deviceName.isEmpty()) {
    config.deviceName = cfg::kDefaultDeviceName;
  }
  if (config.apSsid.isEmpty()) {
    config.apSsid = cfg::kDefaultApSsid;
  }
}

void saveConfig(Preferences &prefs, const BridgeConfig &config) {
  prefs.begin(cfg::kPrefsNamespace, false);
  prefs.putULong(cfg::kPrefsBaudKey, config.baudRate);
  prefs.putUChar(cfg::kPrefsWifiModeKey, static_cast<uint8_t>(config.wifiMode));
  prefs.putString(cfg::kPrefsDeviceNameKey, config.deviceName);
  prefs.putString(cfg::kPrefsStaSsidKey, config.staSsid);
  prefs.putString(cfg::kPrefsStaPasswordKey, config.staPassword);
  prefs.putString(cfg::kPrefsApSsidKey, config.apSsid);
  prefs.putString(cfg::kPrefsApPasswordKey, config.apPassword);
  prefs.end();
}

}  // namespace config_store
