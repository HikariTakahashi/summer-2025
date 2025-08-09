const WebSocket = require("ws");
const socket = new WebSocket("ws://localhost:8080");

socket.on("open", function () {
  let angle = 0;
  setInterval(() => {
    // ランダムな距離データを生成
    const randomDistance = Math.floor(Math.random() * 21) + 80;

    const data = {
      distance: randomDistance,
      arg: angle, // 角度を1度ずつ増加させる
    };

    const message = JSON.stringify(data);
    socket.send(message);
    console.log(`Sent: ${message}`);

    angle += 1; // 角度を1度ずつ増加
    if (angle >= 360) angle = 0; // 360度に達したら0度に戻す
  }, 60); // 100ミリ秒ごとにメッセージを送信（角度を1度ずつ増加）
});
