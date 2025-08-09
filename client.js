const WebSocket = require("ws");

class ArduinoDataSimulator {
  constructor() {
    this.socket = null;
    this.isConnected = false;
    this.simulationInterval = null;

    // センサー値の初期値
    this.temperature = 25.0;
    this.humidity = 60.0;
    this.sensorValue = 512;
    this.uptime = 0;

    // データ送信間隔
    this.sendInterval = 1000; // 1秒

    this.connect();
  }

  connect() {
    console.log("🤖 Arduino WebSocketクライアント起動中...");
    console.log("📡 WebSocketサーバーに接続中...");

    this.socket = new WebSocket("ws://localhost:8080");

    this.socket.on("open", () => {
      console.log("✅ WebSocketサーバーに接続しました");
      this.isConnected = true;

      // 接続確認メッセージを送信
      this.sendConnectionMessage();

      // データ送信開始
      this.startSimulation();
    });

    this.socket.on("close", () => {
      console.log("❌ WebSocket接続が切断されました");
      this.isConnected = false;
      this.stopSimulation();

      // 5秒後に自動再接続
      setTimeout(() => {
        console.log("🔄 再接続を試みます...");
        this.connect();
      }, 5000);
    });

    this.socket.on("error", (error) => {
      console.error("⚠️ WebSocketエラー:", error.message);
    });

    this.socket.on("message", (data) => {
      console.log("📥 サーバーからメッセージ:", data.toString());
    });
  }

  sendConnectionMessage() {
    if (!this.isConnected) return;

    const connectionData = {
      status: "connected",
      device: "Arduino",
      timestamp: Date.now(),
      ip: "192.168.1.100",
    };

    const message = JSON.stringify(connectionData);
    this.socket.send(message);
    console.log("📤 [接続確認] 送信:", message);
  }

  startSimulation() {
    console.log("🔄 Arduinoデータシミュレーション開始");
    console.log("📊 以下のデータを1秒間隔で送信します:");
    console.log("   - 温度センサー (15-35°C)");
    console.log("   - 湿度センサー (30-90%)");
    console.log("   - アナログセンサー (0-1023)");
    console.log("   - システム情報");
    console.log("");

    this.simulationInterval = setInterval(() => {
      if (this.isConnected) {
        this.updateSensorValues();
        this.sendSensorData();
      }
    }, this.sendInterval);
  }

  stopSimulation() {
    if (this.simulationInterval) {
      clearInterval(this.simulationInterval);
      this.simulationInterval = null;
      console.log("⏹️ シミュレーション停止");
    }
  }

  updateSensorValues() {
    // 温度センサー（リアルな変動をシミュレート）
    this.temperature += (Math.random() - 0.5) * 2; // ±1度の変動
    if (this.temperature < 15.0) this.temperature = 15.0;
    if (this.temperature > 35.0) this.temperature = 35.0;

    // 湿度センサー（1秒に1度ずつ増加）
    this.humidity += 1;
    if (this.humidity > 90.0) this.humidity = 30.0; // 90%に達したら30%にリセット

    // アナログセンサー（0-1023の範囲）
    this.sensorValue += Math.floor((Math.random() - 0.5) * 100);
    if (this.sensorValue < 0) this.sensorValue = 0;
    if (this.sensorValue > 1023) this.sensorValue = 1023;

    // アップタイム（秒）
    this.uptime += 1;
  }

  sendSensorData() {
    // Arduinoから送信されるのと同じフォーマットのJSONデータ
    const sensorData = {
      temperature: Math.round(this.temperature * 100) / 100, // 小数点2桁
      humidity: Math.round(this.humidity * 10) / 10, // 小数点1桁
      sensorValue: this.sensorValue,
      timestamp: Date.now(),
      uptime: this.uptime,
      freeMemory: Math.floor(Math.random() * 512) + 1536, // 1536-2048の範囲
      wifiRSSI: Math.floor(Math.random() * 30) - 70, // -70 to -40 dBm
    };

    try {
      const message = JSON.stringify(sensorData);
      this.socket.send(message);

      // コンソールに送信データを表示
      console.log(
        `📤 [${new Date().toLocaleTimeString()}] 温度:${
          sensorData.temperature
        }°C, 湿度:${sensorData.humidity}%, センサー:${
          sensorData.sensorValue
        }, RSSI:${sensorData.wifiRSSI}dBm`
      );
    } catch (error) {
      console.error("❌ データ送信エラー:", error);
    }
  }

