#include <ppu-lv2.h>
#include <stdio.h>
#include <stdlib.h>
#include <io/pad.h>
//Image, as all the other functions, is already included inside NoRSX header
#include "NoRSX.h"

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <string>

#define MAX_BUFFERS 2

std::string appname="XMBMANPLS";
std::string APPFOLDER="/dev_hdd0/game/"+appname;
std::string mainfolder=APPFOLDER+"/resources/";
std::string fw_version;
int fw_version_index;
int menu1_size[]= { 4, 2 };
int menu2_size = 3;

static int exitapp, xmbopen;

static inline void eventHandler(u64 status, u64 param, void * userdata)
{
	switch(status)
	{
		case SYSUTIL_EXIT_GAME:
			exitapp = 0;
			break;
		case SYSUTIL_MENU_OPEN:
			xmbopen = 1;
			break;
		case SYSUTIL_MENU_CLOSE:
			xmbopen = 0;
			break;
	}
}

msgType MSG_OK = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_ERROR = (msgType)(MSG_DIALOG_ERROR | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_YESNO = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO | MSG_DIALOG_DISABLE_CANCEL_ON);

s32 lv2_get_platform_info(uint8_t platform_info[0x18])
{
   lv2syscall1(387, (uint64_t) platform_info);
   return_to_user_prog(s32);
}

s32 sysFsMount(const char* MOUNT_POINT, const char* TYPE_OF_FILESYSTEM, const char* PATH_TO_MOUNT, int IF_READ_ONLY){
          lv2syscall8(837, (u64)MOUNT_POINT, (u64)TYPE_OF_FILESYSTEM, (u64)PATH_TO_MOUNT, 0, IF_READ_ONLY, 0, 0, 0);
          return_to_user_prog(s32);
}

s32 sysFsUnmount(const char* PATH_TO_UNMOUNT){
          lv2syscall1(838, (u64)PATH_TO_UNMOUNT);
          return_to_user_prog(s32);
}

s32 copy_file(const char* cfrom, const char* ctoo)
{
  FILE *from, *to;
  char ch;

  /* open source file */
  if((from = fopen(cfrom, "rb"))==NULL) return 1;

  /* open destination file */
  if((to = fopen(ctoo, "wb"))==NULL) return 1;

  /* copy the file */
  while(!feof(from)) {
    ch = fgetc(from);
    if(ferror(from))  return 1;
    if(!feof(from)) fputc(ch, to);
    if(ferror(to))  return 1;
  }

  if(fclose(from)==EOF) return 1;

  if(fclose(to)==EOF) return 1;

  return 0;
}

