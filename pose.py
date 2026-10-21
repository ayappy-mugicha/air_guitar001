import atexit
import signal
import sys

import cv2
from ultralytics import YOLO
import mqttpub

MODEL_PATH = "yolo11n-pose.pt"
IMG_SIZE = 416
KEYPOINT_SCORE_MIN = 0.5
WINDOW_NAME = "YOLO11 Pose"
WINDOW_WIDTH = 1030
WINDOW_HEIGHT = 920

RIGHT_SHOULDER = 6
RIGHT_ELBOW = 8
RIGHT_WRIST = 10

video_path = 0
capture = cv2.VideoCapture(video_path)

KEYPOINTS_NAMES = [
    "nose",  # 0 鼻
    "eye(L)",  # 1 左目
    "eye(R)",  # 2 右目
    "ear(L)",  # 3 左耳
    "ear(R)",  # 4 右耳
    "shoulder(L)",  # 5 左肩
    "shoulder(R)",  # 6 右肩
    "elbow(L)",  # 7 左肘
    "elbow(R)",  # 8 右肘
    "wrist(L)",  # 9 左手首
    "wrist(R)",  # 10 右手首
    "hip(L)",  # 11 左腰
    "hip(R)",  # 12 右腰
    "knee(L)",  # 13 左膝
    "knee(R)",  # 14 右膝
    "ankle(L)",  # 15 左足首
    "ankle(R)",  # 16 右足首
]

# モデルの読み込み
model = YOLO(MODEL_PATH)
topic = "guitar/stroke"
client = mqttpub.publish_init()
last_stroke_state = None
_shutdown_done = False


def _shutdown() -> None:
    global _shutdown_done
    if _shutdown_done:
        return
    _shutdown_done = True
    mqttpub.publish_stop(client)
    if capture.isOpened():
        capture.release()
    cv2.destroyAllWindows()


def _handle_exit_signal(_signum: int, _frame: object) -> None:
    _shutdown()
    sys.exit(0)


for sig in (signal.SIGINT, signal.SIGTERM):
    signal.signal(sig, _handle_exit_signal)
if hasattr(signal, "SIGBREAK"):
    signal.signal(signal.SIGBREAK, _handle_exit_signal)
atexit.register(_shutdown)


def process_frame(frame):
    global last_stroke_state
    results = model(frame, imgsz=IMG_SIZE, verbose=False)

    annotated_frame = results[0].plot()

    if results[0].keypoints is None or len(results[0].keypoints.xy) == 0:
        print("人物が検出されませんでした")
        return annotated_frame

    keypoints = results[0].keypoints.xy[0]  # 1人目のキーポイント
    confs = results[0].keypoints.conf[0]   # 1人目の信頼度

    for idx, (point, score) in enumerate(zip(keypoints, confs)):
        x, y = int(point[0]), int(point[1])
        if score < KEYPOINT_SCORE_MIN:
            continue

        print(
            f"Keypoint Name={KEYPOINTS_NAMES[idx]}, X={x}, Y={y}, Score={score:.4f}")

        cv2.rectangle(
            annotated_frame,
            (x, y),
            (x + 3, y + 3),
            (255, 0, 255),
            cv2.FILLED,
            cv2.LINE_AA,
        )

        cv2.putText(
            annotated_frame,
            KEYPOINTS_NAMES[idx],
            (x + 5, y),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.5,
            (255, 0, 255),
            1,
            cv2.LINE_AA,
        )

    down_raised = False
    up_raised = False
    current_stroke_state = None

    arm_visible = (
        confs[RIGHT_SHOULDER] >= KEYPOINT_SCORE_MIN
        and confs[RIGHT_ELBOW] >= KEYPOINT_SCORE_MIN
        and confs[RIGHT_WRIST] >= KEYPOINT_SCORE_MIN
    )
    if not arm_visible:
        print("------------------------------------------------------")
        return annotated_frame

    right_elbow = keypoints[RIGHT_ELBOW]
    right_wrist = keypoints[RIGHT_WRIST]
    right_shoulder = keypoints[RIGHT_SHOULDER]

    if (right_wrist[1] > right_shoulder[1]) and right_shoulder[0] < right_wrist[0]:
        if right_wrist[1] > right_elbow[1]:
            down_raised = True
            current_stroke_state = "up"
        if right_wrist[1] < right_elbow[1] > right_shoulder[1]:
            up_raised = True
            current_stroke_state = "down"

    if current_stroke_state != last_stroke_state:
        if current_stroke_state == "down":
            mqttpub.publish(client, topic, 0)
            print("--- ダウンストロークを送信 ---")
        elif current_stroke_state == "up":
            mqttpub.publish(client, topic, 1)
            print("--- アップストロークを送信 ---")
        last_stroke_state = current_stroke_state

    if down_raised:
        cv2.putText(
            annotated_frame,
            "down stroke",
            (50, 150),
            fontFace=cv2.FONT_HERSHEY_SIMPLEX,
            fontScale=1.5,
            color=(0, 128, 255),
            thickness=4,
            lineType=cv2.LINE_AA,
        )
    elif up_raised:
        cv2.putText(
            annotated_frame,
            "up stroke",
            (50, 150),
            fontFace=cv2.FONT_HERSHEY_SIMPLEX,
            fontScale=1.5,
            color=(0, 128, 255),
            thickness=4,
            lineType=cv2.LINE_AA,
        )

    print("------------------------------------------------------")
    return annotated_frame


cv2.namedWindow(WINDOW_NAME, cv2.WINDOW_NORMAL)
cv2.resizeWindow(WINDOW_NAME, WINDOW_WIDTH, WINDOW_HEIGHT)

try:
    while capture.isOpened():
        success, frame = capture.read()
        if not success:
            break

        annotated_frame = process_frame(frame)
        cv2.imshow(WINDOW_NAME, annotated_frame)

        if cv2.waitKey(1) & 0xFF == ord("q"):
            break
finally:
    _shutdown()
