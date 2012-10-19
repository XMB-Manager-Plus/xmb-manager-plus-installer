#include "xmbmp-graphics.h"
#include "xmbmp-mount-unmount.h"
//#include "xmbmp-debug.h"
#include "xmbmp-file.h"

//global vars
string mainfolder;
string fw_version="";
string ttype="";
string menu1[99];
int menu1_size=0;
string menu2[99][99];
string menu2_path[99][99];
int menu2_size[99];
string menu3[99];
int menu3_size=0;

//headers
const string currentDateTime();
//bool replace(string& str, const string& from, const string& to);
string doit(string operation, string foldername, string fw_folder, string app);
int restore(string foldername);
int install(string firmware_folder, string app_choice);
int delete_all();
int delete_one(string foldername, string type);
void get_firmware_info();
void draw_menu(int menu_id, int selected,int choosed, int menu1_position, int menu1_restore);
int make_menu_to_array(int whatmenu);
int menu_restore_available();
s32 main(s32 argc, char* argv[]);

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const string currentDateTime()
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

/*bool replace(string& str, const string& from, const string& to)
{
    size_t start_pos = str.find(from);
    if(start_pos == string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}*/

string doit(string operation, string foldername, string fw_folder, string app)
{
	DIR *dp;
	struct dirent *dirp;
	string source_paths[100];
	string dest_paths[100];
	string check_paths[100];
	string check_path;
	string sourcefile;
	string destfile;
	string filename;
	string dest;
	string source;
	string title;
	int findex=0;
	int mountblind=0;
	double copy_totalsize=0;
	double copy_currentsize=0;
	double source_size=0;
	double dest_size=0;
	double freespace_size=0;
	int numfiles_total=0;
	int numfiles_current=0;
	unsigned int pos = 0;
	int j=0;
	int i=0;
	string ret="";
	string *files_list = NULL;  //Pointer for an array to hold the filenames.
	string *final_list_source = NULL;  //Pointer for an array to hold the filenames.
	string *final_list_dest = NULL;  //Pointer for an array to hold the filenames.

	if (operation=="backup")
	{
		//create_dir(mainfolder+"/backups");
		//create_dir(mainfolder+"/backups/" + foldername);
		check_path=mainfolder+"/apps/"+app+"/"+fw_folder;
		dp = opendir (check_path.c_str());
		if (dp == NULL) return "Cannot open directory "+check_path;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				if (dirp->d_type == DT_DIR)
				{
					//create_dir(mainfolder+"/backups/"+foldername+"/"+dirp->d_name);
					check_paths[findex]=check_path+"/"+dirp->d_name;
					source_paths[findex]=correct_path(dirp->d_name,2);
					dest_paths[findex]=mainfolder+"/backups/"+foldername+"/"+dirp->d_name;
					if (source_paths[findex].find("dev_blind")!=string::npos) mountblind=1;
					findex++;
				}
				/*else if (dirp->d_type == DT_REG && dirp->d_name.find(".zip")!=string::npos)//check zip files
				{
					ret=string z_inflate(FILE *source, FILE *dest);
					if (ret != "") return ret;
				}*/
			}
		}
		closedir(dp);
		title="Backing up files ...";
	}
	else if (operation=="restore")
	{
		check_path=mainfolder+"/backups/"+foldername;
		dp = opendir (check_path.c_str());
		if (dp == NULL) return "Cannot open directory "+check_path;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0 && dirp->d_type == DT_DIR)
			{
				source_paths[findex]=check_path + "/" + dirp->d_name;
				dest_paths[findex]=correct_path(dirp->d_name,2);
				if (dest_paths[findex].find("dev_blind")!=string::npos) mountblind=1;
				findex++;
			}
		}
		closedir(dp);
		title="Restoring files ...";
	}
	else if (operation=="install")
	{
		check_path=mainfolder+"/apps/"+app+"/"+fw_folder;
		dp = opendir (check_path.c_str());
		if (dp == NULL) return "Cannot open directory "+check_path;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0 && dirp->d_type == DT_DIR)
			{
				source_paths[findex]=check_path + "/" + dirp->d_name;
				dest_paths[findex]=correct_path(dirp->d_name,2);
				if (dest_paths[findex].find("dev_blind")!=string::npos) mountblind=1;
				findex++;
			}
			//check zip files
		}
		closedir(dp);
		title="Copying files ...";
	}

	if (mountblind==1)
	{
		if (is_dev_blind_mounted()!=0) mount_dev_blind();
		if (is_dev_blind_mounted()!=0) return "Dev_blind not mounted!";
		if (exists("/dev_flash/vsh/resource/explore/xmb/xmbmp.cfg")!=1) create_file("/dev_blind/vsh/resource/explore/xmb/xmbmp.cfg");
	}

	//count files
	final_list_source = new string[5000];
	final_list_dest = new string[5000];
	for(j=0;j<findex;j++)
	{
		if (operation=="backup") check_path=check_paths[j];
		else check_path=source_paths[j];
		//Mess.Dialog(MSG_OK,("check_path: "+check_path).c_str());
		files_list=recursiveListing(check_path);
		i=0;
		while (strcmp(files_list[i].c_str(),"") != 0)
		{
			files_list[i].replace(files_list[i].find(check_path), check_path.size()+1, "");
			sourcefile=source_paths[j]+"/"+files_list[i];
			destfile=dest_paths[j]+"/"+files_list[i];
			//Mess.Dialog(MSG_OK,(operation+"\nsource: "+sourcefile+"\ndest:"+destfile).c_str());
			if (!(operation=="backup" && exists(sourcefile.c_str())!=1))
			{
				copy_totalsize+=get_filesize(sourcefile.c_str());
				final_list_source[numfiles_total]=sourcefile;
				final_list_dest[numfiles_total]=destfile;
				filename=final_list_source[i].substr(final_list_source[i].find_last_of("/")+1);
				source=final_list_source[i].substr(0,final_list_source[i].find_last_of("/")+1);
				dest=final_list_dest[i].substr(0,final_list_dest[i].find_last_of("/")+1);
				//Mess.Dialog(MSG_OK,(operation+"\nsource: "+final_list_source[numfiles_total]+"\ndest:"+final_list_dest[numfiles_total]).c_str());
				if (dest.find("dev_blind")!=string::npos) mountblind=1;
				numfiles_total+=1;
			}
			i++;
		}
	}

	//copy files
	i=0;
	while (strcmp(final_list_source[i].c_str(),"") != 0)
	{
		sourcefile=final_list_source[i];
		destfile=final_list_dest[i];
		filename=final_list_source[i].substr(final_list_source[i].find_last_of("/")+1);
		source=final_list_source[i].substr(0,final_list_source[i].find_last_of("/")+1);
		dest=final_list_dest[i].substr(0,final_list_dest[i].find_last_of("/")+1);
		source_size=get_filesize(sourcefile.c_str());
		dest_size=get_filesize(destfile.c_str());
		freespace_size=get_free_space(dest.c_str())+dest_size;
		if (source_size >= freespace_size) ret="Not enough space to copy the file ("+filename+") to destination path ("+dest+").";
		else
		{
			if (exists(dest.c_str())!=1)
			{
				pos = 0;
				do
				{
					pos=dest.find_first_of("/", pos+1);
					sourcefile=dest.substr(0, pos+1);
					//Mess.Dialog(MSG_OK,("folder: "+sourcefile+" "+int_to_string(pos)+" "+int_to_string((int)dest.size()-1)).c_str());
					if (exists(sourcefile.c_str())!=1) create_dir(sourcefile);
				}
				while (pos != dest.size()-1);
			}
			ret=copy_file(title, source.c_str(), dest.c_str(), filename.c_str(), copy_currentsize, copy_totalsize, numfiles_current, numfiles_total,0);
		}
		if (ret != "") return ret;
		copy_currentsize+=source_size;
		numfiles_current+=1;
		i++;
	}

	//check files
	i=0;
	copy_currentsize=0;
	numfiles_current=0;
	while (strcmp(final_list_source[i].c_str(),"") != 0)
	{
		sourcefile=final_list_source[i];
		destfile=final_list_dest[i];
		filename=final_list_source[i].substr(final_list_source[i].find_last_of("/")+1);
		source=final_list_source[i].substr(0,final_list_source[i].find_last_of("/")+1);
		dest=final_list_dest[i].substr(0,final_list_dest[i].find_last_of("/")+1);
		ret=copy_file("Checking files ...", source.c_str(), dest.c_str(), filename.c_str(), copy_currentsize, copy_totalsize, numfiles_current, numfiles_total,1);
		if (ret != "") return ret;
		copy_currentsize+=source_size;
		numfiles_current+=1;
		i++;
	}

	return "";
}

