# M5Stack Core ESP32 Button LCD Demo

対象ボード: M5Stack Core ESP32  
PlatformIO Board ID: `m5stack-core-esp32`  
Framework: Arduino

## 動作仕様

- ボタンA: LCD画面を白色にし、中央に黒字で「おはよう」と表示
- ボタンB: LCD画面を黄色にし、中央に黒字で「こんにちは」と表示
- ボタンC: LCD画面を濃い青色にし、中央に白字で「こんにちは」と表示

## 使い方

1. VS Code + PlatformIO で、このフォルダを開きます。
2. PlatformIO が依存ライブラリを自動取得するまで待ちます。
3. M5Stack Core ESP32 を USB 接続します。
4. PlatformIO の `Upload` を実行します。

CLI の場合:

```sh
pio run -t upload
```

シリアルモニタを開く場合:

```sh
pio device monitor -b 115200
```

## 補足

日本語表示用に M5GFX の `fonts::efontJA_24` を使用しています。ソースファイルは UTF-8 として保存してください。
