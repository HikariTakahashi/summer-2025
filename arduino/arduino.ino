// =================================================================
// ライブラリのインクルード
// =================================================================
#include <WiFiS3.h>
#include <WebSocketsClient.h>
#include <DHT.h>
#include "arduino_secrets.h"

// =================================================================
// ピン設定
// =================================================================
// --- モーター & 超音波センサー ---
const int enA = 9;
const int in1 = 8;
const int in2 = 7;
const int enB = 3;
const int in3 = 6;
const int in4 = 5;
const int trigPin = 12;
const int echoPin = 11;
// --- DHT11 センサー ---
const int dhtPin = 2; // DHT11のピンをD2に統一
// --- リレー ---
const int relayPin = 13;

// =================================================================
// オブジェクトの初期化
// =================================================================
// --- DHTセンサー ---
#define DHTTYPE DHT11
DHT dht(dhtPin, DHTTYPE);
// --- Wi-Fi & WebSocket ---
const char* host = SECRET_SERVER_HOST;
const int port = SECRET_SERVER_PORT;
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
WebSocketsClient webSocket;

// =================================================================
// 制御用定数 & グローバル変数
// =================================================================
// --- モーター制御 ---
const int OBSTACLE_DISTANCE = 15;
const int ACCEL_INTERVAL = 20;
const int ACCEL_FACTOR = 10;
const int TURN_SPEED = 150;
const int TURN_DURATION = 500;
int currentSpeed = 0;
unsigned long lastUpdateTime = 0;
// --- タイマー管理 ---
unsigned long lastSensorReadTime = 0; // センサー読み取り & リレー制御用
unsigned long lastSendTime = 0;       // WebSocket送信用
// --- 共有データ ---
float lastReadHumidity = 0.0; // 最後に読み取った湿度を保持する変数

// =================================================================
// WebSocket イベント処理
// =================================================================
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      break;
    case WStype_CONNECTED:
      Serial.print("[WSc] Connected to url: ");
      Serial.println((char *)payload);
      break;
    case WStype_TEXT:
      Serial.print("[WSc] get text: ");
      Serial.println((char *)payload);
      break;
    default:
      break;
  }
}

// =================================================================
// セットアップ関数
// =================================================================
void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }

  // --- ハードウェア初期化 ---
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // リレーを初期状態でOFF
  dht.begin();
  Serial.println("Hardware initialized.");

  // --- Wi-Fi接続 ---
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

  // --- WebSocket接続 ---
  webSocket.begin(host, port, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

// =================================================================
// メインループ
// =================================================================
void loop() {
  // WebSocketの接続維持
  webSocket.loop();

  // --- 2秒ごとにセンサー読み取り & リレー制御 ---
  if (millis() - lastSensorReadTime > 2000) {
    lastSensorReadTime = millis();
    float h = dht.readHumidity();

    if (!isnan(h)) {
      lastReadHumidity = h; // 読み取った値をグローバル変数に保存
      Serial.print("湿度: ");
      Serial.print(h);
      Serial.print(" %");

      if (h < 50.0) {
        digitalWrite(relayPin, LOW);
        Serial.println(" -> リレーON");
      } else {
        digitalWrite(relayPin, HIGH);
        Serial.println(" -> リレーOFF");
      }
    } else {
      Serial.println("センサーの読み取りに失敗しました");
    }
  }

  // --- 5秒ごとにWebSocketでデータ送信 ---
  if (webSocket.isConnected() && (millis() - lastSendTime > 5000)) {
    lastSendTime = millis();
    
    // JSON形式の文字列を作成
    String message = "{\"humidity\":" + String(lastReadHumidity, 1) + "}";
    
    Serial.print("Sending message: ");
    Serial.println(message);
    webSocket.sendTXT(message);
  }

  // --- モーター制御 ---
  long distance = measureDistance();
  if (distance > 0 && distance < OBSTACLE_DISTANCE) {
    pivotTurn();
    stopMotors();
    currentSpeed = 0;
  } else {
    accelerateMotors();
  }
}

// =================================================================
// ヘルパー関数 (モーター制御など)
// =================================================================

void accelerateMotors() {
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= ACCEL_INTERVAL) {
    lastUpdateTime = currentTime;
    if (currentSpeed < 255) {
      int speedIncrease = max(1, (255 - currentSpeed) / ACCEL_FACTOR);
      currentSpeed += speedIncrease;
      if (currentSpeed > 255) {
        currentSpeed = 255;
      }
    }
  }
  setMotorSpeed(currentSpeed, true);
}

void stopMotors() {
  setMotorSpeed(0, true);
}

void pivotTurn() {
  setMotorA(TURN_SPEED, true);
  setMotorB(TURN_SPEED, false);
  delay(TURN_DURATION);
}

void setMotorSpeed(int speed, bool forward) {
  setMotorA(speed, forward);
  setMotorB(speed, forward);
}

long measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000L);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

void setMotorA(int speed, bool forward) {
  if (forward) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  }
  analogWrite(enA, speed);
}

void setMotorB(int speed, bool forward) {
  if (forward) {
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
  } else {
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
  }
  analogWrite(enB, speed);
}