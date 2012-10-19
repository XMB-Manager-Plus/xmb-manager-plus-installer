#ifndef __XMBMP_GRAPHICS_H__
#define __XMBMP_GRAPHICS_H__
#include "xmbmp-include.h"
#include "xmbmp-file.h"

extern NoRSX *Graphics;
extern NoRSX_Bitmap Precalculated_Layer;
extern Bitmap BMap;
extern Background B1;
extern Font F1;
extern Font F2;
extern MsgDialog Mess;

extern msgType MSG_OK;
extern msgType MSG_ERROR;
extern msgType MSG_YESNO_DNO;
extern msgType MSG_YESNO_DYES;

extern string mainfolder;

string int_to_string(int number);
int show_terms();
s32 center_text_x(int fsize, const char* message);
void draw_copy(string title, const char *dirfrom, const char *dirto, const char *filename, string cfrom, double copy_currentsize, double copy_totalsize, int numfiles_current, int numfiles_total, size_t countsize);
void make_background(string version, string type);

#endif
