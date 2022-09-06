//  vq_logger - v0.0.1
//    Do this:
//       #define VQ_LOGGER_IMPLEMENTATION
//    before you include this file in *only one* C or C++ file to create the
//    implementation.
//
//    // i.e. it should look like this:
//    #include ...
//    #include ...
//    #include ...
//    #define VQ_LOGGER_IMPLEMENTATION
//    #include "log.h"

#ifndef VQ_LOGGER_H
#define VQ_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VQ_DEF_TYPE
#define VQ_DEF_TYPE extern
#endif // VQ_DEF_TYPE

typedef struct logger logger_t;

typedef struct log_event log_event_t;

typedef enum log_level {
  LOG_TRACE = 0,
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARN,
  LOG_ERROR,
  LOG_FATAL,
  LOG_DISABLED
} log_level_t;

#define VQ_LOG_TRACE(...) logger_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define VQ_LOG_DEBUG(...) logger_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define VQ_LOG_INFO(...) logger_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define VQ_LOG_WARN(...) logger_log(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define VQ_LOG_ERROR(...) logger_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define VQ_LOG_FATAL(...) logger_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

VQ_DEF_TYPE const char *logger_level_to_string(int level);
VQ_DEF_TYPE void logger_set_level(log_level_t level);
VQ_DEF_TYPE log_level_t logger_get_level();
VQ_DEF_TYPE void logger_log(log_level_t level, const char *file_name, int line,
                            const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#ifdef VQ_LOGGER_IMPLEMENTATION

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

typedef struct logger {
  void *stream;
  int level;
} logger_t;

typedef struct log_event {
  log_level_t level;
  int line;
  const char *fmt;
  const char *file_name;
  struct tm *time;
  void *stream;
  va_list arg;
} log_event_t;

static logger_t s_logger;
static pthread_mutex_t s_logger_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *level_strings[] = {"TRACE", "DEBUG", "INFO",
                                      "WARN",  "ERROR", "FATAL"};

#ifdef VQ_LOGGER_USE_COLOR

#define ANSI_BLACK "\x1b[30m"
#define ANSI_WHITE "\x1b[97m"

#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_ORANGE "\x1b[33m"
#define ANSI_BLUE "\x1b[94m"
#define ANSI_GRAY "\x1b[90m"

#define ANSI_BOLD_RED "\x1b[1;31m"

#define ANSI_RESET "\x1b[0m"

static const char *logger_level_colors[] = {
    ANSI_WHITE, ANSI_BLUE, ANSI_GREEN, ANSI_ORANGE, ANSI_RED, ANSI_BOLD_RED};
#endif

static void print_to_stdout(log_event_t *e) {
  char buf[16];
  size_t time_stamp_len = strftime(buf, sizeof(buf), "%H:%M:%S", e->time);
  buf[time_stamp_len] = '\0';

#ifdef VQ_LOGGER_USE_COLOR
  fprintf(e->stream, "%s %s%-5s" ANSI_RESET ANSI_GRAY " %s:%d " ANSI_RESET, buf,
          logger_level_colors[e->level], level_strings[e->level], e->file_name,
          e->line);
#else
  fprintf(e->stream, "%s %-5s %s:%d ", buf, level_strings[e->level],
          e->file_name, e->line);
#endif

  vfprintf(e->stream, e->fmt, e->arg);
  fprintf(e->stream, "\n");
  fflush(e->stream);
}

static void lock(void) { pthread_mutex_lock(&s_logger_mutex); }

static void unlock(void) { pthread_mutex_unlock(&s_logger_mutex); }

static void setup_event(log_event_t *e, void *stream) {
  if (!e->time) {
    time_t t = time(NULL);
    e->time = localtime(&t);
  }
  e->stream = stream;
}

const char *logger_level_to_string(int level) { return level_strings[level]; }

void logger_set_level(log_level_t level) { s_logger.level = level; }

log_level_t logger_get_level() { return s_logger.level; }

void logger_log(log_level_t level, const char *file_name, int line,
                const char *fmt, ...) {
  log_event_t e = {
      .level = level,
      .line = line,
      .fmt = fmt,
      .file_name = file_name,
  };

  lock();

  if (level >= s_logger.level) {
    setup_event(&e, stdout);
    va_start(e.arg, fmt);
    print_to_stdout(&e);
    va_end(e.arg);
  }

  unlock();
}

#endif // VQ_LOGGER_IMPLEMENTATION

#endif // VQ_LOGGER_H