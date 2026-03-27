#include "ble_bridge.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_gap_ble_api.h>

#include "bridge_runtime.h"
#include "config.h"

namespace {

BLEServer *gBleServer = nullptr;
BLECharacteristic *gBleCharacteristic = nullptr;
bool gBleDeviceConnected = false;
bool gBleOldDeviceConnected = false;
bool gBleMitmProtected = false;
String gBleServiceUuid;
String gBleCharacteristicUuid;
uint32_t gBlePin = 0;

class SecurityCallbacks : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() override {
    Serial.printf("→ BLE PassKey Request - Returning PIN: %06u\n", gBlePin);
    Serial.println("  iOS should prompt for PIN entry now...");
    return gBlePin;
  }

  void onPassKeyNotify(uint32_t pass_key) override {
    Serial.printf("→ BLE PassKey Notify: %06u ", pass_key);
    if (pass_key == gBlePin) {
      Serial.println("- matches our PIN ✓");
      Serial.println("  Waiting for authentication to complete...");
    } else {
      Serial.printf("- does NOT match our PIN (%06u) ✗\n", gBlePin);
    }
  }

  bool onSecurityRequest() override {
    Serial.println("→ BLE Security Request - Accepting pairing request");
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) override {
    Serial.println("→ BLE Authentication Complete:");
    if (cmpl.success) {
      Serial.printf("  ✓ SUCCESS - Auth Mode: 0x%02X ", cmpl.auth_mode);
      bool bonded = (cmpl.auth_mode & 0x01);
      bool mitm = (cmpl.auth_mode & 0x04);
      bool sc = (cmpl.auth_mode & 0x08);
      gBleMitmProtected = mitm && bonded;
      
      Serial.printf("[%s%s%s]\n", 
                    bonded ? "BONDED " : "NOT-BONDED ",
                    mitm ? "MITM " : "NO-MITM ",
                    sc ? "SC" : "LEGACY");
      
      if (!bonded) {
        Serial.println("  ✗ BONDING FAILED - forcing disconnect");
        if (gBleServer) {
          gBleServer->disconnect(gBleServer->getConnId());
        }
      } else if (!mitm) {
        Serial.println("  ⚠ NO MITM - forcing disconnect");
        if (gBleServer) {
          gBleServer->disconnect(gBleServer->getConnId());
        }
      } else {
        Serial.println("  ✓ Properly bonded with MITM protection");
      }
    } else {
      gBleMitmProtected = false;
      Serial.printf("  ✗ FAILED - Reason: %d - forcing disconnect\n", cmpl.fail_reason);
      if (gBleServer) {
        gBleServer->disconnect(gBleServer->getConnId());
      }
    }
  }

  bool onConfirmPIN(uint32_t pin) override {
    Serial.printf("→ BLE Confirm PIN: %06u (expected: %06u) ", pin, gBlePin);
    bool valid = (pin == gBlePin);
    if (valid) {
      Serial.println("✓");
      Serial.println("  PIN confirmed - authentication should succeed");
    } else {
      Serial.println("✗");
      Serial.println("  PIN mismatch - pairing will fail");
    }
    return valid;
  }
};

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    gBleDeviceConnected = true;
    Serial.println("BLE client connected.");
  }

  void onDisconnect(BLEServer *pServer) override {
    gBleDeviceConnected = false;
    Serial.println("BLE client disconnected.");
  }
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    // Data written to BLE characteristic will be handled in serviceBle()
  }
};

}  // namespace

namespace ble_bridge {

void setupBle(const BridgeConfig &config) {
  if (config.bluetoothMode != BluetoothMode::ON) {
    return;
  }

  gBleServiceUuid = config.BLEServiceUuid;
  gBleCharacteristicUuid = config.BLECharacteristicUuid;
  gBlePin = config.BLEPin.toInt();

  Serial.println("Starting BLE...");
  BLEDevice::init(config.deviceName.c_str());
  Serial.println("BLE initialized");

  // Set security callbacks for PIN handling
  BLEDevice::setSecurityCallbacks(new SecurityCallbacks());

  // Configure security using ESP-IDF API directly for full control
  // This avoids BLESecurity wrapper conflicts with setStaticPIN()
  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;  // 0x0D
  esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;  // Display Only
  uint8_t key_size = 16;
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
  uint32_t passkey = gBlePin;

  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(esp_ble_auth_req_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(esp_ble_io_cap_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));

  Serial.printf("Security configured: auth_req=0x%02X iocap=%d pin=%06u\n", auth_req, iocap, passkey);

  gBleServer = BLEDevice::createServer();
  gBleServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = gBleServer->createService(gBleServiceUuid.c_str());

  gBleCharacteristic = pService->createCharacteristic(
      gBleCharacteristicUuid.c_str(),
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);

  // Require encryption to trigger pairing on first access
  gBleCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

  // Add CCCD descriptor for notifications
  BLE2902 *pDescriptor = new BLE2902();
  pDescriptor->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  gBleCharacteristic->addDescriptor(pDescriptor);
  
  gBleCharacteristic->setCallbacks(new CharacteristicCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(gBleServiceUuid.c_str());
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("========================================");
  Serial.printf("BLE started. Device name: %s\n", config.deviceName.c_str());
  Serial.printf("Service UUID: %s\n", gBleServiceUuid.c_str());
  Serial.printf("Characteristic UUID: %s\n", gBleCharacteristicUuid.c_str());
  Serial.println("Security: SC + MITM + Bonding (ESP-IDF API)");
  Serial.println("----------------------------------------");
  Serial.printf("** BLE PIN: %06u **\n", gBlePin);
  Serial.println("========================================");
}

void shutdownBle() {
  if (gBleServer != nullptr) {
    BLEDevice::deinit(true);
    gBleServer = nullptr;
    gBleCharacteristic = nullptr;
    gBleDeviceConnected = false;
    gBleOldDeviceConnected = false;
    gBleMitmProtected = false;
    Serial.println("BLE shutdown.");
  }
}

void serviceBle(HardwareSerial &uart) {
  if (gBleServer == nullptr || gBleCharacteristic == nullptr) {
    return;
  }

  // Handle connection state changes
  if (gBleDeviceConnected && !gBleOldDeviceConnected) {
    gBleOldDeviceConnected = gBleDeviceConnected;
  }

  if (!gBleDeviceConnected && gBleOldDeviceConnected) {
    delay(500);  // Give the Bluetooth stack time to prepare
    gBleServer->startAdvertising();
    Serial.println("BLE advertising restarted.");
    gBleOldDeviceConnected = gBleDeviceConnected;
  }

  // Update status LED based on BLE connection
  bridge_runtime::updateBridgeStatusLed(gBleDeviceConnected);

  if (!gBleDeviceConnected) {
    return;
  }

  // Read data from BLE and send to UART
  std::string rxValue = gBleCharacteristic->getValue();
  if (rxValue.length() > 0) {
    uart.write((const uint8_t *)rxValue.data(), rxValue.length());
    gBleCharacteristic->setValue("");  // Clear the buffer after reading
  }

  // Read data from UART and send to BLE
  if (uart.available()) {
    uint8_t buf[512];
    size_t n = uart.readBytes(buf, sizeof(buf));
    if (n > 0) {
      gBleCharacteristic->setValue(buf, n);
      gBleCharacteristic->notify();
    }
  }
}

bool isBleConnected() { return gBleDeviceConnected; }

}  // namespace ble_bridge
