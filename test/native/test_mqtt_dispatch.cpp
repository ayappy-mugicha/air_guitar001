#include "Arduino.h"
#include "config.h"
#include "mqtt_dispatch.h"
#include "pca9685.h"
#include "pca9685_stub.h"
#include "session_auth.h"
#include "stroke.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

SerialClass Serial;

static int g_failures = 0;

static const char kSid[] = "0123456789abcdef0123456789abcdef";
static const time_t kNow = 1731200000;
static const time_t kIat = 1731200000;
static const time_t kExp = 1731200600;

static void expect_true(bool cond, const char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        g_failures++;
    }
}

static void b64urlEncode(const unsigned char *data, size_t len, char *out) {
    static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    size_t o = 0;
    size_t i = 0;
    while (i + 3 <= len) {
        const uint32_t n =
            ((uint32_t)data[i] << 16) | ((uint32_t)data[i + 1] << 8) | (uint32_t)data[i + 2];
        out[o++] = tbl[(n >> 18) & 63];
        out[o++] = tbl[(n >> 12) & 63];
        out[o++] = tbl[(n >> 6) & 63];
        out[o++] = tbl[n & 63];
        i += 3;
    }
    if (i < len) {
        uint32_t n = (uint32_t)data[i] << 16;
        if (i + 1 < len) {
            n |= (uint32_t)data[i + 1] << 8;
        }
        out[o++] = tbl[(n >> 18) & 63];
        out[o++] = tbl[(n >> 12) & 63];
        if (i + 1 < len) {
            out[o++] = tbl[(n >> 6) & 63];
        }
    }
    out[o] = '\0';
}

static void makeToken(char *out, size_t out_len, const char *sid, int64_t iat, int64_t exp) {
    char payload[96];
    snprintf(payload, sizeof(payload), "{\"sid\":\"%s\",\"iat\":%lld,\"exp\":%lld}", sid,
             (long long)iat, (long long)exp);
    char payload_b64[160];
    b64urlEncode((const unsigned char *)payload, strlen(payload), payload_b64);
    unsigned char sig[32];
    sessionHmacSha256((const unsigned char *)SESSION_HMAC_SECRET, strlen(SESSION_HMAC_SECRET),
                      (const unsigned char *)payload_b64, strlen(payload_b64), sig);
    char sig_b64[64];
    b64urlEncode(sig, 32, sig_b64);
    snprintf(out, out_len, "%s.%s", payload_b64, sig_b64);
}

static void makeCode(char *out, size_t out_len, const char *tok, const char *chord, uint32_t seq) {
    snprintf(out, out_len, "{\"v\":1,\"tok\":\"%s\",\"chord\":\"%s\",\"seq\":%u}", tok, chord, seq);
}

static void makeStroke(char *out, size_t out_len, const char *tok, const char *stroke, uint32_t seq) {
    snprintf(out, out_len, "{\"v\":1,\"tok\":\"%s\",\"stroke\":\"%s\",\"seq\":%u}", tok, stroke,
             seq);
}

static void beginCase(void) {
    sessionAuthReset();
    sessionAuthSetNow(kNow);
    sessionAuthSetMillis(0);
    pca9685ResetLog();
}

static void test_trim_and_copy() {
    char buf[32];
    const unsigned char payload[] = {'c', '\r', '\n'};
    expect_true(mqttCopyPayload(buf, sizeof(buf), payload, 3), "copy short chord payload");
    mqttTrimPayload(buf);
    expect_true(strcmp(buf, "c") == 0, "trim CR/LF from chord payload");

    const unsigned char stroke[] = {'1', ' '};
    expect_true(mqttCopyPayload(buf, sizeof(buf), stroke, 2), "copy short stroke payload");
    mqttTrimPayload(buf);
    expect_true(strcmp(buf, "1") == 0, "trim trailing space from stroke payload");
}

static void test_copy_keeps_json_sized_payload() {
    unsigned char payload[200];
    memset(payload, 'A', sizeof(payload));
    payload[0] = '{';
    payload[199] = '}';
    char buf[MQTT_CMD_PAYLOAD_LEN];
    expect_true(mqttCopyPayload(buf, sizeof(buf), payload, sizeof(payload)),
                "copy 200-byte payload into 256-byte buffer");
    expect_true(strlen(buf) == sizeof(payload), "200-byte payload is not truncated");
    expect_true(buf[0] == '{' && buf[199] == '}', "copied payload boundaries");
}