s32 install(std::string fw, std::string app)
{
	const char* DEV_BLIND = "CELL_FS_IOS:BUILTIN_FLSH1";	// dev_flash
	const char* FAT = "CELL_FS_FAT"; //it's also for fat32
	std::string MOUNT_POINT = "/dev_blind"; //our mount point

	//we may need to check if it is already mounted:
	sysFSStat dir;

	std::string direct;
	std::string sourcefile;
	std::string destfile;
	DIR *dp;
	struct dirent *dirp;
	struct stat filestat;

	std::string DEST_RCO_FOLDER=MOUNT_POINT+"/vsh/resource";
	std::string DEST_XML_FOLDER=MOUNT_POINT+"/vsh/resource/explore/xmb";
	std::string DEST_SPRX_FOLDER=MOUNT_POINT+"/vsh/module";
	int files_copied=0;

	int is_mounted = sysFsStat(MOUNT_POINT.c_str(), &dir);
	if(is_mounted != 0)
		sysFsMount(DEV_BLIND, FAT, MOUNT_POINT.c_str(), 0);
	//copy XML files
	direct=mainfolder+"/"+fw_version+"/"+fw+"/"+app+"/xml";
	dp = opendir (direct.c_str());
	if (dp == NULL) return 1;
	//F2.Printf(textX,250,COLOR_BLUE,"Reading folder %s", direct.c_str());
	while ( (dirp = readdir(dp) ) )
	{
		sourcefile = direct + "/" + dirp->d_name;
		destfile = DEST_XML_FOLDER + "/" + dirp->d_name;
		if (stat( sourcefile.c_str(), &filestat )) continue;
		if (S_ISDIR( filestat.st_mode ))         continue;
		copy_file(sourcefile.c_str(), destfile.c_str());
		files_copied++;
	}
	closedir(dp);

	//copy RCO files
	direct=mainfolder+"/"+fw_version+"/"+fw+"/"+app+"/rco";
	dp = opendir (direct.c_str());
	if (dp == NULL) return 1;
	//F2.Printf(textX,250,COLOR_BLUE,"Reading folder %s", direct.c_str());
	while ( (dirp = readdir(dp) ) )
	{
		sourcefile = direct + "/" + dirp->d_name;
		destfile = DEST_RCO_FOLDER + "/" + dirp->d_name;
		if (stat( sourcefile.c_str(), &filestat )) continue;
		if (S_ISDIR( filestat.st_mode ))         continue;
		copy_file(sourcefile.c_str(), destfile.c_str());
		files_copied++;
	}
	closedir(dp);

	//copy SPRX files
	direct=mainfolder+"/"+fw_version+"/"+fw+"/"+app+"/sprx";
	dp = opendir (direct.c_str());
	if (dp == NULL) return 1;
	//F2.Printf(textX,250,COLOR_BLUE,"Reading folder %s", direct.c_str());
	while ( (dirp = readdir(dp) ) )
	{
		sourcefile = direct + "/" + dirp->d_name;
		destfile = DEST_SPRX_FOLDER + "/" + dirp->d_name;
		if (stat( sourcefile.c_str(), &filestat )) continue;
		if (S_ISDIR( filestat.st_mode ))         continue;
		copy_file(sourcefile.c_str(), destfile.c_str());
		files_copied++;
	}
	closedir(dp);

	return 0;
}

s32 center_text_x(NoRSX *Graphics, int fsize, const char* message)
{
	return (Graphics->width-(strlen(message)*fsize/2))/2;
}

