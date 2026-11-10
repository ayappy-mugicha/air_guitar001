#include "session_auth.h"
#include "config.h"

#include <stdint.h>
#include <string.h>
#include <time.h>

#ifndef SESSION_TIME_MIN
#define SESSION_TIME_MIN 1600000000L
#endif
#ifndef SESSION_CLOCK_SKEW_SEC
#define SESSION_CLOCK_SKEW_SEC 30
#endif
#ifndef MQTT_CMD_RATE_LIMIT
#define MQTT_CMD_RATE_LIMIT 20
#endif
#ifndef MQTT_CMD_RATE_WINDOW_MS
#define MQTT_CMD_RATE_WINDOW_MS 1000
#endif

#if defined(ARDUINO)
#include <Arduino.h>
#include <mbedtls/md.h>

bool sessionHmacSha256(const unsigned char *key, size_t key_len, const unsigned char *data,
                       size_t data_len, unsigned char out[32]) {
    if (key == nullptr || data == nullptr || out == nullptr) {
        return false;
    }
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (info == nullptr) {
        return false;
    }
    return mbedtls_md_hmac(info, key, key_len, data, data_len, out) == 0;
}
#endif

enum {
    KEY_V = 1u,
    KEY_TOK = 2u,
    KEY_CHORD = 4u,
    KEY_STROKE = 8u,
    KEY_SEQ = 16u,
    KEY_SID = 32u,
    KEY_IAT = 64u,
    KEY_EXP = 128u
};

struct JsonString {
    const char *p;
    size_t n;
};

struct CommandEnvelope {
    int64_t v;
    JsonString tok;
    JsonString chord;
    JsonString stroke;
    int64_t seq;
    unsigned seen;
};

struct TokenPayload {
    JsonString sid;
    int64_t iat;
    int64_t exp;
    unsigned seen;
};

static bool g_now_override = false;
static time_t g_now_value = 0;
static bool g_millis_override = false;
static uint32_t g_millis_value = 0;

struct SessionSlot {
    char sid[SESSION_SID_LEN + 1];
    uint32_t last_seq;
    int64_t exp;
    bool occupied;
    bool current;
};

static SessionSlot g_slots[SESSION_SID_SLOTS];

static bool g_have_pending = false;
static SessionCommand g_pending;

static unsigned g_rate_count = 0;
static unsigned long g_rate_window_start = 0;

static bool g_clock_locked = false;
static time_t g_lock_unix = 0;
static uint32_t g_last_millis = 0;
static uint64_t g_elapsed_ms = 0;

static uint32_t sessionMillis(void) {
    if (g_millis_override) {
        return g_millis_value;
    }
#if defined(ARDUINO)
    return millis();
#else
    return 0;
#endif
}

static void sessionAdvanceMillis(uint32_t ms) {
    if (!g_clock_locked) {
        return;
    }
    // uint32 delta matches ESP32 millis() wrap. Adding into uint64 keeps
    // elapsed from collapsing to 0 the way a 64-bit unsigned long would.
    const uint32_t delta = ms - g_last_millis;
    g_last_millis = ms;
    g_elapsed_ms += (uint64_t)delta;
}

static time_t sessionNow(void) {
    const time_t wall = g_now_override ? g_now_value : time(nullptr);
    const uint32_t ms = sessionMillis();
    if (!g_clock_locked) {
        if (wall >= (time_t)SESSION_TIME_MIN) {
            g_clock_locked = true;
            g_lock_unix = wall;
            g_last_millis = ms;
            g_elapsed_ms = 0;
        }
        return wall;
    }
    // After the first plausible NTP sample, ignore later wall-clock jumps
    // (including SNTP spoof / rollback) and advance only with millis().
    sessionAdvanceMillis(ms);
    return g_lock_unix + (time_t)(g_elapsed_ms / 1000ULL);
}

void sessionAuthReset(void) {
    g_now_override = false;
    g_now_value = 0;
    g_millis_override = false;
    g_millis_value = 0;
    g_clock_locked = false;
    g_lock_unix = 0;
    g_last_millis = 0;
    g_elapsed_ms = 0;
    memset(g_slots, 0, sizeof(g_slots));
    g_have_pending = false;
    memset(&g_pending, 0, sizeof(g_pending));
    g_rate_count = 0;
    g_rate_window_start = 0;
}

void sessionAuthSetNow(time_t now) {
    g_now_override = true;
    g_now_value = now;
}

void sessionAuthSetMillis(unsigned long ms) {
    g_millis_override = true;
    g_millis_value = (uint32_t)ms;
}

void sessionAuthTick(void) {
    (void)sessionNow();
}

bool sessionAuthClockLocked(void) {
    return g_clock_locked;
}

