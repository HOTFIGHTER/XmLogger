//
// Created by xmly on 2019-09-21.
//

#include <string>
#include <sys/stat.h>
#include <inttypes.h>
#include "appender.h"
#include "xloggerbase.h"
#include "log_buffer.h"
#include "loginfo_extract.h"

#include <logger/util/mmap_util.h>
#include <logger/util/tickcount.h>
#include <boost/bind.hpp>
#include <boost/filesystem/operations.hpp>
#include <logger/util/time_utils.h>
#include <logger/thread/thread.h>

#define LOG_EXT "txt"

static TAppenderMode sg_mode = kAppednerAsync;
static std::string sg_logdir;
static std::string sg_cache_logdir;
static std::string sg_logfileprefix;
static std::string sg_current_dir;

static uint64_t sg_max_file_size = 0;
static int sg_cache_log_days = 0;
static time_t sg_openfiletime = 0;
static const unsigned int kBufferBlockLength = 150 * 1024;
static const long kMaxLogAliveTime = 10 * 24 * 60 * 60;    // 10 days in second
static const long kMinLogAliveTime = 24 * 60 * 60;    // 1 days in second
static long sg_max_alive_time = kMaxLogAliveTime;
static bool sg_consolelog_open = true;

static Condition &sg_cond_buffer_async = *(new Condition());
static Mutex sg_mutex_buffer_async;
static Mutex sg_mutex_log_file;

static void __async_log_thread();
static Thread sg_thread_async(&__async_log_thread);

static FILE *sg_logfile = NULL;
static LogBuffer *sg_log_buff = NULL;
//内存映射文件句柄
static boost::iostreams::mapped_file &sg_mmmap_file = *(new boost::iostreams::mapped_file);

/*
 * 设置控制台日志是否打开
 * */
void appender_set_console_log(bool _is_open) {
  sg_consolelog_open = _is_open;
}

/*
 * 设置文件最大大小
 * */
void appender_set_max_file_size(uint64_t _max_byte_size) {
  sg_max_file_size = _max_byte_size;
}

void appender_set_max_alive_duration(long _max_time) {
  if (_max_time >= kMinLogAliveTime) {
    sg_max_alive_time = _max_time;
  }
}

/*
 * 向控制台打印日志
 * */
void consoleLog(const XLoggerInfo *_info, const char *_log) {
  if (NULL == _info || NULL == _log) return;
  static const char *levelStrings[] = {
      "V",
      "D",  // debug
      "I",  // info
      "W",  // warn
      "E",  // error
      "F"  // fatal
  };
  char strFuncName[128] = {0};
  ExtractFunctionName(_info->func_name, strFuncName, sizeof(strFuncName));
  const char *file_name = ExtractFileName(_info->filename);
  char log[16 * 1024] = {0};
  snprintf(log,
           sizeof(log),
           "[%s][%s][%s, %s, %d][%s\n",
           levelStrings[_info->level],
           NULL == _info->tag ? "" : _info->tag,
           file_name,
           strFuncName,
           _info->line,
           _log);
  console_log("[%s][%s, %s, %d][%s\n",
              NULL == _info->tag ? "" : _info->tag,
              file_name,
              strFuncName,
              _info->line,
              _log);
  //需要打印到linux Android的控制台
}

static void __writetips2console(const char *_tips_format, ...) {
  if (NULL == _tips_format) {
    return;
  }
  XLoggerInfo info;
  memset(&info, 0, sizeof(XLoggerInfo));
  char tips_info[4096] = {0};
  va_list ap;
  va_start(ap, _tips_format);
  vsnprintf(tips_info, sizeof(tips_info), _tips_format, ap);
  va_end(ap);
  consoleLog(&info, tips_info);
}

