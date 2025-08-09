#include <WebSocketsClient.h>
#include <WiFiS3.h>
#include "arduino_secrets.h"

const char* host = SECRET_SERVER_HOST;
const int port = SECRET_SERVER_PORT;

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

WebSocketsClient webSocket;
int counter = 0;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.print("[WSc] Connected to url: ");
      Serial.println((char *)payload);
      webSocket.sendTXT("Connected from Arduino!");
      break;
    case WStype_TEXT:
      Serial.print("[WSc] get text: ");
      Serial.println((char *)payload);
      break;
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }

  Serial.print("Connecting to ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  webSocket.begin(host, port, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();

  static unsigned long lastSendTime = 0;
  if (webSocket.isConnected() && (millis() - lastSendTime > 5000)) {
    lastSendTime = millis();
    
    String message = String(counter++);
    Serial.print("Sending message: ");
    Serial.println(message);
    
    webSocket.sendTXT(message);
  }
}