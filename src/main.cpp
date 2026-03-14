#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>

#include "bridge_runtime.h"
#include "bridge_types.h"
#include "config.h"
#include "config_store.h"
#include "web_ui.h"
#include "wifi_manager.h"

namespace {

BridgeConfig gConfig;
Preferences gPrefs;
WebServer gWeb(cfg::kWebPort);
WiFiServer gTcpServer(cfg::kTcpPort);
WiFiClient gTcpClient;

bool gRestartScheduled = false;
uint32_t gRestartAt = 0;

void scheduleRestart(uint32_t delayMs) {
  gRestartScheduled = true;
  gRestartAt = millis() + delayMs;
}

void handleRoot() { gWeb.send(200, "text/html", webui::renderPage(gConfig, wifi_manager::currentIp())); }

void handleStatus() {
  String body;
  body.reserve(256);
  body += "mode=";
  body += webui::modeToString(gConfig.wifiMode);
  body += "\nip=";
  body += wifi_manager::currentIp();
  body += "\nbaud=";
  body += String(gConfig.baudRate);
  body += "\n";
  gWeb.send(200, "text/plain", body);
}

void handleSave() {
  BridgeConfig next;
  String errorMessage;
  if (!webui::parseConfigFromRequest(gWeb, gConfig, next, errorMessage)) {
    gWeb.send(400, "text/plain", errorMessage);
    return;
  }

  config_store::saveConfig(gPrefs, next);
  gConfig = next;

  gWeb.send(200, "text/html",
            "<html><body><h2>Saved</h2><p>Settings stored. Rebooting now...</p></body></html>");
  scheduleRestart(800);
}

void setupWeb() {
  gWeb.on("/", HTTP_GET, handleRoot);
  gWeb.on("/status", HTTP_GET, handleStatus);
  gWeb.on("/save", HTTP_POST, handleSave);
  gWeb.begin();
  Serial.println("Web server started on port 80.");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("MobisquirtBridge booting...");

  config_store::loadConfig(gPrefs, gConfig);
  if (wifi_manager::setupWifi(gConfig)) {
    config_store::saveConfig(gPrefs, gConfig);
  }
  setupWeb();
  bridge_runtime::setupBridge(gTcpServer, Serial1, gConfig);

  Serial.printf("Open config UI: http://%s/\n", wifi_manager::currentIp().c_str());
}

void loop() {
  gWeb.handleClient();
  bridge_runtime::serviceTcpBridge(gTcpServer, gTcpClient, Serial1);

  if (gRestartScheduled && static_cast<int32_t>(millis() - gRestartAt) >= 0) {
    Serial.println("Restarting...");
    ESP.restart();
  }
}
