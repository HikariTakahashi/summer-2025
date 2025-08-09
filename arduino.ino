#include <Arduino.h>
#include <WiFiS3.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// WiFi settings - 実際の値に変更してください
#define WIFI_SSID "YOUR_WIFI_NETWORK_NAME"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// WebSocket settings　
// コマンドプロンプトで「ipconfig」と入力して出てきたアドレスを入力
// ※わからなければAIかHikariに聞いてください
#define WEBSOCKET_SERVER "000.00.00.00"
#define WEBSOCKET_PORT 8080

WebSocketsClient webSocket;

// センサー値のシミュレーション
float temperature = 25.0;
float humidity = 60.0;
int sensorValue = 512;

// タイマー管理
unsigned long lastSensorRead = 0;
unsigned long lastDataSend = 0;
const unsigned long SENSOR_INTERVAL = 500;  // 500ms
const unsigned long SEND_INTERVAL = 1000;   // 1秒

bool wifiConnected = false;
bool wsConnected = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  
  Serial.println("=================================");
  Serial.println("Arduino WebSocket Client Starting");
  Serial.println("=================================");
  
  // WiFi初期化チェック
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("❌ WiFiモジュールが見つかりません！");
    while (true) {
      delay(1000);
    }
  }
  
  // WiFi接続
  connectToWiFi();
  
  // WebSocket初期化
  initializeWebSocket();
  
  Serial.println("✅ セットアップ完了");
  Serial.println("=================================");
}

void connectToWiFi() {
  Serial.println("📶 WiFi接続開始...");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
    
    if (attempts % 10 == 0) {
      Serial.println();
      Serial.print("接続試行中... (");
      Serial.print(attempts);
      Serial.println("/30)");
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println();
    Serial.println("✅ WiFi接続成功！");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());
  } else {
    Serial.println();
    Serial.println("❌ WiFi接続失敗");
    while (true) {
      delay(1000);
    }
  }
}

void initializeWebSocket() {
  Serial.println("🔌 WebSocket初期化...");
  Serial.print("サーバー: ");
  Serial.print(WEBSOCKET_SERVER);
  Serial.print(":");
  Serial.println(WEBSOCKET_PORT);
  
  // WebSocketイベントハンドラーを設定
  webSocket.onEvent(webSocketEvent);
  
  // WebSocketサーバーに接続
  webSocket.begin(WEBSOCKET_SERVER, WEBSOCKET_PORT, "/");
  
  // 再接続設定
  webSocket.setReconnectInterval(5000);
  
  // 追加設定
  webSocket.enableHeartbeat(15000, 3000, 2);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      wsConnected = false;
      Serial.println("❌ [WebSocket] 切断されました");
      break;
      
    case WStype_CONNECTED:
      wsConnected = true;
      Serial.print("✅ [WebSocket] 接続成功: ");
      Serial.println((char*)payload);
      
      // 接続確認メッセージを送信
      sendConnectionMessage();
      break;
      
    case WStype_TEXT:
      Serial.print("📥 [WebSocket] メッセージ受信: ");
      Serial.println((char*)payload);
      break;
      
    case WStype_ERROR:
      Serial.print("⚠️ [WebSocket] エラー: ");
      Serial.println((char*)payload);
      break;
      
    case WStype_PING:
      Serial.println("🏓 [WebSocket] PING受信");
      break;
      
    case WStype_PONG:
      Serial.println("🏓 [WebSocket] PONG受信");
      break;
      
    default:
      Serial.print("❓ [WebSocket] 不明なイベント: ");
      Serial.println(type);
      break;
  }
}

void sendConnectionMessage() {
  if (!wsConnected) return;
  
  // 接続確認用のJSONメッセージ
  StaticJsonDocument<200> doc;
  doc["status"] = "connected";
  doc["device"] = "Arduino";
  doc["timestamp"] = millis();
  doc["ip"] = WiFi.localIP().toString();
  
  String message;
  serializeJson(doc, message);
  
  webSocket.sendTXT(message);
  Serial.print("📤 [送信] 接続確認: ");
  Serial.println(message);
}

void readSensors() {
  // センサー値をシミュレート（実際のセンサーに置き換えてください）
  
  // 温度センサー（ランダムな変動）
  temperature += (random(-10, 11) / 10.0);
  if (temperature < 15.0) temperature = 15.0;
  if (temperature > 35.0) temperature = 35.0;
  
  // 湿度センサー（ランダムな変動）
  humidity += (random(-5, 6) / 10.0);
  if (humidity < 30.0) humidity = 30.0;
  if (humidity > 90.0) humidity = 90.0;
  
  // アナログセンサー値
  sensorValue = analogRead(A0);
}

void sendSensorData() {
  if (!wsConnected) {
    Serial.println("⚠️ WebSocket未接続 - データ送信スキップ");
    return;
  }
  
  // JSONデータを作成
  StaticJsonDocument<300> doc;
  doc["temperature"] = round(temperature * 100) / 100.0;  // 小数点2桁
  doc["humidity"] = round(humidity * 10) / 10.0;          // 小数点1桁
  doc["sensorValue"] = sensorValue;
  doc["timestamp"] = millis();
  doc["uptime"] = millis() / 1000;
  doc["freeMemory"] = freeMemory();
  doc["wifiRSSI"] = WiFi.RSSI();
  
  String message;
  serializeJson(doc, message);
  
  // WebSocketでデータ送信
  webSocket.sendTXT(message);
  
  // シリアルモニターに表示
  Serial.print("📤 [送信] ");
  Serial.print("温度:");
  Serial.print(temperature);
  Serial.print("°C, 湿度:");
  Serial.print(humidity);
  Serial.print("%, センサー:");
  Serial.print(sensorValue);
  Serial.print(", RSSI:");
  Serial.print(WiFi.RSSI());
  Serial.println("dBm");
}

int freeMemory() {
  // 簡易的なメモリ使用量計算
  return 2048 - 1024; // ダミー値
}

void checkConnections() {
  // WiFi接続チェック
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      wifiConnected = false;
      wsConnected = false;
      Serial.println("❌ WiFi接続が失われました");
    }
    
    // WiFi再接続試行
    Serial.println("🔄 WiFi再接続中...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    delay(5000);
    
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      Serial.println("✅ WiFi再接続成功");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    }
  }
}

void loop() {
  // 接続チェック（10秒ごと）
  static unsigned long lastConnectionCheck = 0;
  if (millis() - lastConnectionCheck > 10000) {
    checkConnections();
    lastConnectionCheck = millis();
  }
  
  // WebSocket処理（必須）
  webSocket.loop();
  
  // センサー読み取り
  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
    readSensors();
    lastSensorRead = millis();
  }
  
  // データ送信
  if (millis() - lastDataSend >= SEND_INTERVAL) {
    sendSensorData();
    lastDataSend = millis();
  }
  
  // CPU負荷軽減
  delay(50);
}