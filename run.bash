cd ~/programs/air_guitar001
source my-venv001/bin/activate
#!/bin/bash

# 実行するプログラムをここで指定してください

PROG_A_CMD=(python strok.py)
PROG_B_CMD=(python code.py)

# --- 1. シグナル捕捉関数 (クリーンアップ) の定義 ---
# Ctrl+C (SIGINT) や終了シグナル (SIGTERM) を受け取ったときに実行される
cleanup() {
    echo "" # プロンプトを改行
    echo "--- 終了シグナルを捕捉しました。子プロセスを終了します ---"
    
    # PIDを使って各プロセスにSIGTERM (シグナル番号 15) を送信
    # 2>/dev/null で、killが失敗したときのエラーメッセージを非表示にする
    kill -TERM "$PID_A" 2>/dev/null
    kill -TERM "$PID_B" 2>/dev/null
    
    # 子プロセスが終了するのを待機
    wait "$PID_A" 2>/dev/null
    wait "$PID_B" 2>/dev/null
    
    echo "--- 両方のプロセスが正常に終了しました。スクリプトを終了します ---"
    exit 0 # スクリプトを正常終了
}

# --- 2. SIGINT/SIGTERM を捕捉する設定 ---
# Ctrl+C (SIGINT) または kill コマンド (SIGTERM) を受け取ったら cleanup 関数を実行する
trap cleanup SIGINT SIGTERM

# --- 2. プロセスの同時実行 ---
echo "プロセスA (${PROG_A_CMD[*]}) をバックグラウンドで開始..."

# 配列全体を実行する際は ${配列名[@]} または ${配列名[*]} を使う
"${PROG_A_CMD[@]}" &
PID_A=$! 

echo "プロセスB (${PROG_B_CMD[*]}) をバックグラウンドで開始..."
"${PROG_B_CMD[@]}" &
PID_B=$!

echo "同時実行中... (Ctrl+C を押すと終了します)"
echo "PID A: $PID_A, PID B: $PID_B"

# --- 4. スクリプトの待機 ---
# スクリプト自体を終了させず、プロセスAが終了するまで待機する
# (ここでは、片方が終了したら全て終了させるか、無限に待ち続ける設定が可能です)
wait "$PID_A"
wait "$PID_B"

# 待機後にここまで来たら、いずれかのプロセスが終了したか、外部から kill されたことを意味する
echo "プロセスが意図せず終了しました。"
cleanup # cleanup関数を呼び出し、残ったプロセスを終了させる