//往文件里写数据
static bool __writefile(const void *_data, size_t _len, FILE *_file) {
  if (NULL == _file) {
    assert(false);
    return false;
  }
  long before_len = ftell(_file);
  if (before_len < 0) return false;

  if (1 != fwrite(_data, _len, 1, _file)) {
    int err = ferror(_file);
    __writetips2console("write file error:%d", err);

    ftruncate(fileno(_file), before_len);
    fseek(_file, 0, SEEK_END);

    char err_log[256] = {0};
    snprintf(err_log, sizeof(err_log), "\nwrite file error:%d\n", err);

    AutoBuffer tmp_buff;
    sg_log_buff->Write(err_log, strnlen(err_log, sizeof(err_log)), tmp_buff);

    fwrite(tmp_buff.Ptr(), tmp_buff.Length(), 1, _file);
    return false;
  }
  return true;
}

/*
 * 将y-log前缀和时间拼成一个前缀文件名
 * */
static std::string __make_logfilenameprefix(const timeval &_tv, const char *_prefix) {
  time_t sec = _tv.tv_sec;
  tm tcur = *localtime((const time_t *) &sec);

  char temp[64] = {0};
  snprintf(temp, 64, "_%d%02d%02d", 1900 + tcur.tm_year, 1 + tcur.tm_mon, tcur.tm_mday);

  std::string filenameprefix = _prefix;
  filenameprefix += temp;

  return filenameprefix;
}

/*
 * 通过前缀和后缀拿到所有的文件，_filename_vec记录所有的文件名
 * */
static void __get_filenames_by_prefix(const std::string &_logdir,
                                      const std::string &_fileprefix,
                                      const std::string &_fileext,
                                      std::vector<std::string> &_filename_vec) {
  //获取存在的目录
  DIR *dir = getDirExists(_logdir.c_str());
  if (dir == NULL) {
    return;
  }

  struct dirent *filename;
  while ((filename = readdir(dir)) != NULL) {
    if (isBeginWith(filename->d_name, _fileprefix.c_str()) == 1
        && isEndWith(filename->d_name, _fileext.c_str()) == 1) {
      _filename_vec.push_back(filename->d_name);
    }
  }
  closedir(dir);
}

static bool __string_compare_greater(const std::string &s1, const std::string &s2) {
  if (s1.length() == s2.length()) {
    return s1 > s2;
  }
  return s1.length() > s2.length();
}

static long __get_next_fileindex(const std::string &_fileprefix, const std::string &_fileext) {
  std::vector<std::string> filename_vec;
  __get_filenames_by_prefix(sg_logdir, _fileprefix, _fileext, filename_vec);
  if (!sg_cache_logdir.empty()) {
    __get_filenames_by_prefix(sg_cache_logdir, _fileprefix, _fileext, filename_vec);
  }

  long index = 0; // long is enought to hold all indexes in one day.
  if (filename_vec.empty()) {
    return index;
  }
  // high -> low排序文件名
  std::sort(filename_vec.begin(), filename_vec.end(), __string_compare_greater);
  std::string last_filename = *(filename_vec.begin());
  std::size_t ext_pos = last_filename.rfind("." + _fileext);
  std::size_t index_len = ext_pos - _fileprefix.length();
  if (index_len > 0) {
    std::string index_str = last_filename.substr(_fileprefix.length(), index_len);
    if (isBeginWith(index_str.c_str(), "_") == 1) {
      index_str = index_str.substr(1);
    }
    //把字符窜转换成数字
    index = atol(index_str.c_str());
  }

  uint64_t filesize = 0;
  //得到目标文件名
  std::string logfilepath = sg_logdir + "/" + last_filename;
  if (isFileExists(logfilepath.c_str())) {
    filesize += getFileSize(logfilepath.c_str());
  }
  if (!sg_cache_logdir.empty()) {
    logfilepath = sg_cache_logdir + "/" + last_filename;
    if (isFileExists(logfilepath.c_str())) {
      filesize += getFileSize(logfilepath.c_str());
    }
  }
  return (filesize > sg_max_file_size) ? index + 1 : index;
}

/*
 * 创建logfile文件
 * _tv为时间，_logdir为日志目录，_fileext为文件后缀，
 * _filepath为文件路径，_prefix为前缀
 * 保存到_filepath
 * */
