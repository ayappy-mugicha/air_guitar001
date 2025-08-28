import cv2
from ultralytics import YOLO

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
model = YOLO("yolov8n-pose.pt")


def process_frame(frame):
    # 推論を実行
    results = model(frame)

    annotated_frame = results[0].plot()

    # 姿勢分析結果のキーポイントを取得する
    keypoints = results[0].keypoints.xy  # 座標
    confs = results[0].keypoints.conf  # 信頼度

    for keypoint in keypoints:
        for idx, point in enumerate(keypoint):
            x, y = int(point[0]), int(point[1])
            score = confs[0][idx]

            # スコアが0.5以下なら描画しない
            if score < 0.5:
                continue

            print(
                f"Keypoint Name={KEYPOINTS_NAMES[idx]}, X={x}, Y={y}, Score={score:.4f}")

            # 紫の四角を描画
            cv2.rectangle(
                annotated_frame,
                (x, y),
                (x + 3, y + 3),
                (255, 0, 255),
                cv2.FILLED,
                cv2.LINE_AA,
            )

            # キーポイントの部位名称を描画
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

    print("------------------------------------------------------")
    return annotated_frame


while capture.isOpened():
    success, frame = capture.read()
    if not success:
        break

    annotated_frame = process_frame(frame)
    cv2.imshow("YOLOv8 Human Pose Estimation", annotated_frame)

    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

capture.release()
cv2.destroyAllWindows()
