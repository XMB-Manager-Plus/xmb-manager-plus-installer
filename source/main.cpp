#include <ppu-lv2.h>
#include <lv2/sysfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <io/pad.h>
#include <NoRSX.h> //Image, as all the other functions, is already included inside NoRSX header
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <string>
#include <algorithm>
#include <time.h>
#include <errno.h>

#define MAX_BUFFERS 2

std::string mainfolder;
std::string fw_version="";
std::string ttype="";
std::string menu1[10];
int menu1_size=0;
int menu1_restore=1;
std::string menu2[10][10];
int menu2_size[10];
std::string menu3[30];
int menu3_size=0;

msgType MSG_OK = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_ERROR = (msgType)(MSG_DIALOG_ERROR | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_YESNO = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO | MSG_DIALOG_DEFAULT_CURSOR_NO);

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

u32 reboot_sys(){ 
	lv2syscall4(379,0x200,0,0,0);
	return_to_user_prog(u32);
}

/*
 * lv2_get_target_type
 *
 * Types:
 * 	1 - Retail
 * 	2 - Debug
 * 	3 - Reference Tool
 */
s32 lv2_get_target_type(uint64_t *type)
{
	lv2syscall1(985, (uint64_t) type);
	return_to_user_prog(s32);
}

void debug_print(NoRSX *Graphics, std::string text)
{
	Background B1(Graphics);
	B1.Mono(COLOR_BLACK);
	Font F1(LATIN2, Graphics);
	F1.Printf(100, 100,0xffffff,20, "%s", text.c_str());
	Graphics->Flip();
	sleep(10);
}

std::string copy_file(const char* cfrom, const char* ctoo)
{
  FILE *from, *to;
  char ch;

  /* open source file */
  if ((from = fopen(cfrom, "rb"))==NULL) return "Cannot open source file ("+(std::string)cfrom+") for reading!";

  /* open destination file */
  if((to = fopen(ctoo, "wb"))==NULL) return "Cannot open destination file ("+(std::string)ctoo+") for writing!";

  /* copy the file */
  while(!feof(from)) {
    ch = fgetc(from);
    if(ferror(from))  return "Error reading source file ("+(std::string)cfrom+")!";
    if(!feof(from)) fputc(ch, to);
    if(ferror(to))  return "Error writing destination file ("+(std::string)ctoo+")!";
  }

  if(fclose(from)==EOF) return "Cannot close source file ("+(std::string)cfrom+")!";

  if(fclose(to)==EOF) return "Cannot close destination file ("+(std::string)ctoo+")!";

  return "";
}

std::string create_file(const char* cpath)
{
  FILE *path;

  /* open destination file */
  if((path = fopen(cpath, "wb"))==NULL) return "Cannot open file ("+(std::string)cpath+") for writing!";
  if(fclose(path)==EOF) return "Cannot close file ("+(std::string)cpath+")!";

  return "";
}

int exists(const char *path)
{
	sysFSStat info;

	if (sysFsStat(path, &info) >= 0) return 1;
	return 0;
}


const std::string fileCreatedDateTime(const char *path)
{
	time_t tmod;
	char buf[80];
	sysFSStat info;

	if (sysFsStat(path, &info) >= 0)
	{
		tmod=info.st_mtime;
		strftime(buf, sizeof(buf), "%Y-%m-%d %Hh%Mm%Ss", localtime(&tmod));
		return buf;
	}
	else return "";
}

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime()
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d %Hh%Mm%Ss", &tstruct);

    return buf;
}

