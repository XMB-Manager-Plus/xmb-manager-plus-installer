#include "xmb-manager-debug.h"
#include "xmb-manager-include.h"

void debug_print(string text)
{
	B1.Mono(COLOR_BLACK);
	F1.Printf(100, 100,0xffffff,20, "%s", text.c_str());
	Graphics->Flip();
	sleep(10);
}