bool sessionSecretIsUsable(const char *secret) {
    if (secret == nullptr) {
        return false;
    }
    const size_t n = strlen(secret);
    if (n < (size_t)SESSION_SECRET_MIN_LEN) {
        return false;
    }
    static const char kPlaceholder[] = "CHANGE_ME";
    const size_t plen = sizeof(kPlaceholder) - 1;
    if (n >= plen && memcmp(secret, kPlaceholder, plen) == 0) {
        return false;
    }
    return true;
}

static const char *skipWs(const char *p, const char *end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
        p++;
    }
    return p;
}

static bool parseString(const char **pp, const char *end, JsonString *out) {
    const char *p = *pp;
    if (p >= end || *p != '"') {
        return false;
    }
    p++;
    const char *start = p;
    while (p < end && *p != '"') {
        if (*p == '\\' || (unsigned char)*p < 0x20) {
            return false;
        }
        p++;
    }
    if (p >= end || *p != '"') {
        return false;
    }
    out->p = start;
    out->n = (size_t)(p - start);
    *pp = p + 1;
    return true;
}

static bool parseInt64(const char **pp, const char *end, int64_t *out) {
    const char *p = *pp;
    if (p >= end || *p < '0' || *p > '9') {
        return false;
    }
    const bool leading_zero = (*p == '0');
    int64_t v = 0;
    int digits = 0;
    while (p < end && *p >= '0' && *p <= '9') {
        digits++;
        if (leading_zero && digits > 1) {
            return false;
        }
        if (v > (INT64_MAX - (*p - '0')) / 10) {
            return false;
        }
        v = v * 10 + (*p - '0');
        p++;
    }
    if (digits == 0) {
        return false;
    }
    if (p < end && (*p == '.' || *p == 'e' || *p == 'E')) {
        return false;
    }
    *out = v;
    *pp = p;
    return true;
}

static bool keyEq(const JsonString *key, const char *lit) {
    const size_t n = strlen(lit);
    return key->n == n && memcmp(key->p, lit, n) == 0;
}

static bool copyCstr(char *dest, size_t dest_len, const JsonString *s) {
    if (dest_len == 0 || s->n >= dest_len) {
        return false;
    }
    memcpy(dest, s->p, s->n);
    dest[s->n] = '\0';
    return true;
}

static bool parseObject(const char *json, size_t json_len, bool (*on_field)(const JsonString *key,
                                                                            const char **pp,
                                                                            const char *end,
                                                                            void *ctx),
                        void *ctx) {
    if (json == nullptr) {
        return false;
    }
    const char *p = json;
    const char *end = json + json_len;
    p = skipWs(p, end);
    if (p >= end || *p != '{') {
        return false;
    }
    p++;
    p = skipWs(p, end);
    if (p < end && *p == '}') {
        p = skipWs(p + 1, end);
        return p == end;
    }
    while (p < end) {
        JsonString key;
        if (!parseString(&p, end, &key) || key.n == 0) {
            return false;
        }
        p = skipWs(p, end);
        if (p >= end || *p != ':') {
            return false;
        }
        p++;
        p = skipWs(p, end);
        if (!on_field(&key, &p, end, ctx)) {
            return false;
        }
        p = skipWs(p, end);
        if (p < end && *p == ',') {
            p++;
            p = skipWs(p, end);
            continue;
        }
        if (p < end && *p == '}') {
            p = skipWs(p + 1, end);
            return p == end;
        }
        return false;
    }
    return false;
}

static bool commandField(const JsonString *key, const char **pp, const char *end, void *ctx) {
    CommandEnvelope *env = (CommandEnvelope *)ctx;
    unsigned bit = 0;
    if (keyEq(key, "v")) {
        bit = KEY_V;
        if (!parseInt64(pp, end, &env->v)) {
            return false;
        }
    } else if (keyEq(key, "tok")) {
        bit = KEY_TOK;
        if (!parseString(pp, end, &env->tok)) {
            return false;
        }
    } else if (keyEq(key, "chord")) {
        bit = KEY_CHORD;
        if (!parseString(pp, end, &env->chord)) {
            return false;
        }
    } else if (keyEq(key, "stroke")) {
        bit = KEY_STROKE;
        if (!parseString(pp, end, &env->stroke)) {
            return false;
        }
    } else if (keyEq(key, "seq")) {
        bit = KEY_SEQ;
        if (!parseInt64(pp, end, &env->seq)) {
            return false;
        }
    } else {
        return false;
    }
    if (env->seen & bit) {
        return false;
    }
    env->seen |= bit;
    return true;
}

