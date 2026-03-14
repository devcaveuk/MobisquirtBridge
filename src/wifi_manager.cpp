#include "wifi_manager.h"

#include <WiFi.h>

#include "config.h"

namespace {

bool startWifiSta(const BridgeConfig &config) {
  if (config.staSsid.isEmpty()) {
    Serial.println("STA mode requested but SSID is empty.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(config.deviceName.c_str());
  WiFi.begin(config.staSsid.c_str(), config.staPassword.c_str());

  Serial.printf("Connecting to STA SSID '%s'...\n", config.staSsid.c_str());
  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < cfg::kStaConnectTimeoutMs) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("STA connected. IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
  }

  Serial.println("STA connect timeout.");
  return false;
}

void startWifiAp(const BridgeConfig &config) {
  WiFi.mode(WIFI_AP);
  WiFi.setHostname(config.deviceName.c_str());

  bool ok = false;
  if (config.apPassword.isEmpty()) {
    ok = WiFi.softAP(config.apSsid.c_str());
  } else {
    ok = WiFi.softAP(config.apSsid.c_str(), config.apPassword.c_str());
  }

  if (!ok) {
    Serial.println("Failed to start AP, retrying with open AP.");
    WiFi.softAP(config.apSsid.c_str());
  }

  Serial.printf("AP started. SSID: %s IP: %s\n", config.apSsid.c_str(),
                WiFi.softAPIP().toString().c_str());
}

}  // namespace

namespace wifi_manager {

bool setupWifi(BridgeConfig &config) {
  WiFi.persistent(false);
  WiFi.disconnect(false, false);
  delay(100);

  if (config.wifiMode == WifiMode::STA && startWifiSta(config)) {
    return false;
  }

  bool configChanged = false;
  if (config.wifiMode == WifiMode::STA) {
    Serial.println("Falling back to AP mode.");
    config.wifiMode = WifiMode::AP;
    configChanged = true;
  }

  startWifiAp(config);
  return configChanged;
}

String currentIp() {
  if (WiFi.getMode() == WIFI_MODE_AP) {
    return WiFi.softAPIP().toString();
  }
  return WiFi.localIP().toString();
}

}  // namespace wifi_manager