static void __make_logfilename(const timeval &_tv,
                               const std::string &_logdir,
                               const char *_prefix,
                               const std::string &_fileext,
                               char *_filepath,
                               unsigned int _len) {

  long index = 0;
  //创建文件前缀名
  std::string logfilenameprefix = __make_logfilenameprefix(_tv, _prefix);
  //获取放置的文件索引位置
  if (sg_max_file_size > 0) {
    index = __get_next_fileindex(logfilenameprefix, _fileext);
  }

  std::string logfilepath = _logdir;
  logfilepath += "/";
  logfilepath += logfilenameprefix;

  if (index > 0) {
    char temp[24] = {0};
    snprintf(temp, 24, "_%ld", index);
    logfilepath += temp;
  }

  logfilepath += ".";
  logfilepath += _fileext;
  //_filepath就是新的文件名
  strncpy(_filepath, logfilepath.c_str(), _len - 1);
  _filepath[_len - 1] = '\0';
}

static bool __openlogfile(const std::string &_log_dir) {
  if (sg_logdir.empty()) return false;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  //logfile打开着，关闭它
  if (NULL != sg_logfile) {
    time_t sec = tv.tv_sec;
    tm tcur = *localtime((const time_t *) &sec);
    tm filetm = *localtime(&sg_openfiletime);
    //要打开的是同一个文件
    if (filetm.tm_year == tcur.tm_year && filetm.tm_mon == tcur.tm_mon
        && filetm.tm_mday == tcur.tm_mday && sg_current_dir == _log_dir)
      return true;

    fclose(sg_logfile);
    sg_logfile = NULL;
  }

  static time_t s_last_time = 0;
  //static uint64_t s_last_tick = 0;
  static char s_last_file_path[1024] = {0};

  //uint64_t now_tick = gettickcount();
  //uint64_t now_tick = 0;
  time_t now_time = tv.tv_sec;

  sg_openfiletime = tv.tv_sec;
  sg_current_dir = _log_dir;

  char logfilepath[1024] = {0};
  __make_logfilename(tv, _log_dir, sg_logfileprefix.c_str(), LOG_EXT, logfilepath, 1024);

  if (now_time < s_last_time) {
    sg_logfile = fopen(s_last_file_path, "ab");

    if (NULL == sg_logfile) {
      __writetips2console("open file error:%d %s, path:%s",
                          errno,
                          strerror(errno),
                          s_last_file_path);
    }

#ifdef __APPLE__
    assert(sg_logfile);
#endif
    return NULL != sg_logfile;
  }
  //第一次肯定打开logfilepath
  sg_logfile = fopen(logfilepath, "ab");

  if (NULL == sg_logfile) {
    __writetips2console("open file error:%d %s, path:%s", errno, strerror(errno), logfilepath);
  }


//  if (0 != s_last_time
//      && (now_time - s_last_time) > (time_t) ((now_tick - s_last_tick) / 1000 + 300)) {
//
//    struct tm tm_tmp = *localtime((const time_t *) &s_last_time);
//    char last_time_str[64] = {0};
//    strftime(last_time_str, sizeof(last_time_str), "%Y-%m-%d %z %H:%M:%S", &tm_tmp);
//
//    tm_tmp = *localtime((const time_t *) &now_time);
//    char now_time_str[64] = {0};
//    strftime(now_time_str, sizeof(now_time_str), "%Y-%m-%d %z %H:%M:%S", &tm_tmp);
//
//    char log[1024] = {0};
//    snprintf(log,
//             sizeof(log),
//             "[F][ last log file:%s from %s to %s, time_diff:%ld, tick_diff:%"
//             PRIu64
//             "\n",
//             s_last_file_path,
//             last_time_str,
//             now_time_str,
//             now_time - s_last_time,
//             now_tick
//                 - s_last_tick);
//
//    AutoBuffer tmp_buff;
//    sg_log_buff->Write(log, strnlen(log, sizeof(log)), tmp_buff);
//    __writefile(tmp_buff.Ptr(), tmp_buff.Length(), sg_logfile);
//  }
  //将logfilepath赋值给s_last_file_path
  memcpy(s_last_file_path, logfilepath, sizeof(s_last_file_path));
//  s_last_tick = now_tick;
  s_last_time = now_time;

#ifdef __APPLE__
  assert(sg_logfile);
#endif
  return NULL != sg_logfile;
}