int restore(string foldername)
{
	string ret="";
	string problems="\n\nPlease be adviced that, depending on what you choose, this process can change /dev_flash files so DON'T TURN OFF YOUR PS3 and DON'T GO TO GAME MENU while the process in running.\n\nIf you have some corruption after copying the files or the installer quits unexpectly check all files before restarting and if possible reinstall the firmware from XMB or Recovery Menu.";
	
	Mess.Dialog(MSG_YESNO_DYES,("Are you sure you want to restore '"+foldername+"' backup?"+problems).c_str());
	if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
	{
		ret=doit("restore", foldername, "", "");
		if (ret == "") //restore success
		{
			if (is_dev_blind_mounted()==0)
			{
				unmount_dev_blind();
				Mess.Dialog(MSG_YESNO_DYES, ("Backup '"+foldername+"' has restored with success.\nYou have changed /dev_flash files, do you want to reboot?").c_str());
				if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1) return 2;
				else return 1;
			}
			Mess.Dialog(MSG_OK,("Backup '"+foldername+"' has restored with success.\nPress OK to continue.").c_str());
			return 1;
		}
		else //problem in the restore process so emit a warning
		{
			Mess.Dialog(MSG_ERROR,("Backup '"+foldername+"' has not restored! A error occured while restoring the backup!\n\nError: "+ret+"\n\nTry to restore again manually, if the error persists, your system may be corrupted, please check all files and if needed reinstall firmare from XMB or recovery menu.").c_str());
		}
	}

	return 0;
}

