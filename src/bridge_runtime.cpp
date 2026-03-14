#include "bridge_runtime.h"

#include "config.h"

namespace bridge_runtime {

void setupBridge(WiFiServer &tcpServer, HardwareSerial &uart, const BridgeConfig &config) {
  uart.begin(config.baudRate, SERIAL_8N1, cfg::kBridgeRxPin, cfg::kBridgeTxPin);
  tcpServer.begin();
  tcpServer.setNoDelay(true);
  Serial.printf("TCP server started on port %u.\n", cfg::kTcpPort);
  Serial.printf("UART bridge on RX=%d TX=%d at %lu baud.\n", cfg::kBridgeRxPin, cfg::kBridgeTxPin,
                static_cast<unsigned long>(config.baudRate));
}

void serviceTcpBridge(WiFiServer &tcpServer, WiFiClient &tcpClient, HardwareSerial &uart) {
  if (!tcpClient || !tcpClient.connected()) {
    WiFiClient incoming = tcpServer.available();
    if (incoming) {
      if (tcpClient) {
        tcpClient.stop();
      }
      tcpClient = incoming;
      tcpClient.setNoDelay(true);
      Serial.printf("TCP client connected: %s\n", tcpClient.remoteIP().toString().c_str());
    }
  }

  if (!tcpClient || !tcpClient.connected()) {
    return;
  }

  uint8_t buf[256];

  while (tcpClient.available()) {
    const size_t n = tcpClient.read(buf, sizeof(buf));
    if (n > 0) {
      uart.write(buf, n);
    }
  }

  while (uart.available()) {
    const size_t n = uart.readBytes(buf, sizeof(buf));
    if (n > 0) {
      tcpClient.write(buf, n);
    }
  }

  if (!tcpClient.connected()) {
    tcpClient.stop();
    Serial.println("TCP client disconnected.");
  }
}

}  // namespace bridge_runtime