static void __closelogfile() {
  if (NULL == sg_logfile) return;

  sg_openfiletime = 0;
  fclose(sg_logfile);
  sg_logfile = NULL;
}

static bool __cache_logs() {
  if (sg_cache_logdir.empty() || sg_cache_log_days <= 0) {
    return false;
  }
  struct timeval tv;
  gettimeofday(&tv, NULL);
  char logfilepath[1024] = {0};
  //创建logfilepath文件名
  __make_logfilename(tv, sg_logdir, sg_logfileprefix.c_str(), LOG_EXT, logfilepath, 1024);
  if (isFileExists(logfilepath)) {
    return false;
  }
  return true;
}

/*
 * 将源src_file转移到目标dst_file
 * */
static bool __append_file(const std::string &_src_file, const std::string &_dst_file) {
  if (_src_file == _dst_file) {
    return false;
  }
  if (!(access(_src_file.c_str(), 0) == 0)) {
    return false;
  }
  if (0 == getFileSize(_src_file.c_str())) {
    return true;
  }
  FILE *src_file = fopen(_src_file.c_str(), "rb");
  if (NULL == src_file) {
    return false;
  }
  FILE *dest_file = fopen(_dst_file.c_str(), "ab");
  if (NULL == dest_file) {
    fclose(src_file);
    return false;
  }
  fseek(src_file, 0, SEEK_END);
  long src_file_len = ftell(src_file);
  long dst_file_len = ftell(dest_file);
  fseek(src_file, 0, SEEK_SET);
  char buffer[4096] = {0};
  while (true) {
    if (feof(src_file)) break;
    size_t read_ret = fread(buffer, 1, sizeof(buffer), src_file);
    if (read_ret == 0) break;
    if (ferror(src_file)) break;
    fwrite(buffer, 1, read_ret, dest_file);
    if (ferror(dest_file)) break;
  }

  if (dst_file_len + src_file_len > ftell(dest_file)) {
    ftruncate(fileno(dest_file), dst_file_len);
    fclose(src_file);
    fclose(dest_file);
    return false;
  }
  fclose(src_file);
  fclose(dest_file);
  return true;
}

static void __log2file(const void *_data, size_t _len, bool _move_file) {
  if (NULL == _data || 0 == _len || sg_logdir.empty()) {
    return;
  }
  ScopedLock lock_file(sg_mutex_log_file);
  //内存映射文件为空直接写入文件，sg_cache_logdir不为空
  if (sg_cache_logdir.empty()) {
    if (__openlogfile(sg_logdir)) {
      __writefile(_data, _len, sg_logfile);
      if (kAppednerAsync == sg_mode) {
        __closelogfile();
      }
    }
    return;
  }

  struct timeval tv;
  gettimeofday(&tv, NULL);
  char logcachefilepath[1024] = {0};
  //新文件名为cache文件名logcachefilepath
  __make_logfilename(tv,
                     sg_cache_logdir,
                     sg_logfileprefix.c_str(),
                     LOG_EXT,
                     logcachefilepath,
                     1024);

  bool cache_logs = __cache_logs();
  console_log("cache_logs:%d\n",cache_logs);
  if ((cache_logs || isFileExists(logcachefilepath))
      && __openlogfile(sg_cache_logdir)) {
    //写入文件,这里的sg_logfile是sg_cache_logdir的logcachefilepath
    __writefile(_data, _len, sg_logfile);
    if (kAppednerAsync == sg_mode) {
      __closelogfile();
    }
    //如果不用往logfilepath里叠加文件
    if (cache_logs || !_move_file) {
      return;
    }

    char logfilepath[1024] = {0};
    //新文件名为目录文件名logfilepath
    __make_logfilename(tv, sg_logdir, sg_logfileprefix.c_str(), LOG_EXT, logfilepath, 1024);
    if (__append_file(logcachefilepath, logfilepath)) {
      if (kAppednerSync == sg_mode) {
        __closelogfile();
      }
      reMove(logcachefilepath);
    }
    return;
  }

  bool write_sucess = false;
  bool open_success = __openlogfile(sg_logdir);
  if (open_success) {
    write_sucess = __writefile(_data, _len, sg_logfile);
    if (kAppednerAsync == sg_mode) {
      __closelogfile();
    }
  }

  if (!write_sucess) {
    if (open_success && kAppednerSync == sg_mode) {
      __closelogfile();
    }

    if (__openlogfile(sg_cache_logdir)) {
      __writefile(_data, _len, sg_logfile);
      if (kAppednerAsync == sg_mode) {
        __closelogfile();
      }
    }
  }
}

