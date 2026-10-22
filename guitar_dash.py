import atexit
import os
import signal
import sys

import dash
from dash import dcc, html, Input, Output
import mqttpub
from mqttsys import MQTT_TOPIC_CODE, MQTT_TOPIC_STROKE, env_int
import paho.mqtt.client as mqtt  # mqtt.Client 型ヒントのためにインポート
import threading
import time

from chord_hold import apply_hold_event, interrupt_demo_on_press
from demo_playback import (
    DEMO_STROKE_INTERVAL_MS,
    new_demo_state,
    note_code_sent,
    start_demo,
    stop_demo,
    tick_demo,
)
from guide_copy import (
    GUIDE_CLOSE_LABEL,
    GUIDE_DIALOG_TITLE,
    GUIDE_OPEN_LABEL,
    GUIDE_STEPS,
    GUIDE_STROKE_ALT,
    GUIDE_STROKE_SRC,
    GUIDE_WARNING,
)

# --- グローバル定数定義 ---
TOPIC_GUITAR_CODE = MQTT_TOPIC_CODE
TOPIC_GUITAR_STROKE = MQTT_TOPIC_STROKE
DASH_HOST = os.environ.get("DASH_HOST", "127.0.0.1")
DASH_PORT = env_int("DASH_PORT", 8050)
TEXT_MAJOR = "メジャー"
TEXT_MINOR = "マイナー"
TEXT_FULL_OPEN = "開放弦"

MAJOR_CHORDS = ["C", "A", "E", "F", "G", "D"]
MINOR_CHORDS = ["D", "A", "E"]

DEMO_STOPPED_CLASS = "demo-button"
DEMO_PLAYING_CLASS = "demo-button is-playing"
DEMO_START_LABEL = "カノン進行のデモを再生"
DEMO_STOP_LABEL = "カノン進行のデモを停止"

# MQTTクライアントをグローバルで管理
mqtt_client: mqtt.Client = None
mqtt_connected = False
mqtt_lock = threading.Lock()  # MQTTクライアントへのアクセスを同期するためのロック
demo_lock = threading.Lock()
_demo_state = new_demo_state()
_held_chords = []


# --- MQTTクライアントの初期化とループ開始 ---
def initialize_mqtt_client():
    global mqtt_client, mqtt_connected
    try:
        mqtt_client = mqttpub.publish_init()
        start_time = time.time()
        while not mqtt_client.is_connected() and (time.time() - start_time < 10):
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


def _guide_step(index, step):
    children = [
        html.Span(str(index), className="guide-step-num"),
        html.H3(step["title"], className="guide-step-title"),
        html.P(step["body"], className="guide-step-body"),
    ]
    classes = "guide-step"
    if step["media"]:
        classes += " is-stroke"
        children.append(
            html.Img(
                src=GUIDE_STROKE_SRC,
                alt=GUIDE_STROKE_ALT,
                className="guide-stroke",
            )
        )
    return html.Li(children, className=classes)


def _guide_dialog():
    return html.Div(
        id="guide-dialog",
        className="guide-dialog",
        hidden=True,
        children=[
            html.Div(className="guide-backdrop", **{"aria-hidden": "true"}),
            html.Div(
                className="guide-sheet",
                role="dialog",
                **{
                    "aria-modal": "true",
                    "aria-labelledby": "guide-heading",
                },
                children=[
                    html.Div(
                        className="guide-sheet-bar",
                        children=[
                            html.H2(GUIDE_DIALOG_TITLE, id="guide-heading"),
                            html.Button(
                                GUIDE_CLOSE_LABEL,
                                id="guide-close",
                                className="guide-close-button",
                                type="button",
                            ),
                        ],
                    ),
                    html.P(GUIDE_WARNING, className="guide-warning"),
                    html.Ol(
                        className="guide-steps",
                        children=[
                            _guide_step(index, step)
                            for index, step in enumerate(GUIDE_STEPS, start=1)
                        ],
                    ),
                ],
            ),
        ],
    )


