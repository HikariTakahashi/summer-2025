const WebSocket = require("ws");
const http = require("http");
const fs = require("fs");
const path = require("path");

// HTTPサーバーを作成
const server = http.createServer((req, res) => {
  if (req.url === "/" || req.url === "/index.html") {
    // Arduinoデータ表示ページを提供
    fs.readFile(path.join(__dirname, "arduino_display.html"), (err, data) => {
      if (err) {
        res.writeHead(404);
        res.end("Arduino display file not found");
        return;
      }
      res.writeHead(200, { "Content-Type": "text/html" });
      res.end(data);
    });
  } else if (req.url === "/radar") {
    // 元のレーダー表示ページ
    fs.readFile(path.join(__dirname, "index.html"), (err, data) => {
      if (err) {
        res.writeHead(404);
        res.end("Radar display file not found");
        return;
      }
      res.writeHead(200, { "Content-Type": "text/html" });
      res.end(data);
    });
  } else {
    res.writeHead(404);
    res.end("Page not found");
  }
});

// WebSocketサーバーをHTTPサーバーに統合
const wss = new WebSocket.Server({ server });

wss.on("connection", (ws) => {
  console.log("Client connected");

  // 接続タイプを識別
  ws.clientType = "unknown";

  ws.on("message", (message) => {
    try {
      const messageStr = message.toString();
      console.log(`Received: ${messageStr}`);

      // Arduino識別（シンプルな数値や特定のフォーマット）
      if (ws.clientType === "unknown") {
        // JSONかどうかチェック
        try {
          JSON.parse(messageStr);
          ws.clientType = "json_client";
        } catch {
          // 数値のみの場合はArduinoと判定
          if (/^\d+(\.\d+)?$/.test(messageStr.trim())) {
            ws.clientType = "arduino";
          } else {
            ws.clientType = "text_client";
          }
        }
        console.log(`Client type identified: ${ws.clientType}`);
      }

      // メッセージを処理してブロードキャスト
      let processedMessage;
      if (ws.clientType === "arduino") {
        // Arduinoからの数値データを構造化
        const value = parseFloat(messageStr.trim());
        processedMessage = JSON.stringify({
          value: value,
          timestamp: new Date().toISOString(),
          source: "arduino",
        });
      } else {
        // その他のクライアントはそのまま
        processedMessage = messageStr;
      }

      // 全てのクライアントにメッセージをブロードキャスト
      wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN && client !== ws) {
          client.send(processedMessage);
        }
      });
    } catch (error) {
      console.error("Message processing error:", error);
    }
  });

  ws.on("close", () => {
    console.log(`Client disconnected (type: ${ws.clientType})`);
  });

  ws.on("error", (error) => {
    console.error("WebSocket error:", error);
  });
});

// HTTPサーバーを起動
server.listen(8080, () => {
  console.log("HTTP server is running on http://localhost:8080");
  console.log("WebSocket server is running on ws://localhost:8080");
});
