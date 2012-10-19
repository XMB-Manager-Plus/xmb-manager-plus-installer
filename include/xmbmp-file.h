#ifndef __XMBMP_FILE_H__
#define __XMBMP_FILE_H__
#include "xmbmp-graphics.h"

string convert_size(double size, string format);
double get_free_space(const char *path);
double get_filesize(const char *path);
string create_file(const char* cpath);
int exists(const char *path);
s32 create_dir(string dirtocreate);
string recursiveDelete(string direct);
const string fileCreatedDateTime(const char *path);
string correct_path(string dpath, int what);
string *recursiveListing(string direct);
string copy_file(string title, const char *dirfrom, const char *dirto, const char *filename, double copy_currentsize, double copy_totalsize, int numfiles_current, int numfiles_total, int check_flag);

#endif