s32 draw_menu(NoRSX *Graphics, int menu_id, int selected,int choosed, std::string status)
{
	std::string menu1[][10]= { { "Normal CFW", "Cobra CFW", "NFW CFW", "Rebug CFW" }, { "Normal CFW", "Rebug CFW" } };
	std::string menu2[]    ={
						"INSTALL XMB Manager Plus",
						"INSTALL Rebug Package Manager",
						"RESTORE Install Package Files"
						};

	std::string IMAGE_PATH=APPFOLDER+"/resources/images/xmbm_transparent.png";
	//std::string IMAGE_PATH=APPFOLDER+"/ICON0.PNG";
	//std::string TITLEFONT_PATH=APPFOLDER+"/resources/fonts/GOODTIME.ttf";
	//std::string TEXTFONT_PATH="/dev_flash/data/font/SCE-PS3-SR-R-LATIN2.TTF";
	std::string TITLEFONT_PATH=APPFOLDER+"/resources/fonts/arial.ttf";
	std::string TEXTFONT_PATH=APPFOLDER+"/resources/fonts/arial.ttf";
	std::string TEXTFONTBOLD_PATH=APPFOLDER+"/resources/fonts/arialbd.ttf";
	int cury=0;

	Background B1(Graphics);
    //i want to display a PNG. i need to define the structure pngData (jpgData if you need to load a JPG).
    pngData png;
	//i need as first thing to initializa the Image class:
	Image I1(Graphics); 
    //Now i need to load the image. you can use LoadPNG (or LoadJPG if a jpg)
	B1.Mono(COLOR_BLACK);
    I1.LoadPNG(IMAGE_PATH.c_str(), &png);
	u32 imgX =(Graphics->width/2)-(png.width/2), imgY = 30;
	//Now you loaded the png into memory. you need to display it: This will draw the image at x = 200 and y = 300 on the buffer.
	I1.AlphaDrawIMG(imgX,imgY,&png);
	int sizeTitleFont = 30;
	int sizeFont = 20;
	//Font F1(0xd38900, sizeTitleFont, TITLEFONT_PATH.c_str(), Graphics);
	//Font F2(TEXTFONT_PATH.c_str(), Graphics);
	//Font F3(TEXTFONTBOLD_PATH.c_str(), Graphics);
	Font F1(LATIN2, Graphics);
	Font F2(LATIN2, Graphics);
	Font F3(LATIN2, Graphics);

	if (menu_id==1)
	{
		F1.Printf(center_text_x(Graphics, sizeTitleFont, "CHOOSE A FIRMWARE"),220,	0xd38900, sizeTitleFont, "CHOOSE A FIRMWARE");
		for(int j=0;j<menu1_size[fw_version_index];j++)
		{
			if (j==choosed) F3.Printf(center_text_x(Graphics, sizeFont, menu1[fw_version_index][j].c_str()),280+(j*(sizeFont+4)),COLOR_RED,sizeFont, "%s",menu1[fw_version_index][j].c_str());
			else if (j==selected) F3.Printf(center_text_x(Graphics, sizeFont, menu1[fw_version_index][j].c_str()),280+(j*(sizeFont+4)),COLOR_YELLOW,sizeFont, "%s",menu1[fw_version_index][j].c_str());
			else F2.Printf(center_text_x(Graphics, sizeFont, menu1[fw_version_index][j].c_str()),280+(j*(sizeFont+4)),COLOR_WHITE,sizeFont, "%s",menu1[fw_version_index][j].c_str());
		}
		F2.Printf(center_text_x(Graphics, sizeFont, "Press START to exit"),280+((menu1_size[fw_version_index]+1)*(sizeFont+4)),COLOR_WHITE,sizeFont, "Press START to exit");
		cury=280+((menu1_size[fw_version_index]+2)*(sizeFont+4));
	}
	else if (menu_id==2)
	{
		F1.Printf(center_text_x(Graphics, sizeTitleFont, "CHOOSE WHAT TO INSTALL"),220, 0xd38900, sizeTitleFont, "CHOOSE WHAT TO INSTALL");
		for(int j=0;j<menu2_size;j++)
		{
			if (j==choosed) F3.Printf(center_text_x(Graphics, sizeFont, menu2[j].c_str()),280+(j*(sizeFont+4)),COLOR_RED,sizeFont, "%s",menu2[j].c_str());
			else if (j==selected) F3.Printf(center_text_x(Graphics, sizeFont, menu2[j].c_str()),280+(j*(sizeFont+4)),COLOR_YELLOW,sizeFont, "%s",menu2[j].c_str());
			else F2.Printf(center_text_x(Graphics, sizeFont, menu2[j].c_str()),280+(j*(sizeFont+4)),COLOR_WHITE,sizeFont, "%s",menu2[j].c_str());
		}
		F2.Printf(center_text_x(Graphics, sizeFont, "Press O to go back"),280+((menu2_size+1)*(sizeFont+4)),COLOR_WHITE,sizeFont, "Press O to go back");
		F2.Printf(center_text_x(Graphics, sizeFont, "Press START to exit"),280+((menu2_size+2)*(sizeFont+4)),COLOR_WHITE,sizeFont, "Press START to exit");
		cury=280+((menu2_size+3)*(sizeFont+4));
	}

	u32 textX =(Graphics->width/2)-230;
	F2.Printf(textX,cury+30,0xc0c0c0,sizeFont,     "Firmware version: %s", fw_version.c_str());
	F2.Printf(textX+300,cury+30,0xc0c0c0,sizeFont,     "Status: %s", status.c_str());
	
	Graphics->Flip();

	return 0;
}

