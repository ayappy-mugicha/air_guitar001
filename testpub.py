import mqttpub
import os
import time

import session_token

topic_stroke = "guitar/stroke"
topic_code = "guitar/code"
codes = ["c", "f", "d", "dm", "e", "a", "em", "am", "g"]
secret = os.environ.get("SESSION_HMAC_SECRET")
if not session_token.secret_is_usable(secret):
    raise SystemExit(
        "SESSION_HMAC_SECRET が未設定、16文字未満、または CHANGE_ME で始まっています"
        "（config.h と同じ実値）"
    )
client = mqttpub.publish_init()
token = session_token.make_token(secret)
seq = 1
while 1:
    for i in range(len(codes)):
        mqttpub.publish(client, topic_code, session_token.make_code_payload(token, codes[i], seq))
        seq += 1
        mqttpub.publish(client, topic_stroke, session_token.make_stroke_payload(token, "0", seq))
        seq += 1
        time.sleep(1)
        mqttpub.publish(client, topic_stroke, session_token.make_stroke_payload(token, "1", seq))
        seq += 1
        time.sleep(1)
