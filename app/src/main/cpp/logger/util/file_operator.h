//
// Created by xmly on 2019-09-25.
//

#ifndef MEDIA_FILE_OPERATOR_H
#define MEDIA_FILE_OPERATOR_H
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <zconf.h>
#include <dirent.h>

bool isFileExists(const char *filename);

DIR *getDirExists(const char *filename);

int getFileSize(const char *filename);

int mkDir(const char *dirname);

void reMove(const char *dirname);

int isBeginWith(const char *str1, const char *str2);

int isEndWith(const char *str1, const char *str2);
#endif //MEDIA_FILE_OPERATOR_H
