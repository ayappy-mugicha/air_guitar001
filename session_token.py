"""Build visitor session tokens and MQTT JSON payloads for guitar/code and guitar/stroke."""

import base64
import hashlib
import hmac
import json
import secrets
import time


def b64url(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).rstrip(b"=").decode()


def make_token(secret, sid=None, lifetime=600, now=None):
    if sid is None:
        sid = secrets.token_hex(16)
    if now is None:
        now = int(time.time())
    payload = json.dumps({"sid": sid, "iat": now, "exp": now + lifetime}, separators=(",", ":"))
    payload_b64 = b64url(payload.encode())
    sig = hmac.new(secret.encode(), payload_b64.encode(), hashlib.sha256).digest()
    return payload_b64 + "." + b64url(sig)


def make_code_payload(token: str, chord: str, seq: int) -> str:
    return json.dumps({"v": 1, "tok": token, "chord": chord, "seq": seq}, separators=(",", ":"))


def make_stroke_payload(token: str, stroke: str, seq: int) -> str:
    return json.dumps({"v": 1, "tok": token, "stroke": stroke, "seq": seq}, separators=(",", ":"))
