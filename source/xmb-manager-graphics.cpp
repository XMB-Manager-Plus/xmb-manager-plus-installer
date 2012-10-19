#include "xmb-manager-graphics.h"

NoRSX *Graphics = new NoRSX(RESOLUTION_1920x1080);
Background B1(Graphics);
Font F1(LATIN2, Graphics);
Font F2(LATIN2, Graphics);
MsgDialog Mess(Graphics);

msgType MSG_OK = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_ERROR = (msgType)(MSG_DIALOG_ERROR | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_YESNO_DNO = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO | MSG_DIALOG_DEFAULT_CURSOR_NO);
msgType MSG_YESNO_DYES = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO);

string mainfolder;

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
