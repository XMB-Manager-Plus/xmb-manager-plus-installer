#ifndef __XMBMP_GRAPHICS_H__
#define __XMBMP_GRAPHICS_H__
#include <NoRSX.h>
#include <string>

using namespace std;

extern NoRSX *Graphics;
extern NoRSX_Bitmap Precalculated_Layer;
extern Bitmap BMap;
extern Background B1;
extern Font F1;
extern Font F2;
extern MsgDialog Mess;
extern Printf PF;

extern msgType MSG_OK;
extern msgType MSG_ERROR;
extern msgType MSG_YESNO_DNO;
extern msgType MSG_YESNO_DYES;

s32 center_text_x(int fsize, const char* message);
void make_background(string version, string type, string folder);

#endif
