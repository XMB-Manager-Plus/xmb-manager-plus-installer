#ifndef __XMBMP_GRAPHICS_H__
#define __XMBMP_GRAPHICS_H__
#include <NoRSX.h>
#include <string>

using namespace std;

extern NoRSX *Graphics;
extern Bitmap BMap;
extern NoRSX_Bitmap Menu_Layer;
extern Background B1;
extern Font F1;
extern Font F2;
extern Image I1;
extern pngData png;
extern MsgDialog Mess;
extern Printf PF;

extern msgType MSG_OK;
extern msgType MSG_ERROR;
extern msgType MSG_YESNO_DNO;
extern msgType MSG_YESNO_DYES;

s32 center_text_x(int fsize, const char* message);
void bitmap_intitalize(string folder);
void bitmap_background(string version, string type);
void draw_menu(int choosed);
int ypos(int y);
int xpos(int x);

#endif
