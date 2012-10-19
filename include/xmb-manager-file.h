#ifndef __XMB_MANAGER_FILE_H__
#define __XMB_MANAGER_FILE_H__
#include "xmb-manager-include.h"
#include "xmb-manager-graphics.h"

string convert_size(double size, string format);
double get_free_space(const char *path);
double get_filesize(const char *path);
string create_file(const char* cpath);
int exists(const char *path);
s32 create_dir(string dirtocreate);
string recursiveDelete(string direct);

#endif