static bool tokenField(const JsonString *key, const char **pp, const char *end, void *ctx) {
    TokenPayload *tok = (TokenPayload *)ctx;
    unsigned bit = 0;
    if (keyEq(key, "sid")) {
        bit = KEY_SID;
        if (!parseString(pp, end, &tok->sid)) {
            return false;
        }
    } else if (keyEq(key, "iat")) {
        bit = KEY_IAT;
        if (!parseInt64(pp, end, &tok->iat)) {
            return false;
        }
    } else if (keyEq(key, "exp")) {
        bit = KEY_EXP;
        if (!parseInt64(pp, end, &tok->exp)) {
            return false;
        }
    } else {
        return false;
    }
    if (tok->seen & bit) {
        return false;
    }
    tok->seen |= bit;
    return true;
}

static int b64urlValue(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '-') {
        return 62;
    }
    if (c == '_') {
        return 63;
    }
    return -1;
}

static bool b64urlDecode(const char *in, size_t in_len, unsigned char *out, size_t out_cap,
                         size_t *out_len) {
    if (in == nullptr || out == nullptr || out_len == nullptr) {
        return false;
    }
    while (in_len > 0 && in[in_len - 1] == '=') {
        in_len--;
    }
    if (in_len == 0) {
        *out_len = 0;
        return true;
    }
    size_t produced = 0;
    size_t i = 0;
    while (i < in_len) {
        int v[4] = {-1, -1, -1, -1};
        int got = 0;
        while (got < 4 && i < in_len) {
            const int d = b64urlValue(in[i++]);
            if (d < 0) {
                return false;
            }
            v[got++] = d;
        }
        if (got < 2) {
            return false;
        }
        const unsigned n = ((unsigned)v[0] << 18) | ((unsigned)(v[1] < 0 ? 0 : v[1]) << 12) |
                           ((unsigned)(v[2] < 0 ? 0 : v[2]) << 6) | (unsigned)(v[3] < 0 ? 0 : v[3]);
        const int bytes = got == 2 ? 1 : got == 3 ? 2 : 3;
        for (int b = 0; b < bytes; b++) {
            if (produced >= out_cap) {
                return false;
            }
            out[produced++] = (unsigned char)((n >> (16 - 8 * b)) & 0xff);
        }
    }
    *out_len = produced;
    return true;
}

static bool hmacEqual32(const unsigned char a[32], const unsigned char b[32]) {
    unsigned char diff = 0;
    for (int i = 0; i < 32; i++) {
        diff |= (unsigned char)(a[i] ^ b[i]);
    }
    return diff == 0;
}

static bool isHex32(const JsonString *s) {
    if (s->n != SESSION_SID_LEN) {
        return false;
    }
    for (size_t i = 0; i < SESSION_SID_LEN; i++) {
        const char c = s->p[i];
        const bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!ok) {
            return false;
        }
    }
    return true;
}

static bool verifyToken(const JsonString *tok, SessionCommand *out) {
    if (tok->n == 0 || tok->n > SESSION_TOK_MAX) {
        return false;
    }
    const char *dot = nullptr;
    for (size_t i = 0; i < tok->n; i++) {
        if (tok->p[i] == '.') {
            if (dot != nullptr) {
                return false;
            }
            dot = tok->p + i;
        }
    }
    if (dot == nullptr) {
        return false;
    }
    const char *payload_b64 = tok->p;
    const size_t payload_b64_len = (size_t)(dot - tok->p);
    const char *sig_b64 = dot + 1;
    const size_t sig_b64_len = tok->n - payload_b64_len - 1;
    if (payload_b64_len == 0 || sig_b64_len == 0) {
        return false;
    }

    const char *secret = SESSION_HMAC_SECRET;
    if (!sessionSecretIsUsable(secret)) {
        return false;
    }
    const size_t secret_len = strlen(secret);

    unsigned char expect[32];
    if (!sessionHmacSha256((const unsigned char *)secret, secret_len,
                           (const unsigned char *)payload_b64, payload_b64_len, expect)) {
        return false;
    }

    unsigned char got[32];
    size_t got_len = 0;
    if (!b64urlDecode(sig_b64, sig_b64_len, got, sizeof(got), &got_len) || got_len != 32) {
        return false;
    }
    if (!hmacEqual32(expect, got)) {
        return false;
    }

    unsigned char payload[160];
    size_t payload_len = 0;
    if (!b64urlDecode(payload_b64, payload_b64_len, payload, sizeof(payload) - 1, &payload_len)) {
        return false;
    }
    payload[payload_len] = '\0';

    TokenPayload claims;
    memset(&claims, 0, sizeof(claims));
    if (!parseObject((const char *)payload, payload_len, tokenField, &claims)) {
        return false;
    }
    if (claims.seen != (KEY_SID | KEY_IAT | KEY_EXP)) {
        return false;
    }
    if (!isHex32(&claims.sid) || claims.exp <= claims.iat) {
        return false;
    }

    const time_t now = sessionNow();
    if (now < (time_t)SESSION_TIME_MIN) {
        return false;
    }
    if (now + (time_t)SESSION_CLOCK_SKEW_SEC < (time_t)claims.iat) {
        return false;
    }
    if (now >= (time_t)claims.exp) {
        return false;
    }
    if (!copyCstr(out->sid, sizeof(out->sid), &claims.sid)) {
        return false;
    }
    out->exp = claims.exp;
    return true;
}

