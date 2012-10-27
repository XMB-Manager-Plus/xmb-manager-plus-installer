#include "xmbmp-graphics.h"

//NoRSX *Graphics = new NoRSX(RESOLUTION_1920x1080);
NoRSX *Graphics = new NoRSX();
Background B1(Graphics);
Bitmap BMap(Graphics);
NoRSX_Bitmap Menu_Layer;
Font F1(LATIN2, Graphics);
Font F2(LATIN2, Graphics);
Image I1(Graphics);
pngData png;
MsgDialog Mess(Graphics);
//Printf PF("/dev_usb000/log.txt");
Printf PF("/dev_hdd0/game/XMBMANPLS/USRDIR/data/log.txt");

msgType MSG_OK = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_ERROR = (msgType)(MSG_DIALOG_ERROR | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_YESNO_DNO = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO | MSG_DIALOG_DEFAULT_CURSOR_NO);
msgType MSG_YESNO_DYES = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO);

int ypos(int y)
{
	return (int)(y*Graphics->height)/720;
}

int xpos(int x)
{
	return (int)(x*Graphics->width)/1280;
}

s32 center_text_x(int fsize, const char* message)
{
	return (Graphics->width-(strlen(message)*fsize/2))/2;
}

void bitmap_intitalize(string folder)
{
	string IMAGE_PATH=folder+"/data/images/logo.png";

	BMap.GenerateBitmap(&Menu_Layer);
    I1.LoadPNG(IMAGE_PATH.c_str(), &png);
}

void bitmap_background(string version, string type)
{
	int sizeFont = ypos(30);

	B1.MonoBitmap(COLOR_BLACK,&Menu_Layer);
	u32 imgX =(Graphics->width/2)-(png.width/2), imgY = ypos(30);
	I1.AlphaDrawIMGtoBitmap(imgX,imgY,&png,&Menu_Layer);
	F2.PrintfToBitmap(center_text_x(sizeFont, "Firmware: X.XX (CEX)"),Graphics->height-(sizeFont+ypos(20)+(sizeFont-ypos(5))+ypos(10)),&Menu_Layer,0xc0c0c0,sizeFont, "Firmware: %s (%s)", version.c_str(), type.c_str());
	F2.PrintfToBitmap(center_text_x(sizeFont-ypos(5), "Installer created by XMBM+ Team"),Graphics->height-((sizeFont-ypos(5))+ypos(10)),&Menu_Layer,0xd38900,sizeFont-ypos(5), "Installer created by XMBM+ Team");
}

void draw_menu(int choosed)
{
	BMap.DrawBitmap(&Menu_Layer);
	Graphics->Flip();
	if (choosed==1) usleep(50*1000);
	else usleep(1000);
}