static void test_copy_rejects_oversize() {
    unsigned char payload[MQTT_CMD_PAYLOAD_LEN];
    memset(payload, 'B', sizeof(payload));
    char buf[MQTT_CMD_PAYLOAD_LEN];
    expect_true(!mqttCopyPayload(buf, sizeof(buf), payload, sizeof(payload)),
                "length == dest_len is rejected");
    expect_true(buf[0] == '\0', "oversize copy leaves dest empty");

    unsigned char almost[MQTT_CMD_PAYLOAD_LEN - 1];
    memset(almost, 'C', sizeof(almost));
    expect_true(mqttCopyPayload(buf, sizeof(buf), almost, sizeof(almost)),
                "length == dest_len-1 fits with NUL");
    expect_true(strlen(buf) == sizeof(almost), "max fitting payload keeps all bytes");
}

static void test_unknown_topic_does_not_drive_pwm() {
    beginCase();
    char tok[192];
    char cmd[256];
    makeToken(tok, sizeof(tok), kSid, kIat, kExp);
    makeCode(cmd, sizeof(cmd), tok, "c", 1);
    expect_true(!mqttDispatch("guitar/other", cmd), "unknown topic rejected");
    expect_true(g_pwm_pulses.empty(), "unknown topic writes no PWM");
}

static void test_python_known_token_vector() {
    beginCase();
    const char tok[] =
        "eyJzaWQiOiIwMTIzNDU2Nzg5YWJjZGVmMDEyMzQ1Njc4OWFiY2RlZiIsImlhdCI6MTczMTIwMDAwMCwiZXhwIjoxNzMx"
        "MjAwNjAwfQ.fwdm6J0CLwSXW1ownhleAVM32a-s9sfjiSUZRVxsD6M";
    char cmd[256];
    makeCode(cmd, sizeof(cmd), tok, "c", 42);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "python HMAC vector is accepted");
    expect_true(!g_pwm_angles.empty(), "known-good token drives servos");
}

static void test_valid_chords_and_strokes() {
    beginCase();
    char tok[192];
    char cmd[256];
    makeToken(tok, sizeof(tok), kSid, kIat, kExp);

    makeCode(cmd, sizeof(cmd), tok, "c", 1);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "dispatch chord c");
    const size_t after_c = g_pwm_angles.size();
    expect_true(after_c >= 2, "chord c writes at least two channels");

    makeCode(cmd, sizeof(cmd), tok, "f", 2);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "dispatch chord f");
    expect_true(g_pwm_angles.size() > after_c, "different chord still writes PWM");

    pca9685ResetLog();
    makeStroke(cmd, sizeof(cmd), tok, "0", 3);
    expect_true(mqttDispatch(MQTT_TOPIC_STROKE, cmd), "stroke down");
    makeStroke(cmd, sizeof(cmd), tok, "1", 4);
    expect_true(mqttDispatch(MQTT_TOPIC_STROKE, cmd), "stroke up");
    expect_true(g_pwm_pulses.size() == 2, "up and down each write pulse");
    expect_true(g_pwm_pulses[0].channel == STROKE_CHANNEL, "down uses stroke channel");
    expect_true(g_pwm_pulses[1].channel == STROKE_CHANNEL, "up uses stroke channel");
    expect_true(g_pwm_pulses[0].pulse_us == STROKE_SERVO_MAX_US, "down pulse");
    expect_true(g_pwm_pulses[1].pulse_us == STROKE_SERVO_MIN_US, "up pulse");
}

static void test_increasing_seq_replays_same_chord() {
    beginCase();
    char tok[192];
    char cmd[256];
    makeToken(tok, sizeof(tok), kSid, kIat, kExp);
    makeCode(cmd, sizeof(cmd), tok, "c", 1);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "first c seq 1");
    const size_t after_first = g_pwm_pulses.size();
    makeCode(cmd, sizeof(cmd), tok, "c", 2);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "second c seq 2");
    expect_true(g_pwm_pulses.size() > after_first, "increasing seq still writes PWM");
}

static void test_plain_and_missing_token_rejected() {
    beginCase();
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, "c"), "plain chord name rejected");
    expect_true(!mqttDispatch(MQTT_TOPIC_STROKE, "0"), "plain stroke rejected");
    expect_true(g_pwm_pulses.empty(), "legacy plaintext writes no PWM");

    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, "{\"v\":1,\"chord\":\"c\",\"seq\":1}"),
                "JSON without tok rejected");
}

static void test_bad_signature_and_expiry_rejected() {
    beginCase();
    char tok[192];
    char cmd[256];
    makeToken(tok, sizeof(tok), kSid, kIat, kExp);
    if (tok[0] == 'A') {
        tok[0] = 'B';
    } else {
        tok[0] = 'A';
    }
    makeCode(cmd, sizeof(cmd), tok, "c", 1);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "tampered token rejected");
    expect_true(g_pwm_pulses.empty(), "tampered token writes no PWM");

    beginCase();
    makeToken(tok, sizeof(tok), kSid, kIat - 700, kIat - 100);
    makeCode(cmd, sizeof(cmd), tok, "c", 1);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "expired token rejected");
    expect_true(g_pwm_pulses.empty(), "expired token writes no PWM");
}