static void evictExpiredSlots(time_t now) {
    for (int i = 0; i < SESSION_SID_SLOTS; i++) {
        if (g_slots[i].occupied && now >= (time_t)g_slots[i].exp) {
            memset(&g_slots[i], 0, sizeof(g_slots[i]));
        }
    }
}

static SessionSlot *findSlot(const char *sid) {
    for (int i = 0; i < SESSION_SID_SLOTS; i++) {
        if (g_slots[i].occupied && strcmp(g_slots[i].sid, sid) == 0) {
            return &g_slots[i];
        }
    }
    return nullptr;
}

static SessionSlot *allocSlot(void) {
    for (int i = 0; i < SESSION_SID_SLOTS; i++) {
        if (!g_slots[i].occupied) {
            return &g_slots[i];
        }
    }
    return nullptr;
}

// HMAC covers sid/iat/exp only (locked token format). Command fields are
// authorized as a bearer session. Keep every seen sid until exp, but only the
// most recently committed sid may drive servos, so a previous visitor's token
// cannot be replayed after handover.
static bool seqAllowed(const SessionCommand *cmd) {
    const time_t now = sessionNow();
    evictExpiredSlots(now);
    SessionSlot *slot = findSlot(cmd->sid);
    if (slot == nullptr) {
        return allocSlot() != nullptr;
    }
    if (!slot->current) {
        return false;
    }
    return cmd->seq > slot->last_seq;
}

bool sessionAuthVerify(const char *topic, const char *message, SessionCommand *out) {
    g_have_pending = false;
    if (topic == nullptr || message == nullptr || out == nullptr) {
        return false;
    }
    memset(out, 0, sizeof(*out));

    const bool is_code = strcmp(topic, MQTT_TOPIC_CODE) == 0;
    const bool is_stroke = strcmp(topic, MQTT_TOPIC_STROKE) == 0;
    if (!is_code && !is_stroke) {
        return false;
    }

    CommandEnvelope env;
    memset(&env, 0, sizeof(env));
    if (!parseObject(message, strlen(message), commandField, &env)) {
        return false;
    }

    const unsigned needed =
        is_code ? (KEY_V | KEY_TOK | KEY_CHORD | KEY_SEQ) : (KEY_V | KEY_TOK | KEY_STROKE | KEY_SEQ);
    if (env.seen != needed || env.v != 1) {
        return false;
    }
    if (env.seq < 0 || env.seq > (int64_t)UINT32_MAX) {
        return false;
    }

    SessionCommand cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.seq = (uint32_t)env.seq;
    if (!verifyToken(&env.tok, &cmd)) {
        return false;
    }

    if (is_code) {
        if (!copyCstr(cmd.chord, sizeof(cmd.chord), &env.chord) || cmd.chord[0] == '\0') {
            return false;
        }
        cmd.has_chord = true;
    } else {
        if (!copyCstr(cmd.stroke, sizeof(cmd.stroke), &env.stroke) || cmd.stroke[0] == '\0') {
            return false;
        }
        cmd.has_stroke = true;
    }

    if (!seqAllowed(&cmd)) {
        return false;
    }

    g_pending = cmd;
    g_have_pending = true;
    *out = cmd;
    return true;
}

bool sessionAuthAllowRate(void) {
    const unsigned long now = sessionMillis();
    if (g_rate_count == 0 || now - g_rate_window_start >= (unsigned long)MQTT_CMD_RATE_WINDOW_MS) {
        g_rate_window_start = now;
        g_rate_count = 0;
    }
    if (g_rate_count >= (unsigned)MQTT_CMD_RATE_LIMIT) {
        return false;
    }
    g_rate_count++;
    return true;
}

void sessionAuthCommit(void) {
    if (!g_have_pending) {
        return;
    }
    evictExpiredSlots(sessionNow());
    SessionSlot *slot = findSlot(g_pending.sid);
    if (slot == nullptr) {
        slot = allocSlot();
        if (slot == nullptr) {
            g_have_pending = false;
            return;
        }
        memset(slot, 0, sizeof(*slot));
        memcpy(slot->sid, g_pending.sid, sizeof(slot->sid));
        slot->occupied = true;
    }
    slot->last_seq = g_pending.seq;
    slot->exp = g_pending.exp;
    for (int i = 0; i < SESSION_SID_SLOTS; i++) {
        g_slots[i].current = false;
    }
    slot->current = true;
    g_have_pending = false;
}
