//
// Created by xmly on 2019-09-21.
//
#include <jni.h>
#include <vector>
#include <string>
#include <logger/util/scoped_jstring.h>
#include <logger/log/xloggerbase.h>
#include <logger/log/appender.h>

#define LONGTHREADID2INT(a) ((a >> 32)^((a & 0xFFFF)))

//要加导出c的标记，不然连接不能识别
extern "C" {
jvalue JNU_GetField(JNIEnv *_env, jobject obj, const char *_name, const char *sig) {
    jvalue result;
    memset(&result, 0, sizeof(result));

    if (_env->ExceptionOccurred()) {
        return result;
    }

    assert(NULL != _env);
    assert(NULL != obj);
    assert(NULL != _name);
    assert(NULL != sig);

    jclass _clazz = _env->GetObjectClass(obj);
    jfieldID fid = _env->GetFieldID(_clazz, _name, sig);
    _env->DeleteLocalRef(_clazz);

    if (NULL == fid) {
        return result;
    }

    switch (*sig) {
        case '[':
        case 'L':
            result.l = _env->GetObjectField(obj, fid);
            break;

        case 'Z':
            result.z = _env->GetBooleanField(obj, fid);
            break;

        case 'B':
            result.b = _env->GetByteField(obj, fid);
            break;

        case 'C':
            result.c = _env->GetCharField(obj, fid);
            break;

        case 'S':
            result.s = _env->GetShortField(obj, fid);
            break;

        case 'I':
            result.i = _env->GetIntField(obj, fid);
            break;

        case 'J':
            result.j = _env->GetLongField(obj, fid);
            break;

        case 'F':
            result.f = _env->GetFloatField(obj, fid);
            break;

        case 'D':
            result.d = _env->GetDoubleField(obj, fid);
            break;

        default:
            _env->FatalError("illegal _descriptor");
            break;
    }
    return result;
}


/*
 * c层打开logger句柄
 * */
void logAppenderOpen(int level, int mode, const char * _cache_dir, const char * _log_dir,
                     const char * _nameprefix, int _cache_log_days) {
    appender_open_with_cache((TAppenderMode) mode,
                             _cache_dir,
                             _log_dir,
                             _nameprefix,
                             _cache_log_days,
                             "");
    xlogger_SetLevel((TLogLevel) level);
}

/*
 * 打开logger句柄
 **/
JNIEXPORT void JNICALL
Java_com_ximalaya_ting_android_xlogger_XLoggerJNI_appenderOpen(JNIEnv *env,
                                                               jobject,
                                                               jint level,
                                                               jint mode,
                                                               jstring _cache_dir,
                                                               jstring _log_dir,
                                                               jstring _nameprefix,
                                                               jint _cache_log_days) {
    if (NULL == _log_dir || NULL == _nameprefix) {
        return;
    }
    std::string cache_dir;
    if (NULL != _cache_dir) {
        ScopedJstring cache_dir_jstr(env, _cache_dir);
        cache_dir = cache_dir_jstr.GetChar();
    }
    ScopedJstring log_dir_jstr(env, _log_dir);
    //前缀字符窜
    ScopedJstring nameprefix_jstr(env, _nameprefix);
    logAppenderOpen(level, mode, cache_dir.c_str(), log_dir_jstr.GetChar(),
                    nameprefix_jstr.GetChar(), _cache_log_days);
}

/*
 * 关闭logger句柄
 * */
JNIEXPORT void JNICALL
Java_com_ximalaya_ting_android_xlogger_XLoggerJNI_appenderClose(JNIEnv *env, jobject) {
    appender_close();
}

void
logWrite(XLoggerInfo& xlog_info_common, const char *tag, const char *filename, const char *funcname,
         const char *log_jst,
         bool isFromC) {
    if (get_sgl_og_close()) {
        return;
    }
    if (isFromC) {
        xlog_info_common.level = kLevelDebug;
        xlog_info_common.pid = 0;
        xlog_info_common.tid = LONGTHREADID2INT(2);
        xlog_info_common.maintid = LONGTHREADID2INT(2);
    }
    xlog_info_common.tag = tag;
    //设置xlog_info信息
    xlog_info_common.filename = filename;
    //设置xlog_info的函数名
    xlog_info_common.func_name = funcname;
    //执行xlog写入操作，log_jst.GetChar(）为日志内容
    xlogger_Write(&xlog_info_common, log_jst);
}

/*
 * c层写入logger句柄
 * */
void logWriteWithoutInfo(const char *funcname, const char *log_jst) {
    XLoggerInfo xlog_info_common;
    gettimeofday(&xlog_info_common.timeval, NULL);
    logWrite(xlog_info_common,"yucollect","yu",funcname,log_jst, true);
}

/*
 * 写入logger句柄
 * */
JNIEXPORT void JNICALL Java_com_ximalaya_ting_android_xlogger_XLoggerJNI_logWrite
        (JNIEnv *env, jobject, jobject _log_info, jstring _log) {
    if (NULL == _log_info || NULL == _log) {
        //要加日志输出
        return;
    }

    jint level = JNU_GetField(env, _log_info, "level", "I").i;

    if (!xlogger_IsEnabledFor((TLogLevel) level)) {
        return;
    }

    jstring tag = (jstring) JNU_GetField(env, _log_info, "tag", "Ljava/lang/String;").l;
    jstring filename = (jstring) JNU_GetField(env, _log_info, "filename", "Ljava/lang/String;").l;
    jstring funcname = (jstring) JNU_GetField(env, _log_info, "funcname", "Ljava/lang/String;").l;
    jint line = JNU_GetField(env, _log_info, "line", "I").i;
    jlong pid = JNU_GetField(env, _log_info, "pid", "J").i;
    jlong tid = JNU_GetField(env, _log_info, "tid", "J").j;
    jlong maintid = JNU_GetField(env, _log_info, "maintid", "J").j;

    XLoggerInfo xlog_info_common;
    gettimeofday(&xlog_info_common.timeval, NULL);
    xlog_info_common.level = (TLogLevel) level;
    xlog_info_common.line = line;
    xlog_info_common.pid = (int) pid;
    xlog_info_common.tid = LONGTHREADID2INT(tid);
    xlog_info_common.maintid = LONGTHREADID2INT(maintid);

    ScopedJstring tag_jstr(env, tag);
    ScopedJstring filename_jstr(env, filename);
    ScopedJstring funcname_jstr(env, funcname);
    ScopedJstring log_jst(env, _log);

    logWrite(xlog_info_common, tag_jstr.GetChar(), filename_jstr.GetChar(),
            funcname_jstr.GetChar(),log_jst.GetChar(), false);
}

JNIEXPORT void JNICALL Java_com_ximalaya_ting_android_xlogger_XLoggerJNI_appenderFlush(JNIEnv *env,
                                                                                       jobject,
                                                                                       jboolean _is_sync) {
    if (_is_sync) {
        appender_flush_sync();
    } else {
        appender_flush();
    }
}

JNIEXPORT jint JNICALL Java_com_ximalaya_ting_android_xlogger_XLoggerJNI_getLogLevel
        (JNIEnv *, jobject) {
    return xlogger_Level();
}

JNIEXPORT void JNICALL Java_com_ximalaya_ting_android_xlogger_XLoggerJNI_setLogLevel
        (JNIEnv *, jobject, jint _log_level) {
    xlogger_SetLevel((TLogLevel) _log_level);
}

JNIEXPORT void JNICALL Java_com_ximalaya_ting_android_xlogger_XLoggerJNI_setAppenderMode
        (JNIEnv *, jobject, jint _mode) {
    appender_setmode((TAppenderMode) _mode);
}
JNIEXPORT void JNICALL Java_com_ximalaya_ting_android_xlogger_XLoggerJNI_setConsoleLogOpen
        (JNIEnv *env, jobject, jboolean _is_open) {
    appender_set_console_log((bool) _is_open);
}

JNIEXPORT void JNICALL Java_com_ximalaya_ting_android_xlogger_XLoggerJNI_setMaxFileSize
        (JNIEnv *env, jobject, jlong _maxSize) {
    appender_set_max_file_size(_maxSize);
}

JNIEXPORT void JNICALL Java_com_ximalaya_ting_android_xlogger_XLoggerJNI_setMaxAliveTime
        (JNIEnv *env, jobject, jlong _maxTime) {
    appender_set_max_alive_duration(_maxTime);
}
}

