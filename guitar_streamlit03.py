import dash
from dash import dcc, html, Input, Output, State
import mqttpub # 修正したmqttpubモジュールをインポート
import paho.mqtt.client as mqtt # mqtt.Client 型ヒントのためにインポート
import threading
import time

# --- グローバル定数定義 ---
TOPIC_GUITAR_CODE = "guitar/code"
TEXT_MAJOR = "メジャー"
TEXT_MINOR = "マイナー"
TEXT_FULL_OPEN = "フルオープン"

MAJOR_CHORDS = ["C", "A", "E", "F", "G", "D"]
MINOR_CHORDS = ["D", "A", "E"]

# MQTTクライアントをグローバルで管理
mqtt_client: mqtt.Client = None
mqtt_connected = False
mqtt_lock = threading.Lock() # MQTTクライアントへのアクセスを同期するためのロック

# --- MQTTクライアントの初期化とループ開始 ---
def initialize_mqtt_client():
    global mqtt_client, mqtt_connected
    try:
        # mqttpub.init_mqtt_client を呼び出す
        mqtt_client = mqttpub.publish_init() 
        # mqtt_client.loop_start() # バックグラウンドスレッドでループを開始
        # 接続が確立するまで少し待つ
        start_time = time.time()
        while not mqtt_client.is_connected() and (time.time() - start_time < 10): # 待機時間を10秒に延長
            print(f"MQTT接続待機中... ({time.time() - start_time:.1f}s)")
            time.sleep(0.5)
        
        if mqtt_client.is_connected():
            mqtt_connected = True
            print("MQTTクライアントの初期化と接続が完了しました。")
        else:
            print("MQTTクライアントの接続に失敗しました。（タイムアウト）")
            mqtt_connected = False

    except Exception as e:
        print(f"MQTTクライアント初期化中にエラーが発生しました: {e}")
        mqtt_connected = False

# アプリケーション起動時にMQTTクライアントを初期化
# DashアプリケーションはWSGIサーバー上で動作するため、
# この初期化は一度だけ、アプリケーションのメインプロセスで行われるべき
print("MQTTクライアントを初期化中...")
initialize_mqtt_client()
print(f"初期化後のMQTT接続状態: {mqtt_connected}")


# --- Dashアプリの初期化 ---
app = dash.Dash(__name__, 
                external_stylesheets=[
                    'https://codepen.io/chriddyp/pen/bWLwgP.css', # Minimal CSS for basic styling
                    # カスタムCSSをここにインラインで追加
                    # または external_stylesheets=[..., '/assets/styles.css'] でファイル指定
                ],
                meta_tags=[{'name': 'viewport', 'content': 'width=device-width, initial-scale=1.0'}])

app.title = "ギターコードコントローラー (Dash)"