static void test_seq_replay_and_rewind_rejected() {
    beginCase();
    char tok[192];
    char cmd[256];
    makeToken(tok, sizeof(tok), kSid, kIat, kExp);
    makeCode(cmd, sizeof(cmd), tok, "c", 5);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "seq 5 accepted");
    pca9685ResetLog();
    makeCode(cmd, sizeof(cmd), tok, "f", 5);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "same seq replay rejected");
    makeCode(cmd, sizeof(cmd), tok, "f", 4);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "rewinding seq rejected");
    expect_true(g_pwm_pulses.empty(), "stale seq writes no PWM");
}

static void test_sid_change_resets_seq() {
    beginCase();
    char tok[192];
    char cmd[256];
    makeToken(tok, sizeof(tok), kSid, kIat, kExp);
    makeCode(cmd, sizeof(cmd), tok, "c", 9);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "first sid seq 9");

    const char sid2[] = "ffffffffffffffffffffffffffffffff";
    makeToken(tok, sizeof(tok), sid2, kIat, kExp);
    makeCode(cmd, sizeof(cmd), tok, "f", 1);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "new sid resets seq");
}

static void test_previous_sid_cannot_replay_after_handover() {
    beginCase();
    char tok_a[192];
    char tok_b[192];
    char cmd[256];
    const char sid2[] = "ffffffffffffffffffffffffffffffff";
    makeToken(tok_a, sizeof(tok_a), kSid, kIat, kExp);
    makeToken(tok_b, sizeof(tok_b), sid2, kIat, kExp);

    makeCode(cmd, sizeof(cmd), tok_a, "c", 5);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "visitor A seq 5");
    makeCode(cmd, sizeof(cmd), tok_b, "f", 1);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "visitor B takes over");

    pca9685ResetLog();
    makeCode(cmd, sizeof(cmd), tok_a, "c", 5);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "A's consumed seq rejected after B");
    makeCode(cmd, sizeof(cmd), tok_a, "g", 6);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "A's leftover token cannot send new seq after B");
    expect_true(g_pwm_pulses.empty(), "previous sid writes no PWM after handover");

    makeCode(cmd, sizeof(cmd), tok_b, "g", 2);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "active sid still advances seq");
}

static void test_expired_sid_slot_is_freed() {
    beginCase();
    char tok[192];
    char cmd[256];
    makeToken(tok, sizeof(tok), kSid, kIat, kExp);
    makeCode(cmd, sizeof(cmd), tok, "c", 1);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "A occupies a slot");

    sessionAuthSetNow(kExp);
    const char sid2[] = "ffffffffffffffffffffffffffffffff";
    makeToken(tok, sizeof(tok), sid2, kExp, kExp + 600);
    makeCode(cmd, sizeof(cmd), tok, "f", 1);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "new sid after previous token expired");

    char tok_a[192];
    makeToken(tok_a, sizeof(tok_a), kSid, kIat, kExp);
    pca9685ResetLog();
    makeCode(cmd, sizeof(cmd), tok_a, "c", 2);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "expired A token still rejected");
    expect_true(g_pwm_pulses.empty(), "expired token writes no PWM");
}

static void test_session_slot_limit() {
    beginCase();
    char sid[SESSION_SID_LEN + 1];
    char tok[192];
    char cmd[256];
    for (unsigned i = 0; i < (unsigned)SESSION_SID_SLOTS; i++) {
        sessionAuthSetMillis(i * (unsigned)MQTT_CMD_RATE_WINDOW_MS);
        snprintf(sid, sizeof(sid), "%032x", i + 1);
        makeToken(tok, sizeof(tok), sid, kIat, kExp);
        makeCode(cmd, sizeof(cmd), tok, "c", 1);
        expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "fill session slot");
    }
    pca9685ResetLog();
    sessionAuthSetMillis((unsigned)SESSION_SID_SLOTS * (unsigned)MQTT_CMD_RATE_WINDOW_MS);
    snprintf(sid, sizeof(sid), "%032x", (unsigned)SESSION_SID_SLOTS + 1);
    makeToken(tok, sizeof(tok), sid, kIat, kExp);
    makeCode(cmd, sizeof(cmd), tok, "c", 1);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "no slot for extra sid");
    expect_true(g_pwm_pulses.empty(), "overflow sid writes no PWM");
}

static void test_unknown_chord_rejected() {
    beginCase();
    char tok[192];
    char cmd[256];
    makeToken(tok, sizeof(tok), kSid, kIat, kExp);
    makeCode(cmd, sizeof(cmd), tok, "h", 1);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "unknown chord rejected");
    expect_true(g_pwm_pulses.empty(), "unknown chord writes no PWM");
}

