import atexit
import os
import signal
import sys

import dash
from dash import dcc, html, Input, Output, State
import mqttpub # 修正したmqttpubモジュールをインポート
import paho.mqtt.client as mqtt # mqtt.Client 型ヒントのためにインポート
import threading
import time

from demo_playback import (
    DEMO_STROKE_INTERVAL_MS,
    new_demo_state,
    note_code_sent,
    start_demo,
    stop_demo,
    tick_demo,
)

# --- グローバル定数定義 ---
TOPIC_GUITAR_CODE = "guitar/code"
TOPIC_GUITAR_STROKE = "guitar/stroke"
TEXT_MAJOR = "メジャー"
TEXT_MINOR = "マイナー"
TEXT_FULL_OPEN = "フルオープン"

MAJOR_CHORDS = ["C", "A", "E", "F", "G", "D"]
MINOR_CHORDS = ["D", "A", "E"]

# MQTTクライアントをグローバルで管理
mqtt_client: mqtt.Client = None
mqtt_connected = False
mqtt_lock = threading.Lock() # MQTTクライアントへのアクセスを同期するためのロック
demo_lock = threading.Lock()
_demo_state = new_demo_state()

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

_mqtt_cleanup_done = False


def _cleanup_mqtt() -> None:
    global _mqtt_cleanup_done
    if _mqtt_cleanup_done or not mqtt_client:
        return
    _mqtt_cleanup_done = True
    mqttpub.publish_stop(mqtt_client)
    print("MQTTクライアントをクリーンアップしました。")


def _handle_exit_signal(_signum: int, _frame: object) -> None:
    _cleanup_mqtt()
    sys.exit(0)


for sig in (signal.SIGINT, signal.SIGTERM):
    signal.signal(sig, _handle_exit_signal)
if hasattr(signal, "SIGBREAK"):
    signal.signal(signal.SIGBREAK, _handle_exit_signal)
atexit.register(_cleanup_mqtt)


# --- Dashアプリの初期化 ---
app = dash.Dash(__name__, 
                external_stylesheets=[
                    'https://codepen.io/chriddyp/pen/bWLwgP.css',
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

    html.Div(className='transport-row', children=[
        html.Button(
            TEXT_FULL_OPEN,
            id='full-open-button',
            n_clicks=0,
            className='full-open-button',
        ),
        html.Button(
            [
                html.Span('demo', className='demo-label'),
                html.Span(className='demo-led', **{'aria-hidden': 'true'}),
                html.Span('停止中', id='demo-status-text', className='demo-status'),
            ],
            id='demo-button',
            n_clicks=0,
            className='demo-button',
            **{
                'aria-pressed': 'false',
                'aria-label': 'カノン進行のデモを再生',
                'type': 'button',
            },
        ),
    ]),

    dcc.Interval(
        id='interval-component',
        interval=1*1000,
        n_intervals=0,
        disabled=False
    ),
    dcc.Interval(
        id='demo-interval',
        interval=DEMO_STROKE_INTERVAL_MS,
        n_intervals=0,
        disabled=True
    )
])

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

def _publish_code(message):
    with mqtt_lock:
        mqttpub.publish(mqtt_client, TOPIC_GUITAR_CODE, message)


def _publish_stroke(value):
    with mqtt_lock:
        mqttpub.publish(mqtt_client, TOPIC_GUITAR_STROKE, value)


def _status_text(chord=None, stroke=None):
    parts = []
    if chord is not None:
        parts.append(f"[{chord}]を送信")
    if stroke is not None:
        parts.append(f"ストローク {stroke}")
    if not parts:
        return dash.no_update
    return " / ".join(parts) + " ✅"


# コードボタンクリック時の処理
@app.callback(
    Output('last-sent-message', 'children'),
    Input({'type': 'chord-button', 'id': dash.ALL}, 'n_clicks'),
    Input('full-open-button', 'n_clicks'),
    prevent_initial_call=True
)
def handle_button_click(chord_clicks, full_open_clicks):
    global _demo_state
    ctx = dash.callback_context

    if not ctx.triggered:
        raise dash.exceptions.PreventUpdate

    button_id_raw = ctx.triggered[0]['prop_id']
    
    message_to_send = ""
    if 'chord-button' in button_id_raw:
        button_id_dict = eval(button_id_raw.split('.')[0]) 
        message_to_send = button_id_dict['id']
    elif 'full-open-button' in button_id_raw:
        message_to_send = "open"
    
    if message_to_send:
        with demo_lock:
            _demo_state = note_code_sent(_demo_state, message_to_send)
        _publish_code(message_to_send)
        return _status_text(chord=message_to_send)
    
    raise dash.exceptions.PreventUpdate


@app.callback(
    Output('demo-interval', 'disabled'),
    Output('demo-button', 'className'),
    Output('demo-button', 'aria-pressed'),
    Output('demo-button', 'aria-label'),
    Output('demo-status-text', 'children'),
    Output('last-sent-message', 'children', allow_duplicate=True),
    Input('demo-button', 'n_clicks'),
    prevent_initial_call=True
)
def toggle_demo(n_clicks):
    global _demo_state
    with demo_lock:
        turning_on = not _demo_state.get("on")
        if turning_on:
            _demo_state, chord, stroke = start_demo(_demo_state)
        else:
            _demo_state, chord = stop_demo(_demo_state)
            stroke = None

    _publish_code(chord)
    if stroke is not None:
        _publish_stroke(stroke)

    if turning_on:
        return (
            False,
            'demo-button is-playing',
            'true',
            'カノン進行のデモを停止',
            '再生中',
            _status_text(chord=chord, stroke=stroke),
        )
    return (
        True,
        'demo-button',
        'false',
        'カノン進行のデモを再生',
        '停止中',
        _status_text(chord=chord),
    )


@app.callback(
    Output('last-sent-message', 'children', allow_duplicate=True),
    Input('demo-interval', 'n_intervals'),
    prevent_initial_call=True
)
def on_demo_tick(n_intervals):
    global _demo_state
    with demo_lock:
        if not _demo_state.get("on"):
            raise dash.exceptions.PreventUpdate
        _demo_state, chord, stroke = tick_demo(_demo_state)

    if chord is not None:
        _publish_code(chord)
    if stroke is not None:
        _publish_stroke(stroke)
    if chord is None and stroke is None:
        raise dash.exceptions.PreventUpdate
    return _status_text(chord=chord, stroke=stroke)

# --- アプリケーションの実行 ---
if __name__ == '__main__':
    supervised = os.environ.get("AIR_GUITAR_SUPERVISED") == "1"
    app.run(
        debug=not supervised,
        host='127.0.0.1',
        port=8050,
        use_reloader=not supervised,
    )
    _cleanup_mqtt()