int install(string firmware_folder, string app_choice)
{
	string ret="";
	string problems="\n\nPlease be adviced that, depending on what you choose, this process can change dev_flash files so DON'T TURN OFF YOUR PS3 and DON'T GO TO GAME MENU while the process in running.\n\nIf you have some corruption after copying the files or the installer quits unexpectly check all files before restarting and if possible reinstall the firmware from XMB or Recovery Menu.";
	string foldername=currentDateTime()+" Before "+app_choice;

	Mess.Dialog(MSG_YESNO_DNO,("Are you sure you want to install '"+app_choice+"'?"+problems).c_str());
	if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
	{
		ret=doit("backup", foldername, firmware_folder, app_choice);
		if (ret == "") //backup success
		{
			ret=doit("install", "", firmware_folder, app_choice);
			if (ret == "") //copy success
			{
				if (is_dev_blind_mounted()==0)
				{
					unmount_dev_blind();
					Mess.Dialog(MSG_YESNO_DYES, ("'"+app_choice+"' has installed with success.\nYou have changed /dev_flash files, do you want to reboot?").c_str());
					if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1) return 2;
					else return 1;
				}
				Mess.Dialog(MSG_OK,("'"+app_choice+"' has installed with success.\nPress OK to continue.").c_str());
				return 1;
			}
			else //problem in the copy process so rollback by restoring the backup
			{
				Mess.Dialog(MSG_ERROR,("'"+app_choice+"' has not installed! A error occured while copying files!\n\nError: "+ret+"\n\nBackup will be restored.").c_str());
				return restore(foldername);
			}
		}
		else //problem in the backup process so rollback by deleting the backup
		{
			Mess.Dialog(MSG_ERROR,("'"+app_choice+"' has not installed! A error occured while doing backuping the files!\n\nError: "+ret+"\n\nIncomplete backup will be deleted.").c_str());
			if (recursiveDelete(mainfolder+"/backups/"+foldername) != "") Mess.Dialog(MSG_ERROR,("Problem while deleting the backup!\n\nError: "+ret+"\n\nTry to delete with a file manager.").c_str());
		}
	}

	return 0;
}

