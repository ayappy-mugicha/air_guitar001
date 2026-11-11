# スタッフ用ツール

## WebサイトのPIN設定

`Set-GuitarPin.ps1` は、文化祭当日にスタッフが Web サイトの当日PINを設定するための Windows PowerShell スクリプトです。Webサイト本体や Cloudflare では実行されません。

### 事前準備

1. Cloudflare の Worker `mqttguitarchordwebsite` に、暗号化された Secret として `STAFF_SECRET` を登録します。
2. このリポジトリのローカル作業フォルダ直下に `.env` を作成し、`.env.example` を参考に、次の行へ Cloudflare と同じ値を設定します。

```ini
STAFF_SECRET=<Cloudflareに登録したものと同じ値>
```

`.env` は `.gitignore` で除外されており、Git の追跡対象ではありません。実値をコミットしたり、`git add -f .env` で強制追加したりしないでください。

### 実行

リポジトリ直下で次を実行し、表示に従って4桁のPINを入力します。

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\Set-GuitarPin.ps1
```

PINを引数で渡すこともできます。

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\Set-GuitarPin.ps1 -Pin 1234
```

成功すると、APIが返したJSTの日付が表示されます。PINは設定した当日だけ有効です。

### 動作確認

ネットワークへ接続せず、`.env` の読み取りとPIN形式の検証だけを確認できます。

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\Set-GuitarPin.ps1 -SelfTest
```

### 再発行時の注意

`STAFF_SECRET` は次の2か所に同じ値が必要です。

- Cloudflare：リクエストを検証するサーバー側の値
- スタッフPCの `.env`：このスクリプトが認証に使うローカル側の値

片方だけを変更すると `401 UNAUTHORIZED` になります。`SESSION_HMAC_SECRET` はこのツールでは使わないため、変更しないでください。