def _chord_pad(letter, quality, chord_id, extra_class):
    return html.Button(
        [
            html.Span(letter, className="pad-letter"),
            html.Span(quality, className="pad-quality"),
            html.Span("押さえ中", className="pad-held-label"),
        ],
        id={"type": "chord-pad", "id": chord_id},
        n_clicks=0,
        className=f"chord-pad {extra_class}",
        **{
            "data-chord": chord_id,
            "aria-pressed": "false",
            "aria-label": f"{letter} {quality} を押さえる",
            "type": "button",
        },
    )


app = dash.Dash(
    __name__,
    meta_tags=[{"name": "viewport", "content": "width=device-width, initial-scale=1.0"}],
)
app.title = "ギターコードコントローラー"

app.layout = html.Div(
    className="app-shell",
    children=[
        html.Header(
            className="topbar",
            children=[
                html.Div(
                    className="topbar-heading",
                    children=[
                        html.H1("ギターコードコントローラー"),
                        html.Button(
                            GUIDE_OPEN_LABEL,
                            id="guide-open",
                            className="guide-open-button",
                            type="button",
                            **{
                                "aria-expanded": "false",
                                "aria-controls": "guide-dialog",
                            },
                        ),
                    ],
                ),
                html.Div(className="topbar-status", children=[
                    html.Div(id="mqtt-status"),
                    html.Div(id="last-sent-message", className="last-sent"),
                ]),
            ],
        ),
        html.Div(
            className="app-container",
            children=[
                html.Section(
                    className="chord-section major-section",
                    children=[
                        html.H2("メジャー"),
                        html.Div(
                            className="button-grid major-grid",
                            children=[
                                _chord_pad(chord, TEXT_MAJOR, chord.lower(), "is-major")
                                for chord in MAJOR_CHORDS
                            ],
                        ),
                    ],
                ),
                html.Section(
                    className="chord-section minor-section",
                    children=[
                        html.H2("マイナー"),
                        html.Div(
                            className="button-grid minor-grid",
                            children=[
                                _chord_pad(chord, TEXT_MINOR, f"{chord.lower()}m", "is-minor")
                                for chord in MINOR_CHORDS
                            ],
                        ),
                    ],
                ),
            ],
        ),
        html.Div(
            className="transport-row",
            children=[
                html.Button(
                    [
                        html.Span(TEXT_FULL_OPEN, className="open-label"),
                        html.Span("コードを外す", className="open-hint"),
                    ],
                    id="full-open-button",
                    n_clicks=0,
                    className="full-open-button",
                    **{"type": "button", "aria-label": "開放弦にする"},
                ),
                html.Button(
                    [
                        html.Span("demo", className="demo-label"),
                        html.Span(className="demo-led", **{"aria-hidden": "true"}),
                        html.Span("停止中", id="demo-status-text", className="demo-status"),
                    ],
                    id="demo-button",
                    n_clicks=0,
                    className=DEMO_STOPPED_CLASS,
                    **{
                        "aria-pressed": "false",
                        "aria-label": DEMO_START_LABEL,
                        "type": "button",
                    },
                ),
            ],
        ),
        _guide_dialog(),
        dcc.Store(id="hold-event"),
        dcc.Interval(
            id="interval-component",
            interval=1 * 1000,
            n_intervals=0,
            disabled=False,
        ),
        dcc.Interval(
            id="demo-interval",
            interval=DEMO_STROKE_INTERVAL_MS,
            n_intervals=0,
            disabled=True,
        ),
    ],
)


def _publish_code(message):
    with mqtt_lock:
        mqttpub.publish(mqtt_client, TOPIC_GUITAR_CODE, message)


def _publish_stroke(value):
    with mqtt_lock:
        mqttpub.publish(mqtt_client, TOPIC_GUITAR_STROKE, value)


def _display_chord(code):
    if not code or code == "open":
        return "開放"
    if code.endswith("m"):
        return f"{code[:-1].upper()}m"
    return code.upper()