# カスタムCSSスタイルをインラインで追加 (または外部ファイルに記述)
# メディアクエリはDashの`style`プロパティでは直接機能しないため、CSSファイルを使うのが理想的ですが、
# ここでは`external_stylesheets`に直接CSS文字列を渡す形で記述します。
# 実際には`/assets/styles.css`のような外部CSSファイルを使うことを強く推奨します。
# 例: `app = dash.Dash(__name__, external_stylesheets=[... , '/assets/styles.css'])`
# そして `assets/styles.css` ファイルに以下のCSSを書く
app.layout = html.Div(style={
    'fontFamily': 'Arial, sans-serif',
    'maxWidth': '900px',
    'margin': '0 auto',
    'padding': '20px',
    'backgroundColor': '#f0f2f5',
    'minHeight': '100vh',
    'boxSizing': 'border-box'
}, children=[
    html.H1("🎸 ギターコードコントローラー", style={'textAlign': 'center', 'color': '#333', 'marginBottom': '25px'}),

    html.Div(id='mqtt-status', style={'textAlign': 'center', 'fontWeight': 'bold', 'marginBottom': '15px'}),
    html.Div(id='last-sent-message', style={'textAlign': 'center', 'color': '#007bff', 'fontSize': '0.9em', 'marginBottom': '20px'}),

    html.Div(className='app-container', style={
        'display': 'flex',
        'flexDirection': 'column', # デフォルトは縦積み (スマホ対応)
        'gap': '20px',
        'padding': '10px 0',
    }, children=[
        # メジャーコードセクション
        html.Div(className='chord-section major-section', style={
            'flex': '1', # 均等に幅を分割 (PCで)
            'backgroundColor': '#fff',
            'borderRadius': '10px', # 角を少し丸く
            'boxShadow': '0 4px 8px rgba(0,0,0,0.15)', # 影を強調
            'padding': '20px 15px', # パディングを調整
            'textAlign': 'center',
            'display': 'flex', # Flexboxを使って内部要素を中央寄せ
            'flexDirection': 'column',
        }, children=[
            html.H2("メジャーコード", style={'color': '#555', 'marginBottom': '20px', 'borderBottom': '1px solid #eee', 'paddingBottom': '10px', 'fontSize': '1.8em'}),
            html.Div(className='button-grid', style={
                'display': 'grid',
                'gridTemplateColumns': 'repeat(auto-fit, minmax(100px, 1fr))', # スマホでは2-3列、PCではもっと
                'gap': '12px', # ボタン間の隙間
                'justifyItems': 'center',
                'flexGrow': '1', # 親の高さに合わせて伸びる
                'alignContent': 'center', # グリッドを中央寄せ
            }, children=[
                html.Button(
                    f"{chord}{TEXT_MAJOR}", 
                    id={'type': 'chord-button', 'id': f"{chord.lower()}"}, 
                    n_clicks=0,
                    style={
                        'fontSize': '20px', 'height': '80px', 'width': '100%', 'maxWidth': '120px',
                        'borderRadius': '12px', 'border': 'none', 'backgroundColor': '#007bff', 'color': 'white',
                        'fontWeight': 'bold', 'boxShadow': '2px 2px 5px rgba(0,0,0,0.2)',
                        'transition': 'all 0.2s ease-in-out',
                        'cursor': 'pointer',
                    }
                ) for chord in MAJOR_CHORDS
            ]),
        ]),

        # マイナーコードセクション
        html.Div(className='chord-section minor-section', style={
            'flex': '1', # 均等に幅を分割 (PCで)
            'backgroundColor': '#fff',
            'borderRadius': '10px', # 角を少し丸く
            'boxShadow': '0 4px 8px rgba(0,0,0,0.15)', # 影を強調
            'padding': '20px 15px', # パディングを調整
            'textAlign': 'center',
            'display': 'flex', # Flexboxを使って内部要素を中央寄せ
            'flexDirection': 'column',
        }, children=[
            html.H2("マイナーコード", style={'color': '#555', 'marginBottom': '20px', 'borderBottom': '1px solid #eee', 'paddingBottom': '10px', 'fontSize': '1.8em'}),
            html.Div(className='button-grid', style={
                'display': 'grid',
                'gridTemplateColumns': 'repeat(auto-fit, minmax(100px, 1fr))',
                'gap': '12px',
                'justifyItems': 'center',
                'flexGrow': '1',
                'alignContent': 'center',
            }, children=[
                html.Button(
                    f"{chord}{TEXT_MINOR}", 
                    id={'type': 'chord-button', 'id': f"{chord.lower()}m"}, 
                    n_clicks=0,
                    style={
                        'fontSize': '20px', 'height': '80px', 'width': '100%', 'maxWidth': '120px',
                        'borderRadius': '12px', 'border': 'none', 'backgroundColor': '#6c757d', 'color': 'white',
                        'fontWeight': 'bold', 'boxShadow': '2px 2px 5px rgba(0,0,0,0.2)',
                        'transition': 'all 0.2s ease-in-out',
                        'cursor': 'pointer',
                    }
                ) for chord in MINOR_CHORDS
            ]),
        ]),
    ]),

    # フルオープンボタン
    html.Div(style={'textAlign': 'center', 'marginTop': '30px', 'marginBottom': '20px'}, children=[
        html.Button(
            TEXT_FULL_OPEN,
            id='full-open-button',
            n_clicks=0,
            style={
                'fontSize': '28px', 'height': '70px', 'width': '80%', 'maxWidth': '280px', # フルオープンボタンの幅を調整
                'borderRadius': '12px', 'border': 'none', 'backgroundColor': '#28a745', 'color': 'white',
                'fontWeight': 'bold', 'boxShadow': '3px 3px 8px rgba(0,0,0,0.3)',
                'transition': 'all 0.2s ease-in-out',
                'cursor': 'pointer',
            }
        )
    ]),

    # ダミーのDiv (コールバックのトリガーとして使用)
    dcc.Interval(
        id='interval-component',
        interval=1*1000, # 1秒ごとに更新 (MQTT接続ステータスチェック用など)
        n_intervals=0,
        disabled=False
    )
])