int delete_all()
{
	string ret="";
	string problems="\n\nPlease DON'T TURN OFF YOUR PS3 and DON'T GO TO GAME MENU while the process in running.";

	Mess.Dialog(MSG_YESNO_DNO,("Are you sure you want to delete all backups?"+problems).c_str());
	if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
	{
		ret=recursiveDelete(mainfolder+"/backups");
		if (ret == "") //delete sucess
		{
			Mess.Dialog(MSG_OK,"All backups deleted!\nPress OK to continue.");
			return 1;
		}
		else //problem in the delete process so emit a warning
		{
			Mess.Dialog(MSG_ERROR,("Backup folders were not deleted! A error occured while deleting the folders!\n\nError: "+ret+"\n\nTry to delete again manually, if the error persists, try other software to delete this folders.").c_str());
		}
	}
	return 0;
}

int delete_one(string foldername, string type)
{
	string ret="";
	string problems="\n\nPlease DON'T TURN OFF YOUR PS3 and DON'T GO TO GAME MENU while the process in running.";

	Mess.Dialog(MSG_YESNO_DNO,("Are you sure you want to delete the "+type+" '"+foldername+"'?"+problems).c_str());
	if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
	{
		if (strcmp(type.c_str(), "backup")==0) ret=recursiveDelete(mainfolder+"/backups/"+foldername);
		else if (strcmp(type.c_str(), "app")==0) ret=recursiveDelete(mainfolder+"/apps/"+foldername);
		if (ret == "") //delete sucess
		{
			Mess.Dialog(MSG_OK,("The "+type+" '"+foldername+"' has been deleted!\nPress OK to continue.").c_str());
			return 1;
		}
		else //problem in the delete process so emit a warning
		{
			Mess.Dialog(MSG_ERROR,("A error occured while deleting the "+type+" '"+foldername+"'!\n\nError: "+ret+"\n\nTry to delete again manually, if the error persists, try other software to delete this folders.").c_str());
		}
	}
	return 0;
}

void get_firmware_info()
{
	uint8_t platform_info[0x18];
	lv2_get_platform_info(platform_info);
	uint32_t fw = platform_info[0]* (1 << 16) + platform_info[1] *(1<<8) + platform_info[2];
	uint64_t targettype;
	lv2_get_target_type(&targettype);

	//check if current version is supported
	if (targettype==1) ttype="CEX";
	else if (targettype==2) ttype="DEX";
	else if (targettype==3) ttype="DECR";
	else ttype="Unknown";
	if (fw==0x40250) fw_version="4.25";
	else if (fw==0x40210) fw_version="4.21";
	else if (fw==0x40200) fw_version="4.20";
	else if (fw==0x40110) fw_version="4.11";
	else if (fw==0x30560) fw_version="3.56";
	else if (fw==0x30550) fw_version="3.55";
	else if (fw==0x30410) fw_version="3.41";
	else if (fw==0x30150) fw_version="3.15";
	else fw_version="0.00";
}

