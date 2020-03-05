//
// Created by xmly on 2019-09-24.
// 文件操作工具类
//
#include <logger/util/file_operator.h>

/*
 * 判断文件是不是存在
 * */
bool isFileExists(const char *filename) {
  return access(filename, F_OK) == 0;
}

/*
 * 获取文件大小
 * */
int getFileSize(const char *filename) {
  struct stat stat_buf;
  stat(filename, &stat_buf);
  unsigned int len = (unsigned int) stat_buf.st_size;
  return len;
}

/*
 * 创建文件夹
 * */
int mkDir(const char *dirname) {
  if (isFileExists(dirname)) {
    return 0;
  } else {
    return mkdir(dirname, 0777);
  }
}

/*
 * 判断是不是文件夹
 * */
DIR *getDirExists(const char *filename) {
  DIR *dir;
  if ((dir = opendir(filename)) == NULL) {
    return NULL;
  }
  return dir;
}

/*
 * 移除文件或文件夹
 * */
void reMove(const char *dirname) {
  remove(dirname);
}

/*
 * 判断字符窜是否以什么开头
 * */
int isBeginWith(const char *str1, const char *str2) {
  if (str1 == NULL || str2 == NULL)
    return -1;
  int len1 = strlen(str1);
  int len2 = strlen(str2);
  if ((len1 < len2) || (len1 == 0 || len2 == 0))
    return -1;
  const char *p = str2;
  int i = 0;
  while (*p != '\0') {
    if (*p != str1[i])
      return 0;
    p++;
    i++;
  }
  return 1;
}

/*
 * 判断字符串是否以什么结尾
 * */
int isEndWith(const char *str1, const char *str2) {
  if (str1 == NULL || str2 == NULL)
    return -1;
  int len1 = strlen(str1);
  int len2 = strlen(str2);
  if ((len1 < len2) || (len1 == 0 || len2 == 0))
    return -1;
  while (len2 >= 1) {
    if (str2[len2 - 1] != str1[len1 - 1])
      return 0;
    len2--;
    len1--;
  }
  return 1;
}


