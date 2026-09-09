// Internal logging helper around the ntre_hr_log_fn callback.
#ifndef NTRE_HR_LOG_H
#define NTRE_HR_LOG_H

#include "ntre_hr.h"

#include <cstdarg>
#include <cstdio>

namespace hr {

struct Logger {
    ntre_hr_log_fn fn = nullptr;
    void* user = nullptr;
    bool verbose = false;

    void log(ntre_hr_log_level level, const char* fmt, ...) const __attribute__((format(printf, 3, 4))) {
        if (level == NTRE_HR_LOG_DEBUG && !verbose) return;
        char buf[2048];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf, fmt, ap);
        va_end(ap);
        if (fn) {
            fn(user, level, buf);
        } else {
            static const char* const names[] = {"debug", "info", "warn", "error"};
            fprintf(stderr, "[ntre_hr %s] %s\n", names[level & 3], buf);
        }
    }

    void debug(const char* fmt, ...) const __attribute__((format(printf, 2, 3))) {
        if (!verbose) return;
        char buf[2048];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf, fmt, ap);
        va_end(ap);
        log(NTRE_HR_LOG_DEBUG, "%s", buf);
    }
    void info(const char* fmt, ...) const __attribute__((format(printf, 2, 3))) {
        char buf[2048];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf, fmt, ap);
        va_end(ap);
        log(NTRE_HR_LOG_INFO, "%s", buf);
    }
    void warn(const char* fmt, ...) const __attribute__((format(printf, 2, 3))) {
        char buf[2048];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf, fmt, ap);
        va_end(ap);
        log(NTRE_HR_LOG_WARN, "%s", buf);
    }
    void error(const char* fmt, ...) const __attribute__((format(printf, 2, 3))) {
        char buf[2048];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof buf, fmt, ap);
        va_end(ap);
        log(NTRE_HR_LOG_ERROR, "%s", buf);
    }
};

} // namespace hr

#endif
