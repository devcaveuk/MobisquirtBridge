#include "ble_bridge.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLESecurity.h>

#include "bridge_runtime.h"
#include "config.h"

namespace {

BLEServer *gBleServer = nullptr;
BLECharacteristic *gBleCharacteristic = nullptr;
bool gBleDeviceConnected = false;
bool gBleOldDeviceConnected = false;
String gBleServiceUuid;
String gBleCharacteristicUuid;
uint32_t gBlePin = 0;

class SecurityCallbacks : public BLESecurityCallbacks {
  uint32_t onPassKeyRequest() override {
    Serial.printf("BLE PassKey Request - Returning PIN: %06u\n", gBlePin);
    Serial.println("iOS should prompt for PIN entry now...");
    return gBlePin;
  }

  void onPassKeyNotify(uint32_t pass_key) override {
    Serial.printf("BLE PassKey Notify: %06u\n", pass_key);
  }

  bool onSecurityRequest() override {
    Serial.println("BLE Security Request - Accepting pairing request");
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) override {
    if (cmpl.success) {
      Serial.println("✓ BLE Pairing Success - Device is now bonded");
    } else {
      Serial.printf("✗ BLE Pairing Failed - Reason: %d\n", cmpl.fail_reason);
      Serial.println("  Common reasons:");
      Serial.println("  - Wrong PIN entered");
      Serial.println("  - Timeout");
      Serial.println("  - User cancelled");
    }
  }

  bool onConfirmPIN(uint32_t pin) override {
    Serial.printf("BLE Confirm PIN: %06u (expected: %06u)\n", pin, gBlePin);
    bool valid = (pin == gBlePin);
    if (valid) {
      Serial.println("✓ PIN matches");
    } else {
      Serial.println("✗ PIN mismatch - pairing will fail");
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

  // Configure BLE security - ESP32 displays PIN, iOS user enters it
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
  BLEDevice::setSecurityCallbacks(new SecurityCallbacks());

  BLESecurity *pSecurity = new BLESecurity();
  // Require Secure Connections, MITM protection, and Bonding - no Just Works
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
  // Display Only - ESP32 "displays" PIN, iOS user must enter it
  pSecurity->setCapability(ESP_IO_CAP_OUT);
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  pSecurity->setKeySize(16);
  pSecurity->setStaticPIN(gBlePin);

  gBleServer = BLEDevice::createServer();
  gBleServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = gBleServer->createService(gBleServiceUuid.c_str());

  gBleCharacteristic = pService->createCharacteristic(
      gBleCharacteristicUuid.c_str(),
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);

  // Set security requirements for the characteristic to trigger pairing
  gBleCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

  // Add CCCD descriptor for notifications and secure it
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
  Serial.println("----------------------------------------");
  Serial.printf("** BLE PIN: %06u **\n", gBlePin);
  Serial.println("----------------------------------------");
  Serial.println("When connecting from iOS:");
  Serial.println("1. Scan and connect to the device");
  Serial.println("2. iOS will prompt for PIN entry");
  Serial.printf("3. Enter: %06u\n", gBlePin);
  Serial.println("========================================");
}

void shutdownBle() {
  if (gBleServer != nullptr) {
    BLEDevice::deinit(true);
    gBleServer = nullptr;
    gBleCharacteristic = nullptr;
    gBleDeviceConnected = false;
    gBleOldDeviceConnected = false;
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