  // 特定のセンサーデータタイプを送信するメソッド
  sendTemperatureAlert() {
    if (!this.isConnected) return;

    const alertData = {
      temperature: 40.5, // 高温アラート
      humidity: this.humidity,
      sensorValue: this.sensorValue,
      timestamp: Date.now(),
      uptime: this.uptime,
      alert: "HIGH_TEMPERATURE",
      freeMemory: 1800,
      wifiRSSI: -55,
    };

    this.socket.send(JSON.stringify(alertData));
    console.log("🚨 [アラート] 高温警告送信:", alertData.temperature + "°C");
  }

  sendLowBatteryAlert() {
    if (!this.isConnected) return;

    const batteryData = {
      temperature: this.temperature,
      humidity: this.humidity,
      sensorValue: 150, // 低電圧を示すアナログ値
      voltage: 3.2, // バッテリー電圧
      timestamp: Date.now(),
      uptime: this.uptime,
      alert: "LOW_BATTERY",
      freeMemory: 1600,
      wifiRSSI: -65,
    };

    this.socket.send(JSON.stringify(batteryData));
    console.log(
      "🔋 [アラート] 低バッテリー警告送信: " + batteryData.voltage + "V"
    );
  }

  // 接続を手動で切断
  disconnect() {
    this.stopSimulation();
    if (this.socket) {
      this.socket.close();
    }
    console.log("🛑 クライアント切断");
  }

  // データ送信頻度を変更
  setDataInterval(intervalMs) {
    this.sendInterval = intervalMs;
    if (this.simulationInterval) {
      this.stopSimulation();
      this.startSimulation();
    }
    console.log(`⏱️ データ送信間隔を${intervalMs}msに変更`);
  }
}

// シミュレーター開始
console.log("=================================");
console.log("Arduino WebSocketクライアント");
console.log("=================================");
console.log("このクライアントは以下をシミュレートします:");
console.log("• 温度センサー (DHT22)");
console.log("• 湿度センサー (DHT22)");
console.log("• アナログセンサー (A0)");
console.log("• WiFi信号強度 (RSSI)");
console.log("• メモリ使用量");
console.log("• システムアップタイム");
console.log("");

const simulator = new ArduinoDataSimulator();

// キーボード入力処理（デバッグ用）
process.stdin.setRawMode(true);
process.stdin.resume();
process.stdin.setEncoding("utf8");

console.log("📝 キーボードコマンド:");
console.log("  [t] - 高温アラート送信");
console.log("  [b] - 低バッテリーアラート送信");
console.log("  [f] - 高速送信モード (500ms間隔)");
console.log("  [s] - 標準送信モード (1000ms間隔)");
console.log("  [q] - 終了");
console.log("");

process.stdin.on("data", (key) => {
  switch (key) {
    case "t":
      simulator.sendTemperatureAlert();
      break;
    case "b":
      simulator.sendLowBatteryAlert();
      break;
    case "f":
      simulator.setDataInterval(500);
      break;
    case "s":
      simulator.setDataInterval(1000);
      break;
    case "q":
    case "\u0003": // Ctrl+C
      console.log("\n🛑 シミュレーター終了中...");
      simulator.disconnect();
      process.exit(0);
      break;
  }
});

// Ctrl+Cで終了
process.on("SIGINT", () => {
  console.log("\n🛑 シミュレーター終了中...");
  simulator.disconnect();
  process.exit(0);
});

// エラーハンドリング
process.on("uncaughtException", (error) => {
  console.error("❌ 予期しないエラー:", error);
  simulator.disconnect();
  process.exit(1);
});