/*
 * 删除超时文件
 * */
static void __del_timeout_file(const std::string &_log_path) {
  time_t now_time = time(NULL);
  DIR *dir = getDirExists(_log_path.c_str());
  if (isFileExists(_log_path.c_str()) && dir != NULL) {
    struct dirent *filename;
    while ((filename = readdir(dir)) != NULL) {
      time_t file_modify_time = 0;
      struct stat buf;
      if (stat(filename->d_name, &buf) >= 0 && !S_ISDIR(buf.st_mode)) {
        file_modify_time = buf.st_mtime;
      }
      if (now_time > file_modify_time && now_time - file_modify_time > sg_max_alive_time) {
        //时间大于最大时间，删除
        remove(filename->d_name);
      }
    }
    closedir(dir);
  }
}

/*
 * 移动过时文件
 * */
static void __move_old_files(const std::string &_src_path,
                             const std::string &_dest_path,
                             const std::string &_nameprefix) {
  if (_src_path == _dest_path) {
    return;
  }

  if (!isFileExists(_src_path.c_str())) {
    return;
  }

  ScopedLock lock_file(sg_mutex_log_file);
  time_t now_time = time(NULL);
  DIR *dir = getDirExists(_src_path.c_str());
  if (dir != NULL) {
    struct dirent *filename;
    while ((filename = readdir(dir)) != NULL) {
      if (isBeginWith(filename->d_name, _nameprefix.c_str()) != 1
          || isEndWith(filename->d_name, LOG_EXT) != 1) {
        continue;
      }
      if (sg_cache_log_days > 0) {
        time_t file_modify_time = 0;
        struct stat buf;
        if (stat(filename->d_name, &buf) >= 0 && !S_ISDIR(buf.st_mode)) {
          file_modify_time = buf.st_mtime;
        }
        if (now_time > file_modify_time
            && (now_time - file_modify_time) < sg_cache_log_days * 24 * 60 * 60) {
          continue;
        }
      }
      if (!__append_file(filename->d_name,
                         sg_logdir + "/" + filename->d_name)) {
        break;
      }
      remove(filename->d_name);
    }
    closedir(dir);
  }
}

/*
 * 基于缓存打开appender
 * */
void appender_open_with_cache(TAppenderMode _mode,
                              const std::string &_cachedir,
                              const std::string &_logdir,
                              const char *_nameprefix,
                              int _cache_days,
                              const char *_pub_key) {
  sg_logdir = _logdir;
  sg_cache_log_days = _cache_days;
  //缓存目录不为空
  if (!_cachedir.empty()) {
    sg_cache_logdir = _cachedir;
    //创建缓存目录文件夹
    int ret = mkDir(_cachedir.c_str());
    if (ret != 0) {
      return;
    }
    //执行删除目录操作
    Thread(boost::bind(&__del_timeout_file, _cachedir)).start_after(2 * 60 * 1000);
    Thread(boost::bind(&__move_old_files,
                       _cachedir,
                       _logdir,
                       std::string(_nameprefix))).start_after(3 * 60 * 1000);
  }
  appender_open(_mode, _logdir.c_str(), _nameprefix, _pub_key);
}

