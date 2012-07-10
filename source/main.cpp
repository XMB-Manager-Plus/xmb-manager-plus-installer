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
#include <time.h>
#include <errno.h>

#define MAX_BUFFERS 2

std::string mainfolder;
std::string fw_version="";
int fw_version_index=-1;
std::string menu1[10];
int menu1_size=0;
int menu1_restore=1;
std::string menu2[10][10];
std::string menu2_fwv[10];
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

std::string copy_file(const char* cfrom, const char* ctoo)
{
  FILE *from, *to;
  char ch;

  /* open source file */
  if((from = fopen(cfrom, "rb"))==NULL) return "Cannot open source file ("+(std::string)cfrom+") for reading!";

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

std::string doit(NoRSX *Graphics, std::string operation, std::string restorefolder, std::string fw, std::string app)
{
	const char* DEV_BLIND = "CELL_FS_IOS:BUILTIN_FLSH1";	// dev_flash
	const char* FAT = "CELL_FS_FAT"; //it's also for fat32
	const char* MOUNT_POINT = "/dev_blind"; //our mount point

	//we may need to check if it is already mounted:
	sysFSStat dir;

	std::string foldername;

	DIR *dp;
	struct dirent *dirp;

	std::string flash[]={"rco","xml","sprx"};
	std::string flash_paths[]={(std::string)MOUNT_POINT+"/vsh/resource", (std::string)MOUNT_POINT+"/vsh/resource/explore/xmb", (std::string)MOUNT_POINT+"/vsh/module"};
	std::string source_paths[3];
	std::string dest_paths[3];
	std::string check_paths[3];
	std::string check_path;
	std::string sourcefile;
	std::string destfile;

	std::string ret="";
	MsgDialog Messa(Graphics);

	int is_mounted = sysFsStat(MOUNT_POINT, &dir);
	if (is_mounted != 0) sysFsMount(DEV_BLIND, FAT, MOUNT_POINT, 0);
	is_mounted = sysFsStat(MOUNT_POINT, &dir);
	if (is_mounted != 0) return "Dev_blind not mounted!";
	if (operation=="backup")
	{
		create_file((flash_paths[1]+"/xmbmp.cfg").c_str());
		foldername=currentDateTime();
		create_dir(mainfolder+"/backups");
		create_dir(mainfolder+"/backups/" + foldername+" Before "+app);
		for(int j=0;j<3;j++)
		{
			create_dir(mainfolder+"/backups/" + foldername+" Before "+app+"/"+flash[j]);
			create_dir(mainfolder+"/resources/"+fw_version+"/"+fw+"/"+app+"/"+flash[j]);
			source_paths[j]=flash_paths[j];
			check_paths[j]=mainfolder+"/resources/"+fw_version+"/"+fw+"/"+app+"/"+flash[j];
			dest_paths[j]=mainfolder+"/backups/"+foldername+" Before "+app+"/"+flash[j];
		}
	}
	else if (operation=="restore")
	{
		for(int j=0;j<3;j++)
		{
			source_paths[j]=mainfolder+"/backups/"+restorefolder+"/"+flash[j];
			dest_paths[j]=flash_paths[j];
		}
	}
	else if (operation=="install")
	{
		for(int j=0;j<3;j++)
		{
			create_dir(mainfolder+"/resources/"+fw_version+"/"+fw+"/"+app+"/"+flash[j]);
			source_paths[j]=mainfolder+"/resources/"+fw_version+"/"+fw+"/"+app+"/"+flash[j];
			dest_paths[j]=flash_paths[j];
		}
	}
	
	//copy files
	for(int j=0;j<3;j++)
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
				ret=copy_file(sourcefile.c_str(), destfile.c_str());
				//Messa.Dialog(MSG_OK,("Operation: "+operation+"\n\nopen: "+check_path+"\n\nfrom: "+sourcefile+"\nto: "+destfile+"\n\nreturn: "+ret).c_str());
				if (ret != "") return ret;
			}
		}
		closedir(dp);
	}

	return "";
}

s32 center_text_x(NoRSX *Graphics, int fsize, const char* message)
{
	return (Graphics->width-(strlen(message)*fsize/2))/2;
}

