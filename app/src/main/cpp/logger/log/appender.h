//
// Created by xmly on 2019-09-21.
//

#ifndef MEDIA_APPENDER_H
#define MEDIA_APPENDER_H
#include <logger/util/file_operator.h>
#include <logger/util/formater.h>

//存取模式
enum TAppenderMode {
  kAppednerAsync,
  kAppednerSync,
};
static volatile bool sg_log_close = true;

bool get_sgl_og_close();

void appender_set_console_log(bool _is_open);
void
appender_open(TAppenderMode _mode, const char *_dir, const char *_nameprefix, const char *_pub_key);
void appender_close();
void appender_open_with_cache(TAppenderMode _mode,
                              const std::string &_cachedir,
                              const std::string &_logdir,
                              const char *_nameprefix,
                              int _cache_days,
                              const char *_pub_key);
void appender_setmode(TAppenderMode _mode);
void appender_set_max_file_size(uint64_t _max_byte_size);
void appender_set_max_alive_duration(long _max_time);
void __appender_async(const XLoggerInfo *_info, const char *_log);
void __appender_sync(const XLoggerInfo *_info, const char *_log);
void appender_flush_sync();
void appender_flush();

#endif //MEDIA_APPENDER_H
