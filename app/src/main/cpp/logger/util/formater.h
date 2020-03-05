//
// Created by xmly on 2019-09-25.
//

#ifndef MEDIA_FORMATER_H
#define MEDIA_FORMATER_H
#include <android/log.h>
#ifndef LOG_TAG
#define  LOG_TAG  "YuCollect"
#endif
//Android终端输出日志Debug级别，Tag名YuCollect
#define console_log(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <algorithm>

#include "logger/log/xloggerbase.h"
#include "logger/log/loginfo_extract.h"
#include "logger/util/ptrbuffer.h"
void log_formater(const XLoggerInfo *_info, const char *_logbody, PtrBuffer &_log);
#endif //MEDIA_FORMATER_H
