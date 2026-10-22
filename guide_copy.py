"""User-facing guide copy shared by the overlay and the booth sheet."""

GUIDE_OPEN_LABEL = "使い方"
GUIDE_CLOSE_LABEL = "閉じる"
GUIDE_DIALOG_TITLE = "使い方"
GUIDE_SHEET_TITLE = "えあ・ぎたー 使い方"
GUIDE_WARNING = "触るのはひとりだけ。同時にみんなでさわらないで。"
GUIDE_STROKE_ALT = "右手がカメラに映る位置でストロークしているところ"
GUIDE_STROKE_SRC = "/assets/guide-stroke.svg"

GUIDE_STEPS = (
    {
        "title": "つながるまで待って",
        "body": "「MQTT 接続完了」って出たら、コードを押してね。",
        "media": False,
    },
    {
        "title": "コードを押さえる",
        "body": "弾きたいコードのボタンを、指で押したまま。離すとコードも外れるよ。",
        "media": False,
    },
    {
        "title": "ストロークする",
        "body": "カメラは右手が映る位置に動かして。あとは自分でジャラーン。タイミングはカメラが見てる。",
        "media": True,
    },
    {
        "title": "コードを外す",
        "body": "「開放弦」を押すと、全部はずれる。",
        "media": False,
    },
    {
        "title": "デモを聞く",
        "body": "自分で弾かずに聞きたいときは「demo」。カノン進行が流れるよ。コードを押すとデモは止まる。",
        "media": False,
    },
)