void draw_menu(int menu_id, int selected, int choosed, int menu1_position, int menu1_restore)
{
	int j=0;
	int posy=0;
	int sizeTitleFont = 40;
	int sizeFont = 30;
	int menu_color=COLOR_WHITE;
	string menu1_text;

	B1.Mono(COLOR_BLACK);
	BMap.DrawBitmap(&Precalculated_Layer);
	posy=260;
	if (menu_id==1)
	{
		F1.Printf(center_text_x(sizeTitleFont, "INSTALLER MENU"),220, 0xd38900, sizeTitleFont, "INSTALLER MENU");
		for(j=0;j<menu1_size;j++)
		{
			if (j<menu1_size-1) posy=posy+sizeFont+4;
			else posy=posy+(2*(sizeFont+4));
			if (j==selected)
			{
				if (choosed==1) menu_color=COLOR_RED;
				else menu_color=COLOR_YELLOW;
			}
			else menu_color=COLOR_WHITE;
			if (j==menu1_size-2 && menu1_restore==0) menu_color=COLOR_GREY;
			if (j<menu1_size-2) menu1_text="INSTALL "+menu1[j];
			else menu1_text=menu1[j];
			F2.Printf(center_text_x(sizeFont, menu1_text.c_str()),posy,menu_color,sizeFont, "%s",menu1_text.c_str());
		}
	}
	else if (menu_id==2)
	{
		F1.Printf(center_text_x(sizeTitleFont, "CHOOSE A FIRMWARE"),220,	0xd38900, sizeTitleFont, "CHOOSE A FIRMWARE");
		for(j=0;j<menu2_size[menu1_position];j++)
		{
			if (j<menu2_size[menu1_position]-1) posy=posy+sizeFont+4;
			else posy=posy+(2*(sizeFont+4));
			if (j==selected)
			{
				if (choosed==1) menu_color=COLOR_RED;
				else menu_color=COLOR_YELLOW;
			}
			else menu_color=COLOR_WHITE;
			F2.Printf(center_text_x(sizeFont, menu2[menu1_position][j].c_str()),posy,menu_color,sizeFont, "%s",menu2[menu1_position][j].c_str());
		}
	}
	else if (menu_id==3)
	{
		F1.Printf(center_text_x(sizeTitleFont, "CHOOSE A BACKUP TO RESTORE"),220,	0xd38900, sizeTitleFont, "CHOOSE A BACKUP TO RESTORE");
		for(j=0;j<menu3_size;j++)
		{
			if (j<menu3_size-2) posy=posy+sizeFont+4;
			else posy=posy+(2*(sizeFont+4));
			if (j==selected)
			{
				if (choosed==1) menu_color=COLOR_RED;
				else menu_color=COLOR_YELLOW;
			}
			else menu_color=COLOR_WHITE;
			F2.Printf(center_text_x(sizeFont, menu3[j].c_str()),posy,menu_color,sizeFont, "%s",menu3[j].c_str());
		}
	}
	Graphics->Flip();
	if (menu_color==COLOR_RED) sleep(0.3);
}

int make_menu_to_array(int whatmenu)
{
	int ifw=0;
	int iapp=0;
	DIR *dp;
	DIR *dp2;
	struct dirent *dirp;
	struct dirent *dirp2;
	string direct;
	string direct2;

	if (whatmenu==1 || whatmenu==2 || whatmenu==0)
	{
		iapp=0;
		direct=mainfolder+"/apps";
		dp = opendir (direct.c_str());
		if (dp == NULL) return 0;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0 && dirp->d_type == DT_DIR)
			{
				//second menu
				ifw=0;
				direct2=direct+"/"+dirp->d_name;
				dp2 = opendir (direct2.c_str());
				if (dp2 == NULL) return 0;
				while ( (dirp2 = readdir(dp2) ) )
				{
					if ( strcmp(dirp2->d_name, ".") != 0 && strcmp(dirp2->d_name, "..") != 0 && strcmp(dirp2->d_name, "") != 0 && dirp2->d_type == DT_DIR)
					{
						string fwfolder=(string)dirp2->d_name;
						string app_fwv=fwfolder.substr(0,fwfolder.find("-"));
						string app_fwt=fwfolder.substr(app_fwv.size()+1,fwfolder.rfind("-")-app_fwv.size()-1);
						string app_fwc=fwfolder.substr(app_fwv.size()+1+app_fwt.size()+1);
						//Mess.Dialog(MSG_OK,(app_fwv+"|"+app_fwt+"|"+app_fwc).c_str());
						if ((strcmp(app_fwv.c_str(), fw_version.c_str())==0 || strcmp(app_fwv.c_str(), "All")==0) && (strcmp(app_fwt.c_str(), ttype.c_str())==0 || strcmp(app_fwt.c_str(), "All")==0))
						{
							menu2[iapp][ifw]=app_fwc;
							menu2_path[iapp][ifw]=dirp2->d_name;
							ifw++;
						}
					}
				}
				closedir(dp2);
				if (ifw!=0) //has apps for the current firmware version
				{
					menu2[iapp][ifw]="Back to main menu";
					ifw++;
					menu2_size[iapp]=ifw;
					menu1[iapp]=dirp->d_name;
					iapp++;
				}
			}
		}
		closedir(dp);
		//print(("iapp:"+int_to_string(iapp)+"\n").c_str());
		if (iapp!=0)
		{
			menu1[iapp]="RESTORE a backup";
			iapp++;
			menu1[iapp]="Exit to XMB";
			iapp++;
		}
		menu1_size=iapp;
	}
	if (whatmenu==3 || whatmenu==0)
	{
		menu3_size=0;
		direct=mainfolder+"/backups";
		dp = opendir (direct.c_str());
		if (dp == NULL) return 0;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0 && dirp->d_type == DT_DIR)
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
	}
	return 1;
}

