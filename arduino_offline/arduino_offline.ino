#include "DHT.h" // DHTセンサーライブラリをインクルード

// --- ピン設定 ---
const int enA = 9;
const int in1 = 8;
const int in2 = 7;
const int enB = 3;
const int in3 = 6;
const int in4 = 5;
const int trigPin = 12;
const int echoPin = 11;
const int dhtPin = 2;       // DHT11センサーを接続したピン
const int relayPin = 13;    // リレーを接続したピン

// --- DHTセンサーのセットアップ ---
#define DHTTYPE DHT11   // 使用するセンサーのタイプをDHT11に指定
DHT dht(dhtPin, DHTTYPE); // DHTオブジェクトを作成

// --- 制御用定数 ---
const int OBSTACLE_DISTANCE = 15;
const int ACCEL_INTERVAL = 20;
const int ACCEL_FACTOR = 10;
const int TURN_SPEED = 150;
const int TURN_DURATION = 500;

// --- 状態を管理する変数 ---
int currentSpeed = 0;
unsigned long lastUpdateTime = 0;
unsigned long lastSensorReadTime = 0;

void setup() {
  // ピンモード設定
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(relayPin, OUTPUT);      // リレーピンを出力に設定
  digitalWrite(relayPin, HIGH);   // リレーを初期状態でOFFにしておく
  
  Serial.begin(9600);
  dht.begin(); // DHTセンサーを開始
}

void loop() {
  // 2秒ごとに湿度センサーの値を読み取って表示
  if (millis() - lastSensorReadTime > 2000) {
    lastSensorReadTime = millis();

    float h = dht.readHumidity();

    if (!isnan(h)) {
      Serial.print("湿度: ");
      Serial.print(h);
      Serial.println(" %");

      // 湿度の値に応じてリレーを制御
      if (h < 50.0) {
        // 湿度が50%未満ならリレーをONにする
        digitalWrite(relayPin, LOW);
        Serial.println(" -> リレーON");
      } else {
        // 湿度が50%以上ならリレーをOFFにする
        digitalWrite(relayPin, HIGH);
        Serial.println(" -> リレーOFF");
      }
    } else {
      Serial.println("センサーの読み取りに失敗しました");
    }
  }

  // --- 以下、既存のモーター制御コード (変更なし) ---
  
  long distance = measureDistance();

  if (distance > 0 && distance < OBSTACLE_DISTANCE) {
    pivotTurn();
    stopMotors();
    currentSpeed = 0;
  } else {
    accelerateMotors();
  }
}

// --- モーターを滑らかに加速させる関数 ---
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

// --- モーターを緊急停止させる関数 ---
void stopMotors() {
  setMotorSpeed(0, true);
}

// --- その場で旋回（ピボットターン）する関数 ---
void pivotTurn() {
  setMotorA(TURN_SPEED, true);
  setMotorB(TURN_SPEED, false);
  delay(TURN_DURATION);
}

// --- 左右のモーターに速度と方向を一度に設定する関数 ---
void setMotorSpeed(int speed, bool forward) {
  setMotorA(speed, forward);
  setMotorB(speed, forward);
}

// --- ヘルパー関数 (変更なし) ---

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