s32 main(s32 argc, const char* argv[])
{
	//this is the structure for the pad controllers
	padInfo padinfo;
	padData paddata;

	sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, eventHandler, NULL);
	std::string firmware_choice;
	std::string app_choice;
	std::string menu1_val[][10]= { { "cfw", "cobra", "nfw", "rebug" }, { "cfw", "rebug" } };
	std::string menu2_val[]={ "xmbmanpls", "pkgmanage", "original" };

	int i;
	int ret=0;
	int menu_position=0;
	int menu2_position=0;

	//this will initialize the controller (7= seven controllers)
	ioPadInit (7);
	//this will initialize NoRSX..
	//NoRSX *Graphics = new NoRSX(RESOLUTION_1920x1080);
	NoRSX *Graphics = new NoRSX(RESOLUTION_1280x720);
	MsgDialog Mess(Graphics);
	uint8_t platform_info[0x18];
	lv2_get_platform_info(platform_info);
	uint32_t fw = platform_info[0]* (1 << 16) + platform_info[1] *(1<<8) + platform_info[2];
	if (fw==0x30550)
	{
		fw_version="3.55";
		fw_version_index=0;
	}
	else if (fw==0x30410)
	{
		fw_version="3.41";
		fw_version_index=1;
	}
	else
	{
		Mess.Dialog(MSG_ERROR,"Only 3.55 and 3.41 firmware supported.");
		goto end;
	}

	menu_position=0;
	menu_1:
	draw_menu(Graphics,1,menu_position,-1,"Waiting");
	while (1)
	{
		ioPadGetInfo (&padinfo);
		//this will wait for a START from any pad
		for(i = 0; i < MAX_PADS; i++)
		{
			if (padinfo.status[i])
			{
				ioPadGetData (i, &paddata);
				if (paddata.BTN_START)
				{
					goto end;
				}
				if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
				{
					if (menu_position<menu1_size[fw_version_index]-1)
					{
						menu_position++;
						goto menu_1;
					}
				}
				if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
				{
					if (menu_position>0)
					{
						menu_position--;
						goto menu_1;
					}
				}
				if (paddata.BTN_CROSS)
				{
					draw_menu(Graphics,1,-1,menu_position,"Waiting");
					firmware_choice=menu1_val[fw_version_index][menu_position];
					sleep(0.05);
					goto continue_to_menu2;
				}
			}
		}
		
		sysUtilCheckCallback();
	}

	continue_to_menu2:
	menu2_position=0;	
	menu_2:
	draw_menu(Graphics,2,menu2_position,-1,"Waiting");
	while (1)
	{
		ioPadGetInfo (&padinfo);
		//this will wait for a START from any pad
		for(i = 0; i < MAX_PADS; i++)
		{
			if (padinfo.status[i])
			{
				ioPadGetData (i, &paddata);
				if (paddata.BTN_START)
				{
					goto end;
				}
				if (paddata.BTN_CIRCLE)
				{
					goto menu_1;
				}
				if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
				{
					if (menu2_position<menu2_size-1)
					{
						menu2_position++;
						goto menu_2;
					}
				}
				if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
				{
					if (menu2_position>0)
					{
						menu2_position--;
						goto menu_2;
					}
				}
				if (paddata.BTN_CROSS)
				{
					draw_menu(Graphics,2,-1,menu2_position,"Installing...");
					app_choice=menu2_val[menu2_position];
					sleep(0.05);
					draw_menu(Graphics,2,menu2_position,-1,"Installing...");
					ret=install(firmware_choice, app_choice);
					if (ret == 0)
					{
						Mess.Dialog(MSG_OK,"Installed!\nPress OK to reboot.");
						//draw_menu(Graphics,2,-1,menu2_position,"Installed!");
						//sleep(2);
						//draw_menu(Graphics,2,-1,menu2_position,"Rebooting...");
						//sleep(2);
						goto end_with_reboot;
					}
					else
					{
						Mess.Dialog(MSG_ERROR,"Not installed!\nSome error occured.");
						draw_menu(Graphics,2,-1,menu2_position,"Error!");
					}
				}
			}
		}
		sysUtilCheckCallback();
	}

	end_with_reboot:
	{
		//this will uninitialize the controllers
		ioPadEnd();
		//This will uninit the NoRSX lib
		Graphics->NoRSX_Exit();
		lv2syscall4(379,0x1200,0,0,0); //reboot
	}

	end:
	{
		//this will uninitialize the controllers
		ioPadEnd();
		//This will uninit the NoRSX lib
		Graphics->NoRSX_Exit();
	}
	return 0;
}
