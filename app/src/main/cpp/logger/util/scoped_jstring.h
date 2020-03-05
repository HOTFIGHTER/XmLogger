//
// Created by xmly on 2019-09-21.
//
#ifndef SCOPEDJSTRING_H_
#define SCOPEDJSTRING_H_

#include <jni.h>

class ScopedJstring {
 public:
  ScopedJstring(JNIEnv *_env, jstring _jstr);
  ScopedJstring(JNIEnv *_env, const char *_char);

  ~ScopedJstring();

  const char *GetChar() const;
  const char *SafeGetChar() const;
  jstring GetJstr() const;

 private:
  ScopedJstring();
  ScopedJstring(const ScopedJstring &);
  ScopedJstring &operator=(const ScopedJstring &);

 private:
  JNIEnv *env_;
  jstring jstr_;
  const char *char_;
  bool jstr2char_;
};

#endif /* SCOPEDJSTRING_H_ */
