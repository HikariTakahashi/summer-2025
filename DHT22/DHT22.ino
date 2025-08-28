#include <WiFiS3.h> // WiFi機能を使うための必須ライブラリ
#include "DHT.h"    // DHTセンサー用のライブラリ

// ----- Wi-Fi設定 -----
// Wi-Fi環境に合わせて書き換える
const char ssid[] = "";
const char pass[] = "";

#define SECRET_SERVER_HOST ""
#define SECRET_SERVER_PORT 8080
// -------------------------------------------------------------

int status = WL_IDLE_STATUS; // Wi-Fiの状態を保存する変数
WiFiServer server(80); // Webサーバー機能を使うための準備 (80番ポートを使用)

// ----- DHTセンサー設定 -----
#define DHTPIN 2      // センサーはデジタル2番ピンに接続
#define DHTTYPE DHT22 // センサーの種類はDHT22
DHT dht(DHTPIN, DHTTYPE); // DHTセンサーを操作するための準備

// ----- HTMLページの設計図 -----
// PROGMEM に保存することで、ArduinoのRAM(作業用メモリ)を節約
const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <title>Arduino 温湿度モニター (API)</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    /* ここにCSSで見栄えを整えるコード */
    html { font-family: sans-serif; text-align: center; }
    body { background-color: #f0f8ff; }
    h1 { font-size: 2.5rem; color: #005A9C; }
    .sensor-value { font-size: 4rem; color: #333; font-weight: bold; margin: 20px; }
  </style>
</head>
<body>
  <h1>Arduino 温湿度モニター</h1>
  <p>温度</p>
  <div class="sensor-value"><span id="temperature">--</span> &deg;C</div>
  <p>湿度</p>
  <div class="sensor-value"><span id="humidity">--</span> %</div>

  <script>
    // 10秒ごと(10000ミリ秒)に、getData関数を自動で実行する
    setInterval(getData, 10000); 
    // ページが読み込まれた直後にも、一度実行する
    window.onload = getData;

    function getData() {
      // Arduinoの「/data」という住所にデータを問い合わせる
      fetch('/data')
        .then(response => response.json()) // 受け取った返事をJSON形式として解釈
        .then(data => { // 解釈したデータを使って処理する
          // HTML内のIDを手がかりに、表示されている数値を書き換える
          document.getElementById('temperature').innerText = data.temperature.toFixed(1);
          document.getElementById('humidity').innerText = data.humidity.toFixed(1);
        })
        .catch(error => { // もしエラーが起きたら
          console.error('エラー:', error);
        });
    }
  </script>
</body>
</html>
)=====";

// setup関数：電源が入ったときに一度だけ実行される初期設定
void setup() {
  Serial.begin(9600); // PCとの通信を開始 (デバッグ用)
  dht.begin();        // センサーの準備を開始
  
  // Wi-Fiに接続できるまで繰り返す
  while (status != WL_CONNECTED) {
    Serial.print("Wi-Fiに接続中: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass); // 接続を試みる
    delay(5000); // 5秒待つ
  }

  server.begin(); // Webサーバーを開始
  Serial.print("サーバーを開始しました。 http://");
  Serial.println(WiFi.localIP()); // 自分のIPアドレスをPCに表示
}

// loop関数：setupが終わった後、無限に繰り返されるメイン処理
void loop() {
  WiFiClient client = server.available(); // ブラウザからのアクセス(クライアント)を待つ

  if (client) { // もしアクセスがあったら
    String firstLine = client.readStringUntil('\n'); // アクセスしてきた内容の1行目だけを読む
    
    // 1行目の内容に応じて、処理を分ける
    if (firstLine.indexOf("GET /data") >= 0) {
      sendJSON(client); // もし「/data」へのアクセスなら、JSONを返す
    } else {
      sendHTML(client); // それ以外へのアクセスなら、HTMLページを返す
    }
    
    delay(10); // 少し待つ
    client.stop(); // 通信を終了する
  }
}

// sendHTML関数：HTMLページをブラウザに送信する処理
void sendHTML(WiFiClient client) {
  client.println("HTTP/1.1 200 OK"); // 「成功しました」という合図
  client.println("Content-Type: text/html"); // 「これはHTMLですよ」という説明
  client.println("Connection: close"); // 「送り終わったら通信を切ります」という宣言
  client.println(); // ヘッダーの終わり (空行)
  client.println(INDEX_HTML); // 保存しておいたHTML本体を送る
}

// sendJSON関数：センサーデータをJSON形式でブラウザに送信する処理
void sendJSON(WiFiClient client) {
  float t = dht.readTemperature(); // 温度を測定
  float h = dht.readHumidity();   // 湿度を測定

  // もし測定に失敗したら "null"、成功したら数値を文字列にする
  String temp_str = isnan(t) ? "null" : String(t, 1);
  String hum_str = isnan(h) ? "null" : String(h, 1);
  
  // 「{"temperature":25.5,"humidity":60.2}」のような文字列を組み立てる
  String json = "{";
  json += "\"temperature\":" + temp_str + ",";
  json += "\"humidity\":" + hum_str;
  json += "}";
  
  client.println("HTTP/1.1 200 OK"); // 「成功しました」という合図
  client.println("Content-Type: application/json"); // 「これはJSONデータですよ」という説明
  client.println("Connection: close"); // 「送り終わったら通信を切ります」という宣言
  client.println(); // ヘッダーの終わり (空行)
  client.println(json); // 組み立てたJSONデータを送る
}
