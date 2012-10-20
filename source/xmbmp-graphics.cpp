#include "xmbmp-graphics.h"

NoRSX *Graphics = new NoRSX(RESOLUTION_1920x1080);
Background B1(Graphics);
NoRSX_Bitmap Precalculated_Layer;
Bitmap BMap(Graphics);
Font F1(LATIN2, Graphics);
Font F2(LATIN2, Graphics);
MsgDialog Mess(Graphics);
Printf PF("/dev_usb000/xmbmanpls_log.txt");

msgType MSG_OK = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_ERROR = (msgType)(MSG_DIALOG_ERROR | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_YESNO_DNO = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO | MSG_DIALOG_DEFAULT_CURSOR_NO);
msgType MSG_YESNO_DYES = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO);

s32 center_text_x(int fsize, const char* message)
{
	return (Graphics->width-(strlen(message)*fsize/2))/2;
}

void make_background(string version, string type, string folder)
{
	string IMAGE_PATH=folder+"/data/images/logo.png";
    pngData png;
	Image I1(Graphics); 
	int sizeFont = 30;

	BMap.GenerateBitmap(&Precalculated_Layer);
	B1.MonoBitmap(COLOR_BLACK,&Precalculated_Layer);
    I1.LoadPNG(IMAGE_PATH.c_str(), &png);
	u32 imgX =(Graphics->width/2)-(png.width/2), imgY = 30;
	I1.AlphaDrawIMGtoBitmap(imgX,imgY,&png,&Precalculated_Layer);
	F2.PrintfToBitmap(center_text_x(sizeFont, "Firmware: X.XX (CEX)"),Graphics->height-(sizeFont+20+(sizeFont-5)+10),&Precalculated_Layer,0xc0c0c0,sizeFont, "Firmware: %s (%s)", version.c_str(), type.c_str());
	F2.PrintfToBitmap(center_text_x(sizeFont-5, "Installer created by XMBM+ Team"),Graphics->height-((sizeFont-5)+10),&Precalculated_Layer,0xd38900,sizeFont-5, "Installer created by XMBM+ Team");
}
