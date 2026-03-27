#include "web_ui.h"

#include "config.h"

namespace {

String htmlEscape(const String &in) {
  String out;
  out.reserve(in.length() + 16);
  for (size_t i = 0; i < in.length(); i++) {
    const char c = in[i];
    switch (c) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      case '\'':
        out += "&#39;";
        break;
      default:
        out += c;
        break;
    }
  }
  return out;
}

String truncateTo(const String &value, size_t maxLen) {
  if (value.length() <= maxLen) {
    return value;
  }
  return value.substring(0, maxLen);
}

bool isSupportedBaud(uint32_t baud) {
  for (size_t i = 0; i < cfg::kSupportedBaudRateCount; i++) {
    if (cfg::kSupportedBaudRates[i] == baud) {
      return true;
    }
  }
  return false;
}

}  // namespace

namespace webui {

const char *modeToString(WifiMode mode) {
  return (mode == WifiMode::AP) ? "ap" : "sta";
}

String renderPage(const BridgeConfig &config, const String &currentIp) {
  String html;
  html.reserve(4096);

  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Mobisquirt Bridge Config</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;background:#f2f5f8;margin:0;padding:20px;}";
  html += ".card{max-width:720px;margin:0 auto;background:#fff;padding:20px;border-radius:12px;box-shadow:0 10px 28px rgba(0,0,0,.08);}";
  html += "h1{margin-top:0;}label{display:block;margin-top:12px;font-weight:600;}";
  html += "input,select{width:100%;padding:10px;margin-top:6px;border:1px solid #ccd2da;border-radius:8px;box-sizing:border-box;}";
  html += ".hint{color:#5e6875;font-size:.9rem;}button{margin-top:18px;padding:12px 18px;border:0;border-radius:8px;background:#0067b8;color:#fff;font-weight:700;cursor:pointer;}";
  html += ".status{background:#edf6ff;border:1px solid #c8e1ff;border-radius:8px;padding:10px;margin-top:12px;}";
  html += "</style></head><body><div class='card'>";

  html += "<h1>Mobisquirt Bridge</h1>";
  html += "<p class='hint'>Configure UART baud and Wi-Fi mode (AP or client STA). Saving reboots the device.</p>";
  html += "<div class='status'><strong>Current IP:</strong> " + htmlEscape(currentIp) + "<br><strong>TCP Port:</strong> " + String(cfg::kTcpPort) + "</div>";

  html += "<form method='POST' action='/save'>";

  html += "<label for='devname'>Device Name</label>";
  html += "<input id='devname' name='devname' maxlength='" + String(cfg::kDeviceNameMaxLen) + "' value='" + htmlEscape(config.deviceName) + "'>";

  html += "<label for='baud'>UART Baud Rate</label>";
  html += "<select id='baud' name='baud'>";
  for (size_t i = 0; i < cfg::kSupportedBaudRateCount; i++) {
    const uint32_t baud = cfg::kSupportedBaudRates[i];
    html += "<option value='" + String(baud) + "'" +
            String(config.baudRate == baud ? " selected" : "") + ">" + String(baud) +
            "</option>";
  }
  html += "</select>";

  html += "<label for='mode'>Wi-Fi Mode</label>";
  html += "<select id='mode' name='mode'>";
  html += "<option value='ap'" + String(config.wifiMode == WifiMode::AP ? " selected" : "") + ">Access Point (AP)</option>";
  html += "<option value='sta'" + String(config.wifiMode == WifiMode::STA ? " selected" : "") + ">Client (STA)</option>";
  html += "</select>";

  html += "<label for='apssid'>AP SSID</label>";
  html += "<input id='apssid' name='apssid' maxlength='" + String(cfg::kSsidMaxLen) + "' value='" + htmlEscape(config.apSsid) + "'>";

  html += "<label for='appass'>AP Password (8-63 chars, leave blank for open AP)</label>";
  html += "<input id='appass' name='appass' maxlength='" + String(cfg::kPasswordMaxLen) + "' value='" + htmlEscape(config.apPassword) + "'>";

  html += "<label for='stassid'>STA SSID</label>";
  html += "<input id='stassid' name='stassid' maxlength='" + String(cfg::kSsidMaxLen) + "' value='" + htmlEscape(config.staSsid) + "'>";

  html += "<label for='stapass'>STA Password</label>";
  html += "<input id='stapass' type='password' name='stapass' maxlength='" + String(cfg::kPasswordMaxLen) + "' value='" + htmlEscape(config.staPassword) + "'>";

  html += "<h2 style='margin-top:24px;'>Bluetooth LE Settings</h2>";
  html += "<label for='bleenabled'>Enable BLE Bridge</label>";
  html += "<select id='bleenabled' name='bleenabled'>";
  html += "<option value='off'" + String(config.bluetoothMode == BluetoothMode::OFF ? " selected" : "") + ">Off</option>";
  html += "<option value='on'" + String(config.bluetoothMode == BluetoothMode::ON ? " selected" : "") + ">On</option>";
  html += "</select>";
  html += "<p class='hint'>BLE can be used as an alternative to WiFi for UART communication</p>";

  html += "<label for='blesvc'>BLE Service UUID</label>";
  html += "<input id='blesvc' name='blesvc' maxlength='" + String(cfg::kBleUuidMaxLen) + "' value='" + htmlEscape(config.BLEServiceUuid) + "'>";

  html += "<label for='blechar'>BLE Characteristic UUID</label>";
  html += "<input id='blechar' name='blechar' maxlength='" + String(cfg::kBleUuidMaxLen) + "' value='" + htmlEscape(config.BLECharacteristicUuid) + "'>";

  html += "<label for='blepin'>BLE PIN (6 digits)</label>";
  html += "<input id='blepin' name='blepin' type='text' pattern='[0-9]{6}' maxlength='6' value='" + htmlEscape(config.BLEPin) + "'>";
  html += "<p class='hint'>6-digit PIN required for BLE pairing</p>";

  html += "<button type='submit'>Save and Reboot</button>";
  html += "</form></div></body></html>";

  return html;
}

bool parseConfigFromRequest(WebServer &web, const BridgeConfig &current, BridgeConfig &next,
                            String &errorMessage) {
  next = current;

  next.deviceName = truncateTo(web.arg("devname"), cfg::kDeviceNameMaxLen);
  if (next.deviceName.isEmpty()) {
    next.deviceName = cfg::kDefaultDeviceName;
  }

  const long baud = web.arg("baud").toInt();
  if (baud <= 0 || !isSupportedBaud(static_cast<uint32_t>(baud))) {
    errorMessage = "Invalid baud rate selection.";
    return false;
  }
  next.baudRate = static_cast<uint32_t>(baud);

  const String mode = web.arg("mode");
  next.wifiMode = (mode == "sta") ? WifiMode::STA : WifiMode::AP;

  next.apSsid = truncateTo(web.arg("apssid"), cfg::kSsidMaxLen);
  next.apPassword = truncateTo(web.arg("appass"), cfg::kPasswordMaxLen);
  next.staSsid = truncateTo(web.arg("stassid"), cfg::kSsidMaxLen);
  next.staPassword = truncateTo(web.arg("stapass"), cfg::kPasswordMaxLen);

  if (next.apSsid.isEmpty()) {
    next.apSsid = cfg::kDefaultApSsid;
  }

  if (!next.apPassword.isEmpty() && next.apPassword.length() < cfg::kMinApPasswordLen) {
    errorMessage = "AP password must be 8-63 chars, or empty for open AP.";
    return false;
  }

  if (next.wifiMode == WifiMode::STA && next.staSsid.isEmpty()) {
    errorMessage = "STA mode requires SSID.";
    return false;
  }

  const String bleEnabled = web.arg("bleenabled");
  next.bluetoothMode = (bleEnabled == "on") ? BluetoothMode::ON : BluetoothMode::OFF;

  next.BLEServiceUuid = truncateTo(web.arg("blesvc"), cfg::kBleUuidMaxLen);
  next.BLECharacteristicUuid = truncateTo(web.arg("blechar"), cfg::kBleUuidMaxLen);

  if (next.BLEServiceUuid.isEmpty()) {
    next.BLEServiceUuid = cfg::kBleServiceUuid;
  }
  if (next.BLECharacteristicUuid.isEmpty()) {
    next.BLECharacteristicUuid = cfg::kBleCharacteristicUuid;
  }

  next.BLEPin = truncateTo(web.arg("blepin"), cfg::kBlePinLen);
  if (next.BLEPin.isEmpty() || next.BLEPin.length() != cfg::kBlePinLen) {
    next.BLEPin = cfg::kBleDefaultPin;
  }
  // Validate PIN is all digits
  for (size_t i = 0; i < next.BLEPin.length(); i++) {
    if (!isdigit(next.BLEPin[i])) {
      errorMessage = "BLE PIN must be exactly 6 digits.";
      return false;
    }
  }

  return true;
}

}  // namespace webui
