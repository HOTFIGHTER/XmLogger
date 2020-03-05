package com.ximalaya.ting.android.xlogger;
/**
 * @author linfeng.Yu
 * @date 2019-09-21. description：
 */
public class XLoggerJNI {
  static {
    System.loadLibrary("xlogger");
  }
  //静态内部类
  static class XLoggerInfo {

    public int level;
    public String tag;
    public String filename;
    public String funcname;
    public int line;
    public long pid;
    public long tid;
    public long maintid;
  }
  /*
   * JNI方式调用C层日志appenderOpen方法
   * */
  public native void appenderOpen(int level, int mode, String cacheDir, String logDir,
      String nameprefix, int cacheDays);

  /*
   * JNI方式调用C层设置日志等级
   * */
  public native void setLogLevel(int logLevel);

  /*
   * JNI方式调用C层获取日志等级
   * */
  public native int getLogLevel();

  /*
   * JNI方式调用C层设置添加模式
   * */
  public native void setAppenderMode(int mode);

  /*
   * JNI方式调用C层设置控制台是否打开
   * */
  public native void setConsoleLogOpen(boolean isOpen);  //set whether the console prints log

  /*
   * JNI方式调用C层设置文件最大值
   * */
  public native void setMaxFileSize(long size);

  /*
  * JNI方式调用C层设置文件最长保留时间
  * */
  public native void setMaxAliveTime(long duration);

  /*
   * JNI方式调用C层关闭添加句柄
   * */
  public native void appenderClose();

  /*
   * JNI方式调用C层写入日志
   * */
  public native void logWrite(XLoggerInfo logInfo, String log);

  /*
   * JNI方式调用C层进行Flush操作
   * */
  public native void appenderFlush(boolean isSync);
}
