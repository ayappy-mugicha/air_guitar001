# XIAO ESP32 ギターコントローラ

Seeed XIAO ESP32S3 が MQTT を購読し、PCA9685 経由でコード押さえサーボとストロークサーボを動かします。トピックとコード名は Raspberry Pi Zero 版と同じです。

## 配線

- PCA9685 I2C アドレス: `0x40`
- XIAO ESP32S3: SDA = D4、SCL = D5（`config.h` の `I2C_SDA_PIN` / `I2C_SCL_PIN` で変更）
- コード用サーボ: チャンネル 4–7
- ストローク用サーボ: チャンネル 8

## 設定

```bash
cp firmware/include/config.example.h firmware/include/config.h
```

`config.h` に Wi-Fi SSID / パスワードを書き、必要なら MQTT ブローカーやサーボ角を調整します。コード配置・パルス幅・チャンネル番号もすべて `config.h` にあります。

## 書き込み

[PlatformIO](https://platformio.org/) を使い、ボード `seeed_xiao_esp32s3` 向けにビルドします。

```bash
pio run
pio run -t upload
pio device monitor
```

Arduino IDE を使う場合は `firmware/src` と `firmware/include` をスケッチへコピーし、ライブラリ **Adafruit PWM Servo Driver Library** と **PubSubClient** を入れてください。ボードは Seeed XIAO ESP32S3、別チップ（C3 / C6）は `platformio.ini` の `board` と I2C ピンを合わせてください。

## MQTT

| トピック | payload |
| --- | --- |
| `guitar/code` | `c` `f` `d` `dm` `e` `a` `em` `am` `g` `open` |
| `guitar/stroke` | `0` = down、`1` = up |

ブローカー既定: `broker.hivemq.com:1883`

ホストから動作確認する例:

```bash
pip install paho-mqtt
python testpub.py
```

`testpub.py` は `mqttpub.py` / `mqttsys.py` を使います。Raspberry Pi 用の C / Python 実行系は `old/` にあります。