def _status_text(chord=None, stroke=None, holding=False, demo_stopped=False):
    parts = []
    if demo_stopped:
        parts.append("demo を停止")
    if chord is not None:
        if chord == "open":
            parts.append("開放")
        elif holding:
            parts.append(f"{_display_chord(chord)} を押さえ中")
        else:
            parts.append(f"[{chord}]を送信")
    if stroke is not None:
        parts.append(f"ストローク {stroke}")
    if not parts:
        return dash.no_update
    return " / ".join(parts)


def _demo_stopped_outputs():
    return (
        True,
        DEMO_STOPPED_CLASS,
        "false",
        DEMO_START_LABEL,
        "停止中",
    )


@app.callback(
    Output("mqtt-status", "children"),
    Input("interval-component", "n_intervals"),
)
def update_mqtt_status(n):
    global mqtt_connected
    if mqtt_connected:
        return html.Span("MQTT 接続完了", className="mqtt-ok")
    return html.Span("MQTT 接続中...", className="mqtt-wait")


@app.callback(
    Output("last-sent-message", "children"),
    Output("demo-interval", "disabled", allow_duplicate=True),
    Output("demo-button", "className", allow_duplicate=True),
    Output("demo-button", "aria-pressed", allow_duplicate=True),
    Output("demo-button", "aria-label", allow_duplicate=True),
    Output("demo-status-text", "children", allow_duplicate=True),
    Input("hold-event", "data"),
    prevent_initial_call=True,
)
def handle_chord_hold(event):
    global _demo_state, _held_chords
    if not event or not isinstance(event, dict):
        raise dash.exceptions.PreventUpdate

    action = event.get("action")
    chord_id = event.get("id")
    with demo_lock:
        demo_was_on = bool(_demo_state.get("on"))
        _held_chords, message_to_send = apply_hold_event(_held_chords, action, chord_id)
        should_stop = (
            interrupt_demo_on_press(demo_was_on, action)
            and message_to_send not in (None, "open")
        )
        if should_stop:
            _demo_state = new_demo_state()
        elif message_to_send:
            _demo_state = note_code_sent(_demo_state, message_to_send)

    if not message_to_send and not should_stop:
        raise dash.exceptions.PreventUpdate

    if message_to_send:
        _publish_code(message_to_send)

    holding = bool(_held_chords) and message_to_send not in (None, "open")
    status = _status_text(
        chord=message_to_send,
        holding=holding,
        demo_stopped=should_stop,
    )
    if should_stop:
        return (status, *_demo_stopped_outputs())
    return (
        status,
        dash.no_update,
        dash.no_update,
        dash.no_update,
        dash.no_update,
        dash.no_update,
    )


@app.callback(
    Output("last-sent-message", "children", allow_duplicate=True),
    Input("full-open-button", "n_clicks"),
    prevent_initial_call=True,
)
def handle_full_open(n_clicks):
    global _demo_state
    with demo_lock:
        _demo_state = note_code_sent(_demo_state, "open")
    _publish_code("open")
    return _status_text(chord="open")


@app.callback(
    Output("demo-interval", "disabled"),
    Output("demo-button", "className"),
    Output("demo-button", "aria-pressed"),
    Output("demo-button", "aria-label"),
    Output("demo-status-text", "children"),
    Output("last-sent-message", "children", allow_duplicate=True),
    Input("demo-button", "n_clicks"),
    prevent_initial_call=True,
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
            DEMO_PLAYING_CLASS,
            "true",
            DEMO_STOP_LABEL,
            "再生中",
            _status_text(chord=chord, stroke=stroke),
        )
    return (
        *_demo_stopped_outputs(),
        _status_text(chord=chord),
    )


@app.callback(
    Output("last-sent-message", "children", allow_duplicate=True),
    Input("demo-interval", "n_intervals"),
    prevent_initial_call=True,
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

if __name__ == "__main__":
    supervised = os.environ.get("AIR_GUITAR_SUPERVISED") == "1"
    app.run(
        debug=not supervised,
        host=DASH_HOST,
        port=DASH_PORT,
        use_reloader=not supervised,
    )
    _cleanup_mqtt()