static void test_malformed_json_rejected() {
    beginCase();
    char tok[192];
    char cmd[320];
    makeToken(tok, sizeof(tok), kSid, kIat, kExp);

    snprintf(cmd, sizeof(cmd), "{\"v\":1,\"tok\":\"%s\",\"chord\":\"c\",\"seq\":1,\"extra\":true}",
             tok);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "unknown field rejected");

    snprintf(cmd, sizeof(cmd), "{\"v\":2,\"tok\":\"%s\",\"chord\":\"c\",\"seq\":1}", tok);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "wrong version rejected");

    snprintf(cmd, sizeof(cmd), "{\"v\":1,\"tok\":\"%s\",\"chord\":1,\"seq\":1}", tok);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "chord number rejected");

    snprintf(cmd, sizeof(cmd), "{\"v\":1,\"tok\":\"%s\",\"chord\":\"c\",\"seq\":1.5}", tok);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "float seq rejected");

    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, "{not json"), "broken JSON rejected");
    expect_true(g_pwm_pulses.empty(), "malformed JSON writes no PWM");
}

static void test_unsynced_clock_fail_closed() {
    sessionAuthReset();
    sessionAuthSetNow(0);
    sessionAuthSetMillis(0);
    pca9685ResetLog();
    char tok[192];
    char cmd[256];
    makeToken(tok, sizeof(tok), kSid, kIat, kExp);
    makeCode(cmd, sizeof(cmd), tok, "c", 1);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "commands rejected before NTP sync");
    expect_true(g_pwm_pulses.empty(), "unsynced clock writes no PWM");
}

static void test_rate_limit() {
    beginCase();
    char tok[192];
    char cmd[256];
    makeToken(tok, sizeof(tok), kSid, kIat, kExp);
    for (unsigned i = 1; i <= (unsigned)MQTT_CMD_RATE_LIMIT; i++) {
        makeCode(cmd, sizeof(cmd), tok, "c", i);
        expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "command inside rate window");
    }
    pca9685ResetLog();
    makeCode(cmd, sizeof(cmd), tok, "c", MQTT_CMD_RATE_LIMIT + 1);
    expect_true(!mqttDispatch(MQTT_TOPIC_CODE, cmd), "rate-limited command rejected");
    expect_true(g_pwm_pulses.empty(), "rate-limited command writes no PWM");

    sessionAuthSetMillis(MQTT_CMD_RATE_WINDOW_MS);
    makeCode(cmd, sizeof(cmd), tok, "c", MQTT_CMD_RATE_LIMIT + 1);
    expect_true(mqttDispatch(MQTT_TOPIC_CODE, cmd), "command accepted in next window");
}

static void test_chord_angle_map_matches_pi_equivalent_pulses() {
    expect_true(
        pca9685Map(0, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, CHORD_SERVO_MIN_US, CHORD_SERVO_MAX_US) ==
            CHORD_SERVO_MIN_US,
        "angle 0 maps to CHORD_SERVO_MIN_US");
    expect_true(
        pca9685Map(180, SERVO_ANGLE_MIN, SERVO_ANGLE_MAX, CHORD_SERVO_MIN_US, CHORD_SERVO_MAX_US) ==
            CHORD_SERVO_MAX_US,
        "angle 180 maps to CHORD_SERVO_MAX_US");
    expect_true(CHORD_SERVO_MIN_US == 381, "Pi-equivalent chord min us");
    expect_true(CHORD_SERVO_MAX_US == 2543, "Pi-equivalent chord max us");
    expect_true(STROKE_SERVO_MIN_US == 381, "Pi-equivalent stroke min us");
    expect_true(STROKE_SERVO_MAX_US == 966, "Pi-equivalent stroke max us");
}

int main() {
    test_trim_and_copy();
    test_copy_keeps_json_sized_payload();
    test_copy_rejects_oversize();
    test_unknown_topic_does_not_drive_pwm();
    test_python_known_token_vector();
    test_valid_chords_and_strokes();
    test_increasing_seq_replays_same_chord();
    test_plain_and_missing_token_rejected();
    test_bad_signature_and_expiry_rejected();
    test_seq_replay_and_rewind_rejected();
    test_sid_change_resets_seq();
    test_previous_sid_cannot_replay_after_handover();
    test_expired_sid_slot_is_freed();
    test_session_slot_limit();
    test_unknown_chord_rejected();
    test_malformed_json_rejected();
    test_unsynced_clock_fail_closed();
    test_rate_limit();
    test_chord_angle_map_matches_pi_equivalent_pulses();

    if (g_failures != 0) {
        printf("%d test(s) failed\n", g_failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