s32 create_dir(std::string dirtocreate)
{
	return mkdir(dirtocreate.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

std::string recursiveDelete(NoRSX *Graphics, std::string direct)
{
	std::string dfile;
	DIR *dp;
	struct dirent *dirp;

	MsgDialog Messa(Graphics);

	dp = opendir (direct.c_str());
	if (dp != NULL)
	{
		while ((dirp = readdir (dp)))
		{
			dfile = direct + "/" + dirp->d_name;
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				//Messa.Dialog(MSG_OK,("Testing: "+dfile).c_str());
				if (dirp->d_type == DT_DIR)
				{
					//Messa.Dialog(MSG_OK,("Is directory: "+dfile).c_str());
					recursiveDelete(Graphics,dfile);
				}
				else
				{
					//Messa.Dialog(MSG_OK,("Deleting file: "+dfile).c_str());
					if ( sysFsUnlink(dfile.c_str()) != 0) return "Couldn't delete file "+dfile+"\n"+strerror(errno);
				}
			}
		}
		(void) closedir (dp);
	}
	else return "Couldn't open the directory";
	//Messa.Dialog(MSG_OK,("Deleting folder: "+direct).c_str());
	if ( rmdir(direct.c_str()) != 0) return "Couldn't delete directory "+direct+"\n"+strerror(errno);
	return "";
}

s32 center_text_x(NoRSX *Graphics, int fsize, const char* message)
{
	return (Graphics->width-(strlen(message)*fsize/2))/2;
}

s32 draw_copy(NoRSX *Graphics, std::string title, std::string from, std::string to)
{
	int sizeTitleFont = 30;
	int sizeFont = 18;
	Font F1(LATIN2, Graphics);
	Font F2(LATIN2, Graphics);
	Background B1(Graphics);
	B1.Mono(COLOR_BLACK);
	//Object R1(Graphics);
	//R1.Rectangle(0, 200, Graphics->width, 200, COLOR_BLACK);
	F1.Printf(center_text_x(Graphics, sizeTitleFont, title.c_str()),220, 0xd38900, sizeTitleFont, title.c_str());
	F2.Printf(center_text_x(Graphics, sizeFont, ("From: "+from).c_str()),260,COLOR_WHITE,sizeFont, "%s",("From: "+from).c_str());
	F2.Printf(center_text_x(Graphics, sizeFont, ("To: "+to).c_str()),290,COLOR_WHITE,sizeFont, "%s",("To: "+to).c_str());
	
	Graphics->Flip();

	return 0;
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

std::string correct_path(std::string dpath, int what)
{
	std::string cpath;

	cpath=dpath;
	if (what==1 || what==2) replace(cpath.begin(), cpath.end(), '~', '/');
	if (what==2) if (cpath.find("dev_flash")!=std::string::npos) cpath.replace( cpath.find("dev_flash"), 9, "dev_blind");

	return "/"+cpath;
}

int is_dev_blind_mounted()
{
	const char* MOUNT_POINT = "/dev_blind"; //our mount point
	sysFSStat dir;

	return sysFsStat(MOUNT_POINT, &dir);
}

int mount_dev_blind()
{
	const char* DEV_BLIND = "CELL_FS_IOS:BUILTIN_FLSH1";	// dev_flash
	const char* FAT = "CELL_FS_FAT"; //it's also for fat32
	const char* MOUNT_POINT = "/dev_blind"; //our mount point

	sysFsMount(DEV_BLIND, FAT, MOUNT_POINT, 0);

	return 0;
}

int unmount_dev_blind()
{
	const char* MOUNT_POINT = "/dev_blind"; //our mount point

	sysFsMount(MOUNT_POINT);

	return 0;
}
std::string doit(NoRSX *Graphics, std::string operation, std::string restorefolder, std::string fw, std::string app)
{
	std::string foldername;

	DIR *dp;
	struct dirent *dirp;
	std::string source_paths[100];
	std::string dest_paths[100];
	std::string check_paths[100];
	std::string check_path;
	std::string sourcefile;
	std::string destfile;
	std::string title;
	int findex=0;
	int mountblind=0;
	int dirnotfound=0;
	std::string ret="";

	MsgDialog Messa(Graphics);

	if (operation=="backup")
	{
		foldername=currentDateTime();
		create_dir(mainfolder+"/backups");
		create_dir(mainfolder+"/backups/" + foldername+" Before "+app);
		check_path=mainfolder+"/apps/"+app+"/"+fw_version+"/"+fw;
		dp = opendir (check_path.c_str());
		if (dp == NULL) return "Cannot open directory "+check_path;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				create_dir(mainfolder+"/backups/" + foldername+" Before "+app+"/"+dirp->d_name);
				check_paths[findex]=mainfolder+"/apps/"+app+"/"+fw_version+"/"+fw+"/"+dirp->d_name;
				if (exists(correct_path(dirp->d_name,1).c_str())==0) dirnotfound=1;
				source_paths[findex]=correct_path(dirp->d_name,2);
				if (source_paths[findex].find("dev_blind")!=std::string::npos) mountblind=1;
				dest_paths[findex]=mainfolder+"/backups/" + foldername+" Before "+app+"/"+dirp->d_name;
				findex++;
			}
		}
		closedir(dp);
		title="Backing up files ...";
	}
	else if (operation=="restore")
	{
		check_path=mainfolder+"/backups/"+restorefolder;
		dp = opendir (check_path.c_str());
		if (dp == NULL) return "Cannot open directory "+check_path;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				source_paths[findex]=check_path + "/" + dirp->d_name;
				if (exists(correct_path(dirp->d_name,1).c_str())==0) dirnotfound=1;
				dest_paths[findex]=correct_path(dirp->d_name,2);
				if (dest_paths[findex].find("dev_blind")!=std::string::npos) mountblind=1;
				findex++;
			}
		}
		closedir(dp);
		title="Restoring files ...";
	}
	else if (operation=="install")
	{
		check_path=mainfolder+"/apps/"+app+"/"+fw_version+"/"+fw;
		dp = opendir (check_path.c_str());
		if (dp == NULL) return "Cannot open directory "+check_path;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				source_paths[findex]=check_path + "/" + dirp->d_name;
				if (exists(correct_path(dirp->d_name,1).c_str())==0) dirnotfound=1;
				dest_paths[findex]=correct_path(dirp->d_name,2);
				if (dest_paths[findex].find("dev_blind")!=std::string::npos) mountblind=1;
				if (exists(dest_paths[findex].c_str())==0) dirnotfound=1;
				findex++;
			}
		}
		closedir(dp);
		title="Copying files ...";
	}

	if (dirnotfound==1) return "Couldn't find destination directory";
	if (mountblind==1)
	{
		if (is_dev_blind_mounted()!=0) mount_dev_blind();
		if (is_dev_blind_mounted()!=0) return "Dev_blind not mounted!";
		create_file("/dev_blind/vsh/resource/explore/xmb/xmbmp.cfg");
	}

	//copy files
	for(int j=0;j<findex;j++)
	{
		if (operation=="backup") check_path=check_paths[j];
		else check_path=source_paths[j];
		dp = opendir (check_path.c_str());
		if (dp == NULL) return "Cannot open directory "+check_path;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				sourcefile=source_paths[j]+"/"+dirp->d_name;
				destfile=dest_paths[j]+"/"+dirp->d_name;
				if (!(operation=="backup" && exists(sourcefile.c_str())==0))
				{
					draw_copy(Graphics, title, sourcefile, destfile);
					//sleep(10);
					ret=copy_file(sourcefile.c_str(), destfile.c_str());
					if (ret != "")
					{
						//Implemet rollback in caso of error while copying
						/*if (operation=="backup") //delete backup
						{
							Mess.Dialog(MSG_ERROR,"An error occured during backup. Backup files will be deleted.");
							ret=recursiveDelete(Graphics, mainfolder+"/backups/" + foldername+" Before "+app);
							return ret;
						}
						if (operation=="install") //rollback
						{
							Mess.Dialog(MSG_ERROR,"An error occured during install. Operation will rollback.");
							ret=doit(Graphics,"restore", foldername+" Before "+app, "", "");
							return ret;
						}*/

						return ret;
					}
					//Implement file compare
				}
			}
		}
		closedir(dp);
	}

	return "";
}

