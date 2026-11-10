import mqttpub
import os
import time

import session_token

topic_stroke = "guitar/stroke"
topic_code = "guitar/code"
codes = ["c", "f", "d", "dm", "e", "a", "em", "am", "g"]
client = mqttpub.publish_init()
secret = os.environ.get("SESSION_HMAC_SECRET")
if not secret:
    raise SystemExit("SESSION_HMAC_SECRET を設定してください（config.h と同じ値）")
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