int menu_restore_available()
{
	if (opendir ((mainfolder+"/backups").c_str()) == NULL) return 0;
	else return 1;
}

s32 main(s32 argc, char* argv[])
{
	//this is the structure for the pad controllers
	padInfo padinfo;
	padData paddata;
	string firmware_choice;
	string app_choice;
	int mcount=0;
	char * pch;
	string ps3loadtid="PS3LOAD00";
	//int i;
	string ret="";
	int menu_restore;
	int menu1_position=0;
	int menu2_position=0;
	int menu3_position=0;
	int reboot=0;
	int current_menu=1, old_current_menu=1;
	int mpos=menu1_position, old_mpos=menu1_position;

	init_print("/dev_usb000/xmbmanpls_log.txt"); //this will initiate the NoRSX log
	ioPadInit (7); //this will initialize the controller (7= seven controllers)
	print("Getting main folder\n");
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
	//Show terms and conditions
	print("Show terms\n");
	if (show_terms()!=1) goto end;
	print("Detecting firmware changes\n");
	if (exists("/dev_flash/vsh/resource/explore/xmb/xmbmp.cfg")!=1 && exists((mainfolder+"/backups").c_str())==1)
	{
		Mess.Dialog(MSG_OK,"The system detected a firmware change. All previous backups will be deleted.");
		ret=recursiveDelete(mainfolder+"/backups");
		if (ret == "") Mess.Dialog(MSG_OK,"All backups deleted!\nPress OK to continue.");
		else Mess.Dialog(MSG_ERROR,("Problem with delete!\n\nError: "+ret).c_str());
	}
	print("Getting firmware info\n");
	get_firmware_info();
	print("Save menu options to array\n");
	if (make_menu_to_array(0)==0) { Mess.Dialog(MSG_ERROR,"Problem reading folder!"); goto end; }
	if (menu1_size==0) { Mess.Dialog(MSG_ERROR,"Your firmware version is not supported."); goto end; }
	menu_restore=menu_restore_available();
	make_background(fw_version, ttype);
	print("Start menu\n");
	Graphics->AppStart();
	draw_menu(current_menu,mpos,0,menu1_position,menu_restore);
	Graphics->Flip();
	while (Graphics->GetAppStatus())
	{
		if (current_menu==1) mpos=menu1_position;
		else if (current_menu==2) mpos=menu2_position;
		else if (current_menu==3) mpos=menu3_position;
		if (old_current_menu!=current_menu || old_mpos!=mpos)
		{
			//B1.Mono(COLOR_BLACK);
			//F2.Printf(50,100,COLOR_RED,30, "menu: %d pos: %d",current_menu, mpos);
			draw_menu(current_menu,mpos,0,menu1_position,menu_restore);
			old_current_menu=current_menu;
			old_mpos=mpos;
		}
		Graphics->Flip();
		ioPadGetInfo (&padinfo);
		//for(i = 0; i < MAX_PADS; i++)
		//{
		if (padinfo.status[0])
		{
			ioPadGetData (0, &paddata);
			if (current_menu==1)
			{
				//if (menu_restore==1 && menu1_position==menu1_size-2) menu1_position--;
				if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
				{
					if (menu1_position<menu1_size-1)
					{
						menu1_position++;
						//if (menu1_position==menu1_size-2 && menu_restore==0) menu1_position++;
					}
					else menu1_position=0;
				}
				if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
				{
					if (menu1_position>0)
					{
						menu1_position--;
						//if (menu1_position==menu1_size-2 && menu_restore==0) menu1_position--;
					}
					else menu1_position=menu1_size-1;
				}
				if (paddata.BTN_CROSS) //Install an app
				{
					draw_menu(current_menu,menu1_position,1,menu1_position,menu_restore);
					if (menu1_position<menu1_size-2)
					{
						app_choice=menu1[menu1_position];
						if (menu2[menu1_position][0]=="All" && menu2_size[menu1_position]==2)
						{
							if (install(menu2_path[menu1_position][0], app_choice)==2)
							{
								reboot=1;
								Graphics->AppExit();
							}
							else
							{ 
								if (make_menu_to_array(3)==0)
								{ Mess.Dialog(MSG_ERROR,"Problem reading folder!"); Graphics->AppExit(); }
								menu_restore=menu_restore_available();
							}
						}
						else current_menu=2;
					}
					else if (menu1_position<menu1_size-1) current_menu=3;
					else if (menu1_position<menu1_size) Graphics->AppExit();
				}
				if (paddata.BTN_SQUARE) //Delete an app
				{
					if (menu1_position<menu1_size-2)
					{
						draw_menu(current_menu,menu1_position,1,menu1_position,menu_restore);
						if (delete_one(menu1[menu1_position], "app")==1)
						{
							if (make_menu_to_array(1)==0)
							{ Mess.Dialog(MSG_ERROR,"Problem reading folder!"); Graphics->AppExit(); }
						}
					}
				}
			}
			else if (current_menu==2)
			{
				if (paddata.BTN_CIRCLE) current_menu=1;
				if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
				{
					if (menu2_position<menu2_size[menu1_position]-1) menu2_position++;
					else menu2_position=0;
				}
				if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
				{
					if (menu2_position>0) menu2_position--;
					else menu2_position=menu2_size[menu1_position]-1;
				}
				if (paddata.BTN_CROSS) //Install an app
				{
					draw_menu(current_menu,menu2_position,1,menu1_position,menu_restore);
					if (menu2_position<menu2_size[menu1_position]-1)
					{
						if (install(menu2_path[menu1_position][menu2_position], app_choice)==2)
						{
							reboot=1;
							Graphics->AppExit();
						}
						else
						{
							if (make_menu_to_array(3)==0)
							{ Mess.Dialog(MSG_ERROR,"Problem reading folder!"); Graphics->AppExit(); }
							menu_restore=menu_restore_available();
						}
					}
					else current_menu=1;
				}
			}
			else if (current_menu==3)
			{
				if (paddata.BTN_CIRCLE) current_menu=1;
				if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
				{
					if (menu3_position<menu3_size-1) menu3_position++;
					else menu3_position=0;
				}
				if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
				{
					if (menu3_position>0) menu3_position--;
					else menu3_position=menu3_size-1;
				}
				if (paddata.BTN_CROSS)
				{
					draw_menu(current_menu,menu3_position,1,menu1_position,menu_restore);
					if (menu3_position<menu3_size-2) //Restore a backup
					{
						if (restore(menu3[menu3_position])==2)
						{
							reboot=1;
							Graphics->AppExit();
						}
					}
					else if (menu3_position<menu3_size-1) //Delete all backups
					{
						if (delete_all()==1)
						{
							if (make_menu_to_array(3)==0)
							{ Mess.Dialog(MSG_ERROR,"Problem reading folder!"); Graphics->AppExit(); }
							menu_restore=menu_restore_available();
							current_menu=1;
						}
					}
					else current_menu=1;
				}
				if (paddata.BTN_SQUARE)
				{
					if (menu3_position<menu3_size-2) //Delete a backup
					{
						draw_menu(current_menu,menu3_position,1,menu1_position,menu_restore);
						if (delete_one(menu3[menu3_position], "backup")==1)
						{
							if (make_menu_to_array(3)==0)
							{ Mess.Dialog(MSG_ERROR,"Problem reading folder!"); Graphics->AppExit(); }
							menu_restore=menu_restore_available();
						}
					}
				}
			}
		}
		//}
	}
	goto end;

	end:
	{
		BMap.ClearBitmap(&Precalculated_Layer);
		print("End\n");
		if (is_dev_blind_mounted()==0) unmount_dev_blind();
		end_print(); //close log
		Graphics->NoRSX_Exit(); //This will uninit the NoRSX lib
		ioPadEnd(); //this will uninitialize the controllers
		if (reboot==1) reboot_sys(); //reboot
	}

	return 0;
}