//执行真正写入的地方
void xlogger_appender(const XLoggerInfo *_info, const char *_log) {
  if (sg_log_close) return;
  //端口打印日志
  if (sg_consolelog_open) consoleLog(_info, _log);

  if (kAppednerSync == sg_mode) {
    __appender_sync(_info, _log);
  } else {
    console_log("异步写入");
    __appender_async(_info, _log);
  }
}

//打开添加器
void appender_open(TAppenderMode _mode,
                   const char *_dir,
                   const char *_nameprefix,
                   const char *_pub_key) {
  assert(_dir);
  assert(_nameprefix);
  if (!sg_log_close) {
    return;
  }
  //调用xloggerbase.c中__xlogger_SetAppender_impl方法设置appender
  xlogger_SetAppender(&xlogger_appender);
  int ret = mkDir(_dir);
  if (ret != 0) {
    return;
  }
  //tickcount_t tick;
  //tick.gettickcount();
  Thread(boost::bind(&__del_timeout_file, _dir)).start_after(2 * 60 * 1000);

  //tick.gettickcount();

  char mmap_file_path[512] = {0};
  //拷贝sg_cache_logdir的路径到mmap_file_path
  snprintf(mmap_file_path,
           sizeof(mmap_file_path),
           "%s/%s.mmap3",
           sg_cache_logdir.empty() ? _dir : sg_cache_logdir.c_str(),
           _nameprefix);

  bool use_mmap = false;
  //mmap_file_path表示文件路径,sg_mmmap_file为内存映射文件
  if (OpenMmapFile(mmap_file_path, kBufferBlockLength, sg_mmmap_file)) {
    //（sg_log_buff是内存映射文件代理）
    //将文件内存映射地址赋值给LogBuffer的数据域PtrBuffer buff_的parray，也就是这时parray_指向的就是
    // 这个data_指针所指向的地址（磁盘文件的内存映射虚拟地址），以后的读写操作都是对parray_进行的
    sg_log_buff = new LogBuffer(sg_mmmap_file.data(), kBufferBlockLength, false, _pub_key);
    use_mmap = true;
  } else {
    char *buffer = new char[kBufferBlockLength];
    sg_log_buff = new LogBuffer(buffer, kBufferBlockLength, true, _pub_key);
    use_mmap = false;
  }

  if (NULL == sg_log_buff->GetData().Ptr()) {
    if (use_mmap && sg_mmmap_file.is_open()){
      console_log("关闭mmp3文件");
      CloseMmapFile(sg_mmmap_file);
    }
    return;
  }


  AutoBuffer buffer;
  //Flush()函数是将PtrBuffer buff_的parray_（磁盘文件的内存映射虚拟地址）所指向的内容memcpy到传入的AutoBuffer类型里
  sg_log_buff->Flush(buffer);
  //区域锁锁住sg_mutex_log_file
  ScopedLock lock(sg_mutex_log_file);
  sg_logdir = _dir;
  //前缀命名
  sg_logfileprefix = _nameprefix;
  //置sg_log_close状态为false，表明log_append句柄打开
  sg_log_close = false;
  appender_setmode(_mode);
  lock.unlock();
//
//  char mark_info[512] = {0};
//  get_mark_info(mark_info, sizeof(mark_info));
//  //如果buffer数据存在，执行文件保存操作
  if (buffer.Ptr()) {
    __log2file(buffer.Ptr(), buffer.Length(), false);
  }
//
//  tickcountdiff_t get_mmap_time = tickcount_t().gettickcount() - tick;
//
//  BOOT_RUN_EXIT(appender_close);
}

bool get_sgl_og_close(){
  return sg_log_close;
}

