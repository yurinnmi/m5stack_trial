# M5Stack Core ESP32 + Unit CO2L(SCD41) CO2 Monitor

対象環境:

- VS Code + PlatformIO
- Board: `m5stack-core-esp32`
- Framework: Arduino
- M5Stack ライブラリ: `M5Unified`
- CO2 センサ: M5Stack Unit CO2L / SCD41, Grove PORT.A 接続

## 機能

- SCD41 から温度、湿度、CO2 値を単発測定します。
- Display は黒背景、白文字です。
- 2 分ごとにセンサ値を更新します。
- Button A: 手動測定して表示更新します。
- Button B: アラート音 ON/OFF をトグルします。初期値は OFF です。
- アラート ON かつ以下の閾値以上なら、内蔵スピーカーで警告音を鳴らします。
  - 温度: 28 ℃以上
  - 湿度: 70 %以上
  - CO2: 1000 ppm以上
- 表示判定:
  - 温度 28 ℃以上: `高温` を赤字表示、未満: `快適`
  - 湿度 70 %以上: `不快` を赤字表示、未満: `快適`
  - CO2 1000 ppm以上: `不快` を赤字表示、未満: `良好`

## センサ接続

Unit CO2L を M5Stack Core ESP32 の PORT.A に接続してください。
M5Unified の `M5.getPin(m5::pin_name_t::port_a_sda/scl)` で PORT.A の SDA/SCL を取得しています。

## センサ特性・制限事項

このプロジェクトは、SCD41 搭載の M5Stack **Unit CO2L** を想定しています。M5Stack の通常の Unit CO2 は SCD40 搭載品で、CO2 測定範囲も異なります。

Unit CO2L / SCD41 の代表仕様:

- I2C アドレス: `0x62`
- CO2 測定範囲: 400〜5000 ppm
- CO2 標準精度: ±(40 ppm + 読値の 5%)
- 温度範囲: -10〜60 ℃
- 湿度範囲: 0〜95 %RH

実装上の注意:

- SCD41 の単発測定を使用しています。初回またはスリープ復帰後の値を避けるため、Sensirion のサンプルに合わせて 1 回目の単発測定値を破棄し、2 回目を表示値にしています。そのため Button A の測定更新には数秒かかります。
- 表示される温度は SCD41 内蔵温湿度センサの値です。M5Stack 本体やケース内の発熱、設置環境の空気流量の影響を受けます。
- CO2 センサは校正状態、換気、設置位置の影響を受けます。警報用途では、実環境での妥当性確認を行ってください。
- 「音声案内」は、追加の音声データなしで動作できるよう、このプロジェクトでは警告音パターンとして実装しています。実際の日本語音声を出す場合は、WAV/MP3 再生や TTS 実装を追加してください。

## ビルド・書き込み

1. VS Code でこのフォルダを開きます。
2. PlatformIO 拡張が依存ライブラリを取得します。
3. PlatformIO の `Build` を実行します。
4. M5Stack Core ESP32 を USB 接続し、`Upload` を実行します。

CLI の場合:

```bash
pio run -e m5stack-core-esp32
pio run -e m5stack-core-esp32 -t upload
pio device monitor -b 115200
```

## 依存ライブラリ

`platformio.ini` の `lib_deps` で取得します。

- `m5stack/M5Unified @ ^0.2.13`
- `sensirion/Sensirion I2C SCD4x @ ^1.1.0`

