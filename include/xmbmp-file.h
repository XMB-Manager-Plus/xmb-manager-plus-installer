#ifndef __XMBMP_FILE_H__
#define __XMBMP_FILE_H__
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <dirent.h>
#include <string>
#include <algorithm>

#define CHUNK 16384

using namespace std;

string int_to_string(int number);
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
void draw_copy(string title, const char *dirfrom, const char *dirto, const char *filename, string cfrom, double copy_currentsize, double copy_totalsize, int numfiles_current, int numfiles_total, size_t countsize);
int show_terms(string folder);
int menu_restore_available(string folder);

#endif