void appender_close() {
  if (sg_log_close) return;
  sg_log_close = true;
  sg_cond_buffer_async.notifyAll();
  if (sg_thread_async.isruning()) {
    sg_thread_async.join();
  }

  ScopedLock buffer_lock(sg_mutex_buffer_async);
  if (sg_mmmap_file.is_open()) {
    if (!sg_mmmap_file.operator!()) memset(sg_mmmap_file.data(), 0, kBufferBlockLength);
    CloseMmapFile(sg_mmmap_file);
  } else {
    delete[] (char *) ((sg_log_buff->GetData()).Ptr());
  }

  delete sg_log_buff;
  sg_log_buff = NULL;
  buffer_lock.unlock();

  ScopedLock lock(sg_mutex_log_file);
  __closelogfile();
}

void appender_setmode(TAppenderMode _mode) {
  sg_mode = _mode;

  sg_cond_buffer_async.notifyAll();

  if (kAppednerAsync == sg_mode && !sg_thread_async.isruning()) {
    sg_thread_async.start();
  }
}

//异步线程io拷贝操作
static void __async_log_thread() {
  while (true) {
    ScopedLock lock_buffer(sg_mutex_buffer_async);
    if (NULL == sg_log_buff) break;
    AutoBuffer tmp;
    sg_log_buff->Flush(tmp);
    lock_buffer.unlock();
    if (NULL != tmp.Ptr())  __log2file(tmp.Ptr(), tmp.Length(), true);
    if (sg_log_close) break;
    sg_cond_buffer_async.wait(15 * 60 * 1000);
  }
}

void __appender_sync(const XLoggerInfo *_info, const char *_log) {
  char temp[16 * 1024] = {0};     // tell perry,ray if you want modify size.
  PtrBuffer log(temp, 0, sizeof(temp));
  log_formater(_info, _log, log);

  AutoBuffer tmp_buff;
  //先将log信息复制到tmp_buf中
  if (!sg_log_buff->Write(log.Ptr(), log.Length(), tmp_buff)) return;

  __log2file(tmp_buff.Ptr(), tmp_buff.Length(), false);
}

void __appender_async(const XLoggerInfo *_info, const char *_log) {
  ScopedLock lock(sg_mutex_buffer_async);
  if (NULL == sg_log_buff) return;

  char temp[16 * 1024] = {0};       //tell perry,ray if you want modify size.
  //初始化log_buff
  PtrBuffer log_buff(temp, 0, sizeof(temp));
  //将_info和_log分配到log_buff中pArray_
  log_formater(_info, _log, log_buff);
  //GetData获取buff_
  if (sg_log_buff->GetData().Length() >= kBufferBlockLength * 4 / 5) {
    console_log("sg_log_buff->GetData().Length() >= kBufferBlockLength * 4 / 5");
    int ret = snprintf(temp,
                       sizeof(temp),
                       "[F][ sg_buffer_async.Length() >= BUFFER_BLOCK_LENTH*4/5, len: %d\n",
                       (int) sg_log_buff->GetData().Length());
    //重置log_buff的初始位置
    log_buff.Length(ret, ret);
  }
  //将void* _data数据处理后memcpy给PtrBuffer的磁盘文件内存映射虚拟地址buff_
  if (!sg_log_buff->Write(log_buff.Ptr(), (unsigned int) log_buff.Length())) return;

  if (sg_log_buff->GetData().Length() >= kBufferBlockLength * 1 / 3
      || (NULL != _info && kLevelFatal == _info->level)) {
    console_log("sg_log_buff->GetData().Length() >= kBufferBlockLength * 1 / 3");
    sg_cond_buffer_async.notifyAll();
  }
}

void appender_flush_sync() {
  if (kAppednerSync == sg_mode) {
    return;
  }
  ScopedLock lock_buffer(sg_mutex_buffer_async);
  if (NULL == sg_log_buff) return;
  AutoBuffer tmp;
  sg_log_buff->Flush(tmp);
  lock_buffer.unlock();
  if (tmp.Ptr()) __log2file(tmp.Ptr(), tmp.Length(), false);
}

void appender_flush() {
  sg_cond_buffer_async.notifyAll();
}
