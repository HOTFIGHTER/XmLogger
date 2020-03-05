//
// Created by xmly on 2019-09-21.
//

#ifndef MEDIA_XLOGGERBASE_H
#define MEDIA_XLOGGERBASE_H

#include <linux/time.h>
#include <zconf.h>
#include <pthread.h>
/*
 * log级别等级枚举
 * */
typedef enum {
  kLevelAll = 0,
  kLevelVerbose = 0,
  kLevelDebug,    // Detailed information on the flow through the system.
  kLevelInfo,     // Interesting runtime events (startup/shutdown), should be conservative and keep to a minimum.
  kLevelWarn,     // Other runtime situations that are undesirable or unexpected, but not necessarily "wrong".
  kLevelError,    // Other runtime errors or unexpected conditions.
  kLevelFatal,    // Severe errors that cause premature termination.
  kLevelNone,     // Special level used to disable all log messages.
} TLogLevel;

typedef struct XLoggerInfo_t {
  TLogLevel level;
  const char *tag;
  const char *filename;
  const char *func_name;
  int line;

  timeval timeval;
  int64_t pid;
  int64_t tid;
  int64_t maintid;
} XLoggerInfo;

extern long xlogger_pid();
extern long xlogger_tid();
extern long xlogger_maintid();

typedef void (*xlogger_appender_t)(const XLoggerInfo *_info, const char *_log);

TLogLevel xlogger_Level();
void xlogger_SetLevel(TLogLevel _level);
int xlogger_IsEnabledFor(TLogLevel _level);
xlogger_appender_t xlogger_SetAppender(xlogger_appender_t _appender);

void xlogger_Print(const XLoggerInfo *_info, const char *_format, ...);
void xlogger_Write(const XLoggerInfo *_info, const char *_log);

#endif //MEDIA_XLOGGERBASE_H