s32 draw_menu(NoRSX *Graphics, int menu_id, int selected,int choosed, int menu1_position)
{
	std::string IMAGE_PATH=mainfolder+"/data/images/logo.png";
	int posy=0;

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
	int sizeFont = 25;
	int menu_color;
	Font F1(LATIN2, Graphics);
	Font F2(LATIN2, Graphics);
	std::string menu1_text;

	posy=260;
	if (menu_id==1)
	{
		F1.Printf(center_text_x(Graphics, sizeTitleFont, "INSTALLER MENU"),220, 0xd38900, sizeTitleFont, "INSTALLER MENU");
		for(int j=0;j<menu1_size;j++)
		{
			if (j<menu1_size-1) posy=posy+sizeFont+4;
			else posy=posy+(2*(sizeFont+4));
			if (j==choosed) menu_color=COLOR_RED;
			else if (j==selected) menu_color=COLOR_YELLOW;
			else menu_color=COLOR_WHITE;
			if (j==menu1_size-2 && menu1_restore==0) menu_color=COLOR_GREY;
			if (j<menu1_size-2) menu1_text="INSTALL "+menu1[j];
			else menu1_text=menu1[j];

			F2.Printf(center_text_x(Graphics, sizeFont, menu1_text.c_str()),posy,menu_color,sizeFont, "%s",menu1_text.c_str());
		}
	}
	else if (menu_id==2)
	{
		F1.Printf(center_text_x(Graphics, sizeTitleFont, "CHOOSE A FIRMWARE"),220,	0xd38900, sizeTitleFont, "CHOOSE A FIRMWARE");
		for(int j=0;j<menu2_size[menu1_position];j++)
		{
			if (j<menu2_size[menu1_position]-1) posy=posy+sizeFont+4;
			else posy=posy+(2*(sizeFont+4));
			if (j==choosed) menu_color=COLOR_RED;
			else if (j==selected) menu_color=COLOR_YELLOW;
			else menu_color=COLOR_WHITE;
			F2.Printf(center_text_x(Graphics, sizeFont, menu2[menu1_position][j].c_str()),posy,menu_color,sizeFont, "%s",menu2[menu1_position][j].c_str());
		}
	}
	else if (menu_id==3)
	{
		F1.Printf(center_text_x(Graphics, sizeTitleFont, "CHOOSE A BACKUP TO RESTORE"),220,	0xd38900, sizeTitleFont, "CHOOSE A BACKUP TO RESTORE");
		for(int j=0;j<menu3_size;j++)
		{
			if (j<menu3_size-2) posy=posy+sizeFont+4;
			else posy=posy+(2*(sizeFont+4));
			if (j==choosed) menu_color=COLOR_RED;
			else if (j==selected) menu_color=COLOR_YELLOW;
			else menu_color=COLOR_WHITE;
			F2.Printf(center_text_x(Graphics, sizeFont, menu3[j].c_str()),posy,menu_color,sizeFont, "%s",menu3[j].c_str());
		}
	}
	F2.Printf(center_text_x(Graphics, sizeFont, "Firmware: X.XX (CEX)"),Graphics->height-(sizeFont+20+(sizeFont-5)+10),0xc0c0c0,sizeFont, "Firmware: %s (%s)", fw_version.c_str(), ttype.c_str());
	F2.Printf(center_text_x(Graphics, sizeFont-5, "Installer created by XMBM+ Team"),Graphics->height-((sizeFont-5)+10),0xd38900,sizeFont-5,     "Installer created by XMBM+ Team");
	
	Graphics->Flip();

	return 0;
}


