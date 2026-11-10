# XIAO ESP32 ギターコントローラ

Seeed XIAO ESP32C3 が MQTT を購読し、PCA9685 経由でコード押さえサーボとストロークサーボを動かします。来場者セッションの命令だけを通すため、payload は HMAC-SHA256 付きトークン付き JSON です。コード名の集合は Raspberry Pi Zero 版と同じです。

## 配線

- PCA9685 I2C アドレス: `0x40`
- XIAO ESP32C3: SDA = D4、SCL = D5（`config.h` の `I2C_SDA_PIN` / `I2C_SCL_PIN` で変更）
- コード用サーボ: チャンネル 4–7
- ストローク用サーボ: チャンネル 8

## 設定

```bash
cp firmware/include/config.example.h firmware/include/config.h
```

`config.h` に Wi-Fi SSID / パスワードと `SESSION_HMAC_SECRET`（サイト側と同じ秘密鍵、16文字以上、`CHANGE_ME` で始めない）を書き、必要なら MQTT ブローカーやサーボ角を調整します。プレースホルダのままだと起動時に停止し、トークン検証も拒否します。`config.h` は git 管理外です。コード配置・パルス幅・チャンネル番号もすべて `config.h` にあります。

時刻は NTP（`NTP_SERVER_1` / `NTP_SERVER_2`）で同期します。同期前のコード命令はすべて破棄します。最初の同期後、認証時計は `millis()` の差分蓄積で進み、SNTP の巻き戻り／飛びと 32bit 回り込みでは過去へ戻りません。シリアルに `NTP: waiting for sync` / `NTP: synced` を出します。

## 書き込み

[PlatformIO](https://platformio.org/) を使い、ボード `seeed_xiao_esp32c3` 向けにビルドします。別チップ（S3 / C6）を使う場合は `platformio.ini` の `board` と、`config.h` の `I2C_SDA_PIN` / `I2C_SCL_PIN` をそのチップに合わせてください。

```bash
pio run
pio run -t upload
pio device monitor
```

Arduino IDE を使う場合は `firmware/src` と `firmware/include` をスケッチへコピーし、ライブラリ **Adafruit PWM Servo Driver Library** と **PubSubClient** を入れてください。ボード選択は Seeed XIAO ESP32C3 にしてください。別チップを使う場合は、IDE のボード選択と `config.h` の I2C ピンを合わせてください。

## MQTT

アプリの payload 上限は 256 バイト、PubSubClient のパケットバッファは `MQTT_CMD_PAYLOAD_LEN + 32`（MQTT ヘッダ＋トピック分）です。平文のコード名は受け付けません。`guitar/stroke` の平文 `0` / `1` はサイト側（MQTTguitarchordwebsite#2 が `guitar/code` のみ JSON 化）との混在デプロイ用に残します。

| トピック | payload |
| --- | --- |
| `guitar/code` | `{"v":1,"tok":"<token>","chord":"c","seq":42}` |
| `guitar/stroke` | 平文 `0` / `1`、または `{"v":1,"tok":"<token>","stroke":"0","seq":43}` |

- `chord`: `c` `f` `d` `dm` `e` `a` `em` `am` `g` `open`
- `stroke`: 平文 `0` / `1`、または JSON 文字列 `"0"` = down、`"1"` = up。JSON 数値の `0` / `1` は型不一致として無言破棄します
- `seq`: 同じ `sid` で前回より大きい整数。新しい `sid` が受理されると、以前の `sid` はトークン期限まで拒否します（共有ギターのセッション交代）
- `sid` スロット: 期限切れ前の sid を最大 `SESSION_SID_SLOTS`（既定 64）件保持します。埋まったあとの新規セッションは、どれかのトークンが切れるまで拒否します。早期 eviction は再送穴になるためしません。サイト側は 10 分あたりの発行をこの件数未満にしてください
- トークン: `base64url(payload JSON) + "." + base64url(HMAC-SHA256(秘密鍵, base64url(payload JSON)))`
- トークン payload: `{"sid":"<hex32>","iat":<unix>,"exp":<iat+600>}`

ブローカー既定: `broker.hivemq.com:1883`

HMAC は Issue #9 の確定仕様どおり `sid` / `iat` / `exp` のみを覆い、`chord` / `stroke` / `seq` は署名対象外です。有効期限内の `tok` は、その `sid` が current のあいだベアラとして使えます。ブローカー認証やコマンド単位 MAC は後続です。

ホストから動作確認する例:

```bash
pip install paho-mqtt
cp firmware/include/config.example.h firmware/include/config.h  # 未作成なら
export SESSION_HMAC_SECRET='CHANGE_ME_SESSION_HMAC_SECRET'     # config.h と同じ値
python testpub.py
```

`testpub.py` は `mqttpub.py` / `mqttsys.py` / `session_token.py` を使います。Raspberry Pi 用の C / Python 実行系は `old/` にあります。

ホスト側の受信ロジック確認:

```bash
cp firmware/include/config.example.h firmware/include/config.h
g++ -std=c++17 -I firmware/include -I test/native \
  -DSESSION_HMAC_SECRET=\"native-test-session-hmac-secret\" \
  firmware/src/mqtt_dispatch.cpp firmware/src/session_auth.cpp \
  firmware/src/chords.cpp firmware/src/stroke.cpp \
  test/native/pca9685_stub.cpp test/native/hmac_sha256.cpp \
  test/native/test_mqtt_dispatch.cpp \
  -o /tmp/test_mqtt_dispatch && /tmp/test_mqtt_dispatch
```