# --- 外部CSSファイルによるレイアウト調整 (Dashの推奨アプローチ) ---
# Dashアプリケーションのルートディレクトリに 'assets' フォルダを作成し、
# その中に 'styles.css' というファイルを作成し、以下の内容を記述します。
# これにより、html.Divのstyleプロパティに直接書くよりも見やすく、メンテしやすいです。
#
# <プロジェクトルート>/
# ├── dash_guitar_controller.py
# ├── mqttpub.py
# ├── mqttsys.py
# └── assets/
#     └── styles.css  <-- このファイル

# assets/styles.css の内容:
"""
@media (min-width: 768px) { /* タブレット以上の画面幅で適用 */
    .app-container {
        flex-direction: row !important; /* 横並びにする */
    }
}

.chord-section {
    min-height: 300px; /* セクションの最小高さを設定して見た目を揃える */
}

/* ボタンのホバーエフェクト */
button:hover {
    transform: translateY(-2px);
    box-shadow: 4px 4px 10px rgba(0,0,0,0.35);
}

/* 個別のボタンの色調整 */
#full-open-button:hover {
    background-color: #218838 !important;
}

.major-section .button-grid button:hover {
    background-color: #0056b3 !important;
}

.minor-section .button-grid button:hover {
    background-color: #5a6268 !important;
}
"""

# --- コールバック関数 ---

# MQTT接続ステータスの更新
@app.callback(
    Output('mqtt-status', 'children'),
    Input('interval-component', 'n_intervals')
)
def update_mqtt_status(n):
    global mqtt_connected # グローバル変数を参照
    if mqtt_connected:
        return html.Span("MQTT接続完了 ✅", style={'color': '#28a745'})
    else:
        return html.Span("MQTT接続中...", style={'color': '#dc3545'})

# コードボタンクリック時の処理
@app.callback(
    Output('last-sent-message', 'children'),
    Input({'type': 'chord-button', 'id': dash.ALL}, 'n_clicks'),
    Input('full-open-button', 'n_clicks'),
    State('last-sent-message', 'children'), # 前回のメッセージを保持
    prevent_initial_call=True # 最初のロード時にコールバックが発火するのを防ぐ
)
def handle_button_click(chord_clicks, full_open_clicks, current_message):
    ctx = dash.callback_context # どのコンポーネントがトリガーしたかを取得

    if not ctx.triggered:
        raise dash.exceptions.PreventUpdate # トリガーがなければ更新しない

    button_id_raw = ctx.triggered[0]['prop_id']
    
    message_to_send = ""
    # 動的IDか通常のIDかで処理を分岐
    if 'chord-button' in button_id_raw:
        # 動的IDの文字列を辞書として評価し、id部分を取得
        button_id_dict = eval(button_id_raw.split('.')[0]) 
        message_to_send = button_id_dict['id']
    elif 'full-open-button' in button_id_raw:
        message_to_send = "open"
    
    if message_to_send:
        with mqtt_lock: # MQTTクライアントへのアクセスを同期
            mqttpub.publish(mqtt_client, TOPIC_GUITAR_CODE, message_to_send)
        return f"[{message_to_send}]を送信 ✅"
    
    raise dash.exceptions.PreventUpdate

# --- アプリケーションの実行 ---
if __name__ == '__main__':
    # run_server は非推奨となり app.run に置き換わった
    app.run(debug=True, host='0.0.0.0', port=8050) # ローカルIPでアクセス可能に
    
    # アプリケーション終了時にMQTTクライアントをクリーンアップ
    # この部分はサーバーの終了方法に依存するため、確実ではない場合がある
    # 例: Ctrl+Cで終了した場合、このコードは実行されないことが多い
    if mqtt_client:
        mqttpub.publish_stop(mqtt_client)
        print("MQTTクライアントをクリーンアップしました。")