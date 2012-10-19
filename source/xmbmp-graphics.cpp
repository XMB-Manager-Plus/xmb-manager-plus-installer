#include "xmbmp-graphics.h"

NoRSX *Graphics = new NoRSX(RESOLUTION_1920x1080);
Background B1(Graphics);
NoRSX_Bitmap Precalculated_Layer;
Bitmap BMap(Graphics);
Font F1(LATIN2, Graphics);
Font F2(LATIN2, Graphics);
MsgDialog Mess(Graphics);

msgType MSG_OK = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_ERROR = (msgType)(MSG_DIALOG_ERROR | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_YESNO_DNO = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO | MSG_DIALOG_DEFAULT_CURSOR_NO);
msgType MSG_YESNO_DYES = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO);

string int_to_string(int number)
{
	if (number == 0) return "0";
	string temp="";
	string returnvalue="";
	while (number>0)
	{
		temp+=number%10+48;
		number/=10;
	}
	for (size_t i=0;i<temp.length();i++)
		returnvalue+=temp[temp.length()-i-1];
	
	return returnvalue;
}

int show_terms()
{
	if (exists((mainfolder+"/terms-accepted.cfg").c_str())!=1)//terms not yet accepted
	{
		Mess.Dialog(MSG_OK,"Permission is hereby granted, FREE of charge, to any person obtaining a copy of this software and associated configuration files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, publish, distribute, sublicense, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n\nThe above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.");
		Mess.Dialog(MSG_YESNO_DYES,"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\nDo you accept this terms?");
		if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
		{
			create_file((mainfolder+"/terms-accepted.cfg").c_str());
			return 1;
		}
		else return 0;
	}
	return 1;
}

s32 center_text_x(int fsize, const char* message)
{
	return (Graphics->width-(strlen(message)*fsize/2))/2;
}

void draw_copy(string title, const char *dirfrom, const char *dirto, const char *filename, string cfrom, double copy_currentsize, double copy_totalsize, int numfiles_current, int numfiles_total, size_t countsize)
{
	int sizeTitleFont = 40;
	int sizeFont = 25;
	string current;

	B1.Mono(COLOR_BLACK);
	F1.Printf(center_text_x(sizeTitleFont, title.c_str()),220, 0xd38900, sizeTitleFont, title.c_str());
	F2.Printf(100, 260, COLOR_WHITE, sizeFont, "Filename: %s", ((string)filename+" ("+convert_size(get_filesize(cfrom.c_str()), "auto").c_str()+")").c_str());
	F2.Printf(100, 320, COLOR_WHITE, sizeFont, "From: %s", dirfrom);
	F2.Printf(100, 350, COLOR_WHITE, sizeFont, "To: %s", dirto);
	current=convert_size(copy_currentsize+(double)countsize, "auto")+" of "+convert_size(copy_totalsize, "auto").c_str()+" copied ("+int_to_string(numfiles_current).c_str()+" of "+int_to_string(numfiles_total).c_str()+" files)";
	F2.Printf(center_text_x(sizeFont, current.c_str()), 410, COLOR_WHITE, sizeFont, "%s", current.c_str());
	Graphics->Flip();
}

void make_background(string version, string type)
{
	string IMAGE_PATH=mainfolder+"/data/images/logo.png";
    pngData png;
	Image I1(Graphics); 
	int sizeFont = 30;

	BMap.GenerateBitmap(&Precalculated_Layer);
	B1.MonoBitmap(COLOR_BLACK,&Precalculated_Layer);
    I1.LoadPNG(IMAGE_PATH.c_str(), &png);
	u32 imgX =(Graphics->width/2)-(png.width/2), imgY = 30;
	//I1.AlphaDrawIMG(imgX,imgY,&png);
	I1.AlphaDrawIMGtoBitmap(imgX,imgY,&png,&Precalculated_Layer);
	F2.PrintfToBitmap(center_text_x(sizeFont, "Firmware: X.XX (CEX)"),Graphics->height-(sizeFont+20+(sizeFont-5)+10),&Precalculated_Layer,0xc0c0c0,sizeFont, "Firmware: %s (%s)", version.c_str(), type.c_str());
	F2.PrintfToBitmap(center_text_x(sizeFont-5, "Installer created by XMBM+ Team"),Graphics->height-((sizeFont-5)+10),&Precalculated_Layer,0xd38900,sizeFont-5,     "Installer created by XMBM+ Team");
}
