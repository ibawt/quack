#ifndef LOG_H
#define LOG_H

typedef enum {
  DEBUG = 0,
  INFO,
  WARN,
  ERROR
} q_log_level;

void q_log(q_log_level lvl, const char *fmt, ...);

#endif
