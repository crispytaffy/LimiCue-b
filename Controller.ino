// Bluetooth接続マスター側（コントローラー）プログラム
#include <M5StickCPlus.h>
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;

// 変数宣言
String slave_name = "LumiCue-b";   // スレーブ側の接続名
String master_name = "Controller"; // マスターの接続名
// uint8_t address[6]  = {0x94, 0xB9, 0x7E, 0x8B, 0x4F, 0xAA};  // MACアドレスで接続の場合、スレーブ側のMACアドレスを設定
bool connected = 0;       // 接続状態格納用
int connect_count = 3;    // 再接続実行回数
String data = "";         // 受信データ格納用
int btn_pw = 0;           // 電源ボタン状態取得用

// 再起動（リスタート）処理
void restart() {
  // 電源ボタン状態取得（1秒以下のONで「2」1秒以上で「1」すぐに「0」に戻る）
  btn_pw = M5.Axp.GetBtnPress();
  if (btn_pw == 2) {  // 電源ボタン短押し（1秒以下）なら
    ESP.restart();    // 再起動
  }
}
// スレーブ側へのデータ送信後に接続を切って消費電力を抑える
void disconnect() {
  // 接続を切る（disconnect()は最大10秒かかる）
  if (SerialBT.disconnect()) {
    M5.Lcd.setTextColor(ORANGE, BLACK);
    M5.Lcd.println("Disconnect");
    Serial.println("Disconnect");
  }
}

// 初期設定 -----------------------------------------------
void setup() {
  M5.begin();             // 本体初期化
  Serial.begin(9600);     // シリアル通信初期化
  // 出力設定
  pinMode(10, OUTPUT);    //本体LED赤
  digitalWrite(10, HIGH); //本体LED初期値OFF（HIGH）
  // LCD表示設定
  M5.Lcd.setTextFont(2);

  // Bluetooth接続開始
  SerialBT.begin(master_name, true);  // マスターとして初期化。trueを書くとマスター、省略かfalseを指定でスレーブ
  M5.Lcd.print("BT Try!\n.");
  Serial.print("BT Try!\n.");
  M5.Lcd.println("Stand-by");
  // connect(address)は高速（最大10秒）、connect(slave_name)は低速（最大30秒）
  // slave_nameでの接続はMACアドレスを調べなくても良いが接続は遅い
  while (connected == 0) {    // connectedが0(未接続)なら接続実行を繰り返す
    if (connect_count != 0) { // 再接続実行回数カウントが0でなければ
      connected = SerialBT.connect(slave_name); // 接続実行（接続名で接続する場合）
      // connected = SerialBT.connect(address);   // 接続実行（MACアドレスで接続する場合）
      M5.Lcd.print(".");
      connect_count--;        // 再接続実行回数カウント -1
    } else {                  // 再接続実行回数カウントが0なら接続失敗
      M5.Lcd.setTextColor(RED, BLACK);
      M5.Lcd.print("\nFailed!");
      Serial.print("\nFailed!");
      while (1) {restart();}  // 無限ループ(再起動待機)
    }
  }
  // 接続確認
  M5.Lcd.setTextColor(WHITE, BLACK);
  if (connected) {                    // 接続成功なら
    M5.Lcd.setTextColor(CYAN, BLACK);
    M5.Lcd.println("\nConnected!");   // 「Connected!」表示
    Serial.println("\nConnected!");
  } else {                            // 接続失敗なら
    M5.Lcd.setTextColor(RED, BLACK);
    M5.Lcd.println("\nFailed!!");     // 「Failed!!」表示
    Serial.println("\nFailed!!");
    while (1) {restart();}            // 無限ループ(再起動待機)
  }
  delay(1000);                        // 接続結果確認画面表示時間
  // 電源ON時のシリアルデータが無くなるまで待つ
  while (Serial.available()) {data = Serial.read();}
  // LCD表示リセット
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println(master_name);        // 接続名表示
}
// メイン -------------------------------------------------
void loop() {
  M5.update();  // 本体ボタン状態更新
  // ボタン操作処理（スレーブ側へデータ送信）※送信のみでよければコメント部を有効にして省エネ
  if (M5.BtnA.wasPressed()) {     // ボタンAが押されていれば
    // SerialBT.connect();        // 再接続 connect(name)で名前解決されたアドレス、または、connect(address)のMACアドレスで再接続
    if (connected == 1) {
      SerialBT.print("LED ON!\r");  // 「A ON!」送信（「CR」区切り文字）
      // disconnect();            // 省エネのため接続を遮断
    }
  }
  if (M5.BtnB.wasPressed()) {     // ボタンBが押されていれば
    // SerialBT.connect();           // 再接続 connect(name)で名前解決されたアドレス、または、connect(address)のMACアドレスで再接続
    if (connected == 1) {
      SerialBT.print("Clear\r");  // 「B ON!」送信（「CR」区切り文字）
      // disconnect();               // 省エネのため接続を遮断
    }
  }
  // シリアル入力データ（「CR」付きで入力）をスレーブ側へ送信
  if (Serial.available()) {               // シリアルデータ受信で
    data = Serial.readStringUntil('\r');  // 「CR」の手前までデータ取得
    M5.Lcd.print("SR: ");                 // シリアルデータ液晶表示
    M5.Lcd.println(data);                 // 液晶表示は改行あり
    Serial.print(data);                   // シリアル出力は改行なし
    // Bluetoothデータ送信
    SerialBT.print(data + "\r"); // 区切り文字「CR」をつけてスレーブ側へ送信
  }
  // LED点灯処理
  if (data == "A ON!") {      // 受信データが「A ON!」なら
    digitalWrite(10, LOW);    // 本体LED点灯
  }
  if (data == "B ON!") {      // 受信データが「B ON!」なら
    digitalWrite(10, HIGH);   // 本体LED消灯
  }
  restart();  // 再起動（リスタート）処理
  delay(20);  // 20ms遅延時間
}
