#ifndef __XMBMP_FILE_H__
#define __XMBMP_FILE_H__
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <dirent.h>
#include <string>
#include <algorithm>

#define CHUNK 16384
#define APP_TITLEID "XMBMANPLS"
#define DEV_TITLEID "PS3LOAD00"

using namespace std;

string int_to_string(int number);
string convert_size(double size, string format);
double get_free_space(const char *path);
double get_filesize(const char *path);
const string fileCreatedDateTime(const char *path);
string create_file(const char* cpath);
int exists(const char *path);
int exists_backups(string folder);
int mkdir_one(string fullpath);
int mkdir_full(string fullpath);
string recursiveDelete(string direct);
string *recursiveListing(string direct);
string correct_path(string dpath, int what);
string get_app_folder(char* path);
void check_firmware_changes(string folder);
int show_terms(string folder);
//void draw_copy(string title, const char *dirfrom, const char *dirto, const char *filename, string cfrom, double copy_currentsize, double copy_totalsize, int numfiles_current, int numfiles_total, size_t countsize);
string copy_file(string title, const char *dirfrom, const char *dirto, const char *filename, double copy_currentsize, double copy_totalsize, int numfiles_current, int numfiles_total, int check_flag);

#endif