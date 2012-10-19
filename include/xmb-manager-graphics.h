#ifndef __XMB_MANAGER_GRAPHICS_H__
#define __XMB_MANAGER_GRAPHICS_H__
#include <NoRSX.h> 
#include "xmb-manager-include.h"
#include "xmb-manager-file.h"

extern NoRSX *Graphics;
extern Background B1;
extern Font F1;
extern Font F2;
extern MsgDialog Mess;

extern msgType MSG_OK;
extern msgType MSG_ERROR;
extern msgType MSG_YESNO_DNO;
extern msgType MSG_YESNO_DYES;

extern string mainfolder;

int show_terms();

#endif
