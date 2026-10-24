#ifndef ARDUINO_H
#define ARDUINO_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#define SDA 6
#define SCL 7

class SerialClass {
public:
    void printf(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
    void println(const char *s) { puts(s); }
    void println() { puts(""); }
    void print(const char *s) { fputs(s, stdout); }
};

extern SerialClass Serial;

#endif
