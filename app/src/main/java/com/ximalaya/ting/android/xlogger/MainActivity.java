package com.ximalaya.ting.android.xlogger;

import android.os.Environment;
import android.os.Handler;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import com.ximalaya.ting.android.xlogger.XLoggerJNI.XLoggerInfo;


public class MainActivity extends AppCompatActivity {

  final String SDCARD = Environment.getExternalStorageDirectory().getAbsolutePath();
  private Handler mHandler=new Handler();

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    mHandler.post(new Runnable() {
      @Override
      public void run() {
        //要使用apk所在的路径
        String path=MainActivity.this.getApplicationContext().getFilesDir().toString();
        XLoggerJNI xLogerJNI=new XLoggerJNI();
        xLogerJNI.appenderOpen(1, 0, path+"/ximalayacache",
            path+"/ximalayalog", "y-log", 0);
        int i=0;
        while (true) {
          XLoggerInfo xLoggerInfo=new XLoggerInfo();
          xLoggerInfo.filename="yu";
          xLoggerInfo.funcname="ok";
          xLoggerInfo.level=1;
          xLoggerInfo.pid=0;
          xLoggerInfo.tag="yucollect";
          xLoggerInfo.tid=2;
          xLoggerInfo.maintid=2;
          xLogerJNI.logWrite(xLoggerInfo, "my name is yulinfeng"+i);
          i++;
        }
      }
    });

  }
}