s32 draw_menu(NoRSX *Graphics, int menu_id, int selected,int choosed, std::string status)
{
	std::string IMAGE_PATH=mainfolder+"/data/images/xmbm_transparent.png";
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
		F1.Printf(center_text_x(Graphics, sizeTitleFont, "CHOOSE WHAT TO INSTALL"),220, 0xd38900, sizeTitleFont, "CHOOSE WHAT TO INSTALL");
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
		for(int j=0;j<menu2_size[fw_version_index];j++)
		{
			if (j<menu2_size[fw_version_index]-1) posy=posy+sizeFont+4;
			else posy=posy+(2*(sizeFont+4));
			if (j==choosed) menu_color=COLOR_RED;
			else if (j==selected) menu_color=COLOR_YELLOW;
			else menu_color=COLOR_WHITE;
			F2.Printf(center_text_x(Graphics, sizeFont, menu2[fw_version_index][j].c_str()),posy,menu_color,sizeFont, "%s",menu2[fw_version_index][j].c_str());
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
	u32 textX =(Graphics->width/2)-230;
	F2.Printf(textX,posy+2*(sizeFont+4),0xc0c0c0,sizeFont,     "Firmware version: %s", fw_version.c_str());
	F2.Printf(textX+300,posy+2*(sizeFont+4),0xc0c0c0,sizeFont,     "Status: %s", status.c_str());
	
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
	std::string dfile;
	std::string direct;
	DIR *dp;
	struct dirent *dirp;

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

	//fetch available firmwares versions
	direct=mainfolder+"/resources";
	dp = opendir (direct.c_str());
	if (dp == NULL) return 0;
	while ( (dirp = readdir(dp) ) )
	{
		dfile = direct + "/" + dirp->d_name;
		if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
		{
			menu2_fwv[ifwv]=dirp->d_name;
			ifwv++;
		}
	}
	closedir(dp);

	//check if current version is supported
	if (fw==0x30560) fw_version="3.56";
	else if (fw==0x30550) fw_version="3.55";
	else if (fw==0x30410) fw_version="3.41";
	else if (fw==0x30150) fw_version="3.15";
	for (int h=0; h<ifwv; h++)
	{
		if (fw_version==menu2_fwv[h]) fw_version_index=h;
	}
	if (fw_version=="" || fw_version_index==-1)
	{
		Mess.Dialog(MSG_ERROR,"Your firmware version is not supported.");
		goto end;
	}

	//fetch available firmwares per version
	for (int h=0; h<ifwv; h++)
	{
		ifw=0;
		direct=mainfolder+"/resources/"+menu2_fwv[h];
		dp = opendir (direct.c_str());
		if (dp == NULL) return 0;
		while ( (dirp = readdir(dp) ) )
		{
			dfile = direct + "/" + dirp->d_name;
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				menu2[h][ifw]=dirp->d_name;
				ifw++;
			}
		}
		closedir(dp);
		menu2[h][ifw]="Back to main menu";
		ifw++;
		menu2_size[h]= ifw;
	}

	//fetch available apps
	iapp=0;
	direct=mainfolder+"/resources/"+menu2_fwv[0]+"/"+menu2[0][0];
	dp = opendir (direct.c_str());
	if (dp == NULL) return 0;
	while ( (dirp = readdir(dp) ) )
	{
		dfile = direct + "/" + dirp->d_name;
		if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
		{
			menu1[iapp]=dirp->d_name;
			iapp++;
		}
	}
	closedir(dp);
	menu1[iapp]="RESTORE a backup";
	iapp++;
	menu1[iapp]="Exit to XMB";
	iapp++;
	menu1_size= iapp;

	//Start second menu
	menu1_position=0;
	
	menu_1:
	if (opendir ((mainfolder+"/backups").c_str()) == NULL) menu1_restore=0;
	else menu1_restore=1;
	draw_menu(Graphics,1,menu1_position,-1,"Waiting");
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
						if (menu1_position==2 && menu1_restore==0) menu1_position++;
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
						if (menu1_position==2 && menu1_restore==0) menu1_position--;
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
					draw_menu(Graphics,1,-1,menu1_position,"Waiting");
					sleep(0.05);
					if (menu1_position<menu1_size-2)
					{
						app_choice=menu1[menu1_position];
						goto continue_to_menu2;
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
				//if (paddata.BTN_START) goto end;
				if (paddata.BTN_CIRCLE) goto menu_1;
				if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
				{
					if (menu2_position<menu2_size[fw_version_index]-1)
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
						menu2_position=menu2_size[fw_version_index]-1;
						goto menu_2;
					}
				}
				if (paddata.BTN_CROSS)
				{
					if (menu2_position<menu2_size[fw_version_index]-1)
					{
						draw_menu(Graphics,2,-1,menu2_position,"Waiting");
						sleep(0.05);
						Mess.Dialog(MSG_YESNO,("Are you sure you want to install "+app_choice+"?\n\nPlease be adviced that this process takes a while and changes dev_flash files so don't turn off your PS3 while the process in running.").c_str());
						if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
						{
							firmware_choice=menu2[fw_version_index][menu2_position];
							draw_menu(Graphics,2,menu2_position,-1,"Making backup...");
							ret="";
							ret=doit(Graphics,"backup", "-", firmware_choice, app_choice);
							if (ret == "")
							{
								draw_menu(Graphics,2,menu2_position,-1,"Installing...");
								ret=doit(Graphics,"install", "-", firmware_choice, app_choice);
								if (ret == "")
								{
									draw_menu(Graphics,2,menu2_position,-1,"Waiting");
									Mess.Dialog(MSG_OK,"Installed!\nPress OK to reboot.");
									//draw_menu(Graphics,2,-1,menu2_position,"Installed!");
									//sleep(2);
									//draw_menu(Graphics,2,-1,menu2_position,"Rebooting...");
									//sleep(2);
									goto end_with_reboot;
								}
							}
							Mess.Dialog(MSG_ERROR,("Not installed!\n\nError: "+ret).c_str());
							draw_menu(Graphics,2,-1,menu2_position,"Waiting");
							sleep(0.05);
							goto menu_2;
						}
						else goto menu_2;
					}
					else
					{
						draw_menu(Graphics,2,-1,menu2_position,"Waiting");
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
		dfile = direct + "/" + dirp->d_name;
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
	draw_menu(Graphics,3,menu3_position,-1,"Waiting");
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
					draw_menu(Graphics,3,-1,menu3_position,"Waiting");
					sleep(0.05);
					if (menu3_position<menu3_size-2) //Restore a backup
					{
						Mess.Dialog(MSG_YESNO,("Are you sure you want to restore "+menu3[menu3_position]+" backup?\n\nPlease be adviced that this process takes a while and changes dev_flash files so don't turn off your PS3 while the process in running.").c_str());
						if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
						{
							draw_menu(Graphics,3,menu3_position,-1,"Restoring...");
							ret="";
							ret=doit(Graphics,"restore", menu3[menu3_position], "", "");
							if (ret == "")
							{
								draw_menu(Graphics,3,menu3_position,-1,"Waiting");
								Mess.Dialog(MSG_OK,"Backup restored!\nPress OK to reboot.");
								goto end_with_reboot;
							}
							Mess.Dialog(MSG_ERROR,("Backup not restored!\n\nError: "+ret).c_str());
							draw_menu(Graphics,3,-1,menu3_position,"Waiting");
							sleep(0.05);
							goto menu_3;
						}
						else goto menu_3;
					}
					else if (menu3_position<menu3_size-1) //Delete all backups
					{
						Mess.Dialog(MSG_YESNO,"Are you sure you want to delete all backups?\n\nPlease be adviced that this process takes a while and deletes files in your hdd files so don't turn off your PS3 while the process in running.");
						if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
						{
							draw_menu(Graphics,3,menu3_position,-1,"Deleting...");
							ret="";
							ret=recursiveDelete(Graphics, mainfolder+"/backups");
							if (ret == "")
							{
								draw_menu(Graphics,3,menu3_position,-1,"Waiting");
								Mess.Dialog(MSG_OK,"All backups deleted!\nPress OK to continue.");
								goto menu_1;
							}
							Mess.Dialog(MSG_ERROR,("Problem with delete!\n\nError: "+ret).c_str());
							draw_menu(Graphics,3,-1,menu3_position,"Waiting");
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
		lv2syscall4(379,0x1200,0,0,0); //reboot
	}

	end:
	{
		//This will uninit the NoRSX lib
		Graphics->NoRSX_Exit();
		//this will uninitialize the controllers
		ioPadEnd();
	}
	return 0;
}
