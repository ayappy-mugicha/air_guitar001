import cv2
from ultralytics import YOLO
import mqttpub

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
topic = "guitar/stroke"
client = mqttpub.publish_init()


def process_frame(frame):
    try:# 推論を実行
        results = model(frame)

        annotated_frame = results[0].plot()

        if results[0].keypoints is None or len(results[0].keypoints.xy) == 0:
            print("人物が検出されませんでした")
            return annotated_frame
        # 姿勢分析結果のキーポイントを取得する
        # keypoints = results[0].keypoints.xy  # 座標
        # confs = results[0].keypoints.conf  # 信頼度

        # for keypoint in keypoints:
        # for idx, point in enumerate(keypoint):
        # x, y = int(point[0]), int(point[1])

        keypoints = results[0].keypoints.xy[0]  # 1人目のキーポイント
        confs = results[0].keypoints.conf[0]   # 1人目の信頼度

        for idx, (point, score) in enumerate(zip(keypoints, confs)):
            x, y = int(point[0]), int(point[1])
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

        # ストロークフラグ
        down_raised = False
        up_raised = False

        # 右手
        right_elbow = keypoints[8]
        right_wrist = keypoints[10]
        right_shoulder = keypoints[6]
        
        # 右腰
        right_hip = keypoints[12]
        
        # 右手腰の下
        if (right_wrist[1] > right_shoulder[1]) and right_shoulder[0] < right_wrist[0]:
            
            # ダウンストローク
            if right_wrist[1] > right_elbow[1]:
                down_raised = True
                mqttpub.publish(client,topic,"down stroke")
                
                
            # アップストローク
            if right_wrist[1] < right_elbow[1] > right_shoulder[1]:
                up_raised = True
                mqttpub.publish(client, topic,"up stroke")


            # 判定に応じた表示
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
    except KeyboardInterrupt:
        mqttpub.publish_stop(client)

while capture.isOpened():
    success, frame = capture.read()
    if not success:
        break

    annotated_frame = process_frame(frame)
    cv2.imshow("YOLOv8 Human Pose Estimation", annotated_frame)

    if cv2.waitKey(1) & 0xFF == ord("q"):
        mqttpub.publish_stop(client)
        break

capture.release()
cv2.destroyAllWindows()