s32 main(s32 argc, char* argv[])
{
	//this is the structure for the pad controllers
	padInfo padinfo;
	padData paddata;

	std::string firmware_choice;
	std::string app_choice;
	std::string direct;
	std::string direct2;
	std::string direct3;
	DIR *dp;
	DIR *dp2;
	DIR *dp3;
	struct dirent *dirp;
	struct dirent *dirp2;
	struct dirent *dirp3;

	int mcount=0;
	char * pch;
	std::string ps3loadtid="PS3LOAD00";

	int i;
	int ifwv=0;
	int ifw=0;
	int iapp=0;
	std::string ret="";
	int menu1_position=0;
	int menu2_position=0;
	int menu3_position=0;

	uint8_t platform_info[0x18];
	lv2_get_platform_info(platform_info);
	uint32_t fw = platform_info[0]* (1 << 16) + platform_info[1] *(1<<8) + platform_info[2];
	uint64_t targettype;
	lv2_get_target_type(&targettype);

	//this will initialize the controller (7= seven controllers)
	ioPadInit (7);
	//this will initialize NoRSX..
	NoRSX *Graphics = new NoRSX(RESOLUTION_1280x720);
	//NoRSX *Graphics = new NoRSX();
	MsgDialog Mess(Graphics);

	//Get main folder
	pch = strtok (argv[0],"/");
	while (pch != NULL)
	{
		if (mcount<4) 
		{
			if (pch==ps3loadtid) mainfolder=mainfolder+"/XMBMANPLS";
			else mainfolder=mainfolder+"/"+pch;
		}
		mcount++;
		pch = strtok (NULL,"/");
	}

	//DETECT FIRMWARE CHANGES WERE
	if (exists("/dev_flash/vsh/resource/explore/xmb/xmbmp.cfg")==0 && exists((mainfolder+"/backups").c_str())==1)
	{
		Mess.Dialog(MSG_OK,"The system detected a firmware change. All previous backups will be deleted.");
		ret=recursiveDelete(Graphics, mainfolder+"/backups");
		if (ret == "") Mess.Dialog(MSG_OK,"All backups deleted!\nPress OK to continue.");
		else Mess.Dialog(MSG_ERROR,("Problem with delete!\n\nError: "+ret).c_str());
	}

	//check if current version is supported
	if (targettype==1) ttype="CEX";
	else if (targettype==2) ttype="DEX";
	else if (targettype==3) ttype="DECR";
	if (fw==0x40250) fw_version="4.25";
	else if (fw==0x40210) fw_version="4.21";
	else if (fw==0x40200) fw_version="4.20";
	else if (fw==0x40110) fw_version="4.11";
	else if (fw==0x30560) fw_version="3.56";
	else if (fw==0x30550) fw_version="3.55";
	else if (fw==0x30410) fw_version="3.41";
	else if (fw==0x30150) fw_version="3.15";

	//make menus arrays
	iapp=0;
	direct=mainfolder+"/apps";
	dp = opendir (direct.c_str());
	if (dp == NULL) return 0;
	while ( (dirp = readdir(dp) ) )
	{
		if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
		{
			//second menu
			ifwv=0;
			direct2=direct+"/"+dirp->d_name;
			dp2 = opendir (direct2.c_str());
			if (dp2 == NULL) return 0;
			while ( (dirp2 = readdir(dp2) ) )
			{
				if ( strcmp(dirp2->d_name, ".") != 0 && strcmp(dirp2->d_name, "..") != 0 && strcmp(dirp2->d_name, "") != 0)
				{
					if (strcmp(dirp2->d_name, fw_version.c_str())==0 || strcmp(dirp2->d_name, "All")==0)
					{
						//debug_print(Graphics, (std::string)dirp->d_name+" "+(std::string)dirp2->d_name+" "+fw_version);
						ifw=0;
						direct3=direct2+"/"+dirp2->d_name;
						dp3 = opendir (direct3.c_str());
						if (dp3 == NULL) return 0;
						while ( (dirp3 = readdir(dp3) ) )
						{
							if ( strcmp(dirp3->d_name, ".") != 0 && strcmp(dirp3->d_name, "..") != 0 && strcmp(dirp3->d_name, "") != 0)
							{
								menu2[iapp][ifw]=dirp3->d_name;
								ifw++;
							}
						}
						closedir(dp3);
						if (ifw!=0) //has apps for the current firmware
						{
							menu2[iapp][ifw]="Back to main menu";
							ifw++;
							menu2_size[iapp]= ifw;
						}
						ifwv++;
					}
				}
			}
			closedir(dp2);
			if (ifwv!=0) //has apps for the current firmware version
			{
				menu1[iapp]=dirp->d_name;
				iapp++;
			}
		}
	}
	closedir(dp);
	if (iapp!=0)
	{
		menu1[iapp]="RESTORE a backup";
		iapp++;
		menu1[iapp]="Exit to XMB";
		iapp++;
		menu1_size=iapp;
	}
	else
	{
		Mess.Dialog(MSG_ERROR,"Your firmware version is not supported.");
		goto end;
	}

	//Start menu
	menu1_position=0;
	
	menu_1:
	if (opendir ((mainfolder+"/backups").c_str()) == NULL)
	{
		menu1_restore=0;
		if (menu1_position==menu1_size-2) menu1_position--;
	}
	else menu1_restore=1;
	draw_menu(Graphics,1,menu1_position,-1,0);
	while (1)
	{
		ioPadGetInfo (&padinfo);
		//this will wait for a START from any pad
		for(i = 0; i < MAX_PADS; i++)
		{
			if (padinfo.status[i])
			{
				ioPadGetData (i, &paddata);
				//if (paddata.BTN_START) goto end;
				if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
				{
					if (menu1_position<menu1_size-1)
					{
						menu1_position++;
						if (menu1_position==menu1_size-2 && menu1_restore==0) menu1_position++;
						goto menu_1;
					}
					else
					{
						menu1_position=0;
						goto menu_1;
					}
				}
				if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
				{
					if (menu1_position>0)
					{
						menu1_position--;
						if (menu1_position==menu1_size-2 && menu1_restore==0) menu1_position--;
						goto menu_1;
					}
					else
					{
						menu1_position=menu1_size-1;
						goto menu_1;
					}
				}
				if (paddata.BTN_CROSS)
				{
					draw_menu(Graphics,1,-1,menu1_position,0);
					sleep(0.05);
					if (menu1_position<menu1_size-2)
					{
						app_choice=menu1[menu1_position];
						if (menu2[menu1_position][0]=="All Firmwares" && menu2_size[menu1_position]==2)
						{
							Mess.Dialog(MSG_YESNO,("Are you sure you want to install "+app_choice+"?\n\nPlease be adviced that this process can take a while (depending on the files size) and can change dev_flash files so don't turn off your PS3 while the process in running.").c_str());
							if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
							{
								firmware_choice=menu2[menu1_position][0];
								ret="";
								ret=doit(Graphics,"backup", "-", firmware_choice, app_choice);
								if (ret == "")
								{
									ret=doit(Graphics,"install", "-", firmware_choice, app_choice);
									if (ret == "")
									{
										if (is_dev_blind_mounted()==0)
										{
											Mess.Dialog(MSG_OK,"Installed!\nPress OK to reboot.");
											goto end_with_reboot;
										}
										else Mess.Dialog(MSG_OK,"Installed!\nPress OK to continue.");
									}
								}
								else Mess.Dialog(MSG_ERROR,("Not installed!\n\nError: "+ret).c_str());
								draw_menu(Graphics,1,-1,menu1_position,0);
								sleep(0.05);
								goto menu_1;
							}
							else goto menu_1;
						}
						else goto continue_to_menu2;
					}
					else if (menu1_position<menu1_size-1)
					{
						goto continue_to_menu3;
					}
					else if (menu1_position<menu1_size)
					{
						goto end;
					}
				}
			}
		}
	}

	continue_to_menu2:
	menu2_position=0;
	menu_2:
	draw_menu(Graphics,2,menu2_position,-1,menu1_position);
	while (1)
	{
		ioPadGetInfo (&padinfo);
		//this will wait for a START from any pad
		for(i = 0; i < MAX_PADS; i++)
		{
			if (padinfo.status[i])
			{
				ioPadGetData (i, &paddata);
				//if (paddata.BTN_START) goto end;
				if (paddata.BTN_CIRCLE) goto menu_1;
				if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
				{
					if (menu2_position<menu2_size[menu1_position]-1)
					{
						menu2_position++;
						goto menu_2;
					}
					else
					{
						menu2_position=0;
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
					else
					{
						menu2_position=menu2_size[menu1_position]-1;
						goto menu_2;
					}
				}
				if (paddata.BTN_CROSS)
				{
					if (menu2_position<menu2_size[menu1_position]-1)
					{
						draw_menu(Graphics,2,-1,menu2_position,menu1_position);
						sleep(0.05);
						Mess.Dialog(MSG_YESNO,("Are you sure you want to install "+app_choice+"?\n\nPlease be adviced that this process can take a while (depending on the files size) and can change dev_flash files so don't turn off your PS3 while the process in running.").c_str());
						if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
						{
							firmware_choice=menu2[menu1_position][menu2_position];
							ret="";
							ret=doit(Graphics,"backup", "-", firmware_choice, app_choice);
							if (ret == "")
							{
								ret=doit(Graphics,"install", "-", firmware_choice, app_choice);
								if (ret == "")
								{
									if (is_dev_blind_mounted()==0)
									{
										Mess.Dialog(MSG_OK,"Installed!\nPress OK to reboot.");
										goto end_with_reboot;
									}
									else Mess.Dialog(MSG_OK,"Installed!\nPress OK to continue.");
								}
							}
							else Mess.Dialog(MSG_ERROR,("Not installed!\n\nError: "+ret).c_str());
							draw_menu(Graphics,2,-1,menu2_position,menu1_position);
							sleep(0.05);
							goto menu_2;
						}
						else goto menu_2;
					}
					else
					{
						draw_menu(Graphics,2,-1,menu2_position,menu1_position);
						sleep(0.05);
						goto menu_1;
					}
				}
			}
		}
		
	}

	continue_to_menu3:
	menu3_size=0;
	direct=mainfolder+"/backups";
	dp = opendir (direct.c_str());
	if (dp == NULL) return 0;
	while ( (dirp = readdir(dp) ) )
	{
		if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
		{
			menu3[menu3_size]=dirp->d_name;
			menu3_size++;
		}
	}
	closedir(dp);
	menu3[menu3_size]="DELETE all backups";
	menu3_size++;
	menu3[menu3_size]="Back to main menu";
	menu3_size++;
	menu3_position=0;
	menu_3:
	draw_menu(Graphics,3,menu3_position,-1,0);
	while (1)
	{
		ioPadGetInfo (&padinfo);
		//this will wait for a START from any pad
		for(i = 0; i < MAX_PADS; i++)
		{
			if (padinfo.status[i])
			{
				ioPadGetData (i, &paddata);
				//if (paddata.BTN_START) goto end;
				if (paddata.BTN_CIRCLE) goto menu_1;
				if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
				{
					if (menu3_position<menu3_size-1)
					{
						menu3_position++;
						goto menu_3;
					}
					else
					{
						menu3_position=0;
						goto menu_3;
					}
				}
				if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
				{
					if (menu3_position>0)
					{
						menu3_position--;
						goto menu_3;
					}
					else
					{
						menu3_position=menu3_size-1;
						goto menu_3;
					}
				}
				if (paddata.BTN_CROSS)
				{
					draw_menu(Graphics,3,-1,menu3_position,0);
					sleep(0.05);
					if (menu3_position<menu3_size-2) //Restore a backup
					{
						Mess.Dialog(MSG_YESNO,("Are you sure you want to restore "+menu3[menu3_position]+" backup?\n\nPlease be adviced that this process can take a while (depending on the files size) and can change dev_flash files so don't turn off your PS3 while the process in running.").c_str());
						if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
						{
							ret="";
							ret=doit(Graphics,"restore", menu3[menu3_position], "", "");
							if (ret == "")
							{
								if (is_dev_blind_mounted()==0)
								{
									Mess.Dialog(MSG_OK,"Backup restored!\nPress OK to reboot.");
									goto end_with_reboot;
								}
								else Mess.Dialog(MSG_OK,"Backup restored!\nPress OK to continue.");
							}
							else Mess.Dialog(MSG_ERROR,("Backup not restored!\n\nError: "+ret).c_str());
							draw_menu(Graphics,3,-1,menu3_position,0);
							sleep(0.05);
							goto menu_3;
						}
						else goto menu_3;
					}
					else if (menu3_position<menu3_size-1) //Delete all backups
					{
						Mess.Dialog(MSG_YESNO,"Are you sure you want to delete all backups?\n\nPlease be adviced that this process can take a while (depending on the files size) and can change dev_flash files so don't turn off your PS3 while the process in running.");
						if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
						{
							ret="";
							ret=recursiveDelete(Graphics, mainfolder+"/backups");
							if (ret == "")
							{
								draw_menu(Graphics,3,menu3_position,-1,0);
								Mess.Dialog(MSG_OK,"All backups deleted!\nPress OK to continue.");
								goto menu_1;
							}
							Mess.Dialog(MSG_ERROR,("Problem with delete!\n\nError: "+ret).c_str());
							draw_menu(Graphics,3,-1,menu3_position,0);
							sleep(0.05);
							goto menu_3;
						}
						else goto menu_3;
					}
					else //Return to Main Menu
					{
						goto menu_1;
					}
				}
			}
		}
		
	}


	end_with_reboot:
	{
		//This will uninit the NoRSX lib
		Graphics->NoRSX_Exit();
		//this will uninitialize the controllers
		ioPadEnd();
		if (is_dev_blind_mounted()==0) unmount_dev_blind();
		reboot_sys(); //reboot
	}

	end:
	{
		//This will uninit the NoRSX lib
		Graphics->NoRSX_Exit();
		//this will uninitialize the controllers
		ioPadEnd();
		if (is_dev_blind_mounted()==0) unmount_dev_blind();
	}
	return 0;
}
