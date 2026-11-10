#ifndef SESSION_AUTH_H
#define SESSION_AUTH_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifndef SESSION_CHORD_MAX
#define SESSION_CHORD_MAX 8
#endif
#ifndef SESSION_STROKE_MAX
#define SESSION_STROKE_MAX 2
#endif
#ifndef SESSION_SID_LEN
#define SESSION_SID_LEN 32
#endif
#ifndef SESSION_TOK_MAX
#define SESSION_TOK_MAX 200
#endif
#ifndef SESSION_SID_SLOTS
#define SESSION_SID_SLOTS 32
#endif

struct SessionCommand {
    char chord[SESSION_CHORD_MAX];
    char stroke[SESSION_STROKE_MAX];
    char sid[SESSION_SID_LEN + 1];
    uint32_t seq;
    int64_t exp;
    bool has_chord;
    bool has_stroke;
};

void sessionAuthReset(void);
void sessionAuthSetNow(time_t now);
void sessionAuthSetMillis(unsigned long ms);

bool sessionAuthVerify(const char *topic, const char *message, SessionCommand *out);
bool sessionAuthAllowRate(void);
void sessionAuthCommit(void);

bool sessionHmacSha256(const unsigned char *key, size_t key_len, const unsigned char *data,
                       size_t data_len, unsigned char out[32]);

#endif
