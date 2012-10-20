#include "xmbmp-syscalls.h"
#include "xmbmp-file.h"
#include "xmbmp-graphics.h"
//#include "xmbmp-debug.h"
#include <io/pad.h>
#include <time.h>
#include <zlib.h>

#define MAX_OPTIONS 10

//global vars
string mainfolder;
string menu1[MAX_OPTIONS];
string menu2[MAX_OPTIONS][MAX_OPTIONS];
string menu2_path[MAX_OPTIONS][MAX_OPTIONS];
string menu3[MAX_OPTIONS];

//headers
const string currentDateTime();
string doit(string operation, string foldername, string fw_folder, string app);
int restore(string foldername);
int install(string firmware_folder, string app_choice);
int delete_all();
int delete_one(string foldername, string type);
int make_menu_to_array(int whatmenu, string vers, string type);
void draw_menu(int menu_id, int msize, int selected, int choosed, int menu1_pos, int menu1_restore);
s32 main(s32 argc, char* argv[]);

const string currentDateTime()
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %Hh%Mm%Ss", &tstruct);

    return buf;
}

int string_array_size(string *arr)
{
	int size=0;
	while (strcmp(arr[size].c_str(),"") != 0)
	{
		size++;
	}
	return size;
}

string doit(string operation, string foldername, string fw_folder, string app)
{
	DIR *dp;
	struct dirent *dirp;
	int findex=0, mountblind=0, numfiles_total=0, numfiles_current=0, j=0, i=0;
	double copy_totalsize=0, copy_currentsize=0, source_size=0, dest_size=0, freespace_size=0;
	string source_paths[100], dest_paths[100], check_paths[100];
	string check_path, sourcefile, destfile, filename, dest, source, title;
	string *files_list = NULL, *final_list_source = NULL, *final_list_dest = NULL;  //Pointer for an array to hold the filenames.
	string ret="";

	if (operation=="backup")
	{
		check_path=mainfolder+"/apps/"+app+"/"+fw_folder;
		dp = opendir (check_path.c_str());
		if (dp == NULL) return "Cannot open directory "+check_path;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				if (dirp->d_type == DT_DIR)
				{
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
		if (exists("/dev_flash/vsh/resource/explore/xmb/xmbmp.cfg")!=0) create_file("/dev_blind/vsh/resource/explore/xmb/xmbmp.cfg");
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
			if (!(operation=="backup" && exists(sourcefile.c_str())!=0))
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
		if (source_size >= freespace_size) return "Not enough space to copy the file ("+filename+") to destination path ("+dest+").";
		else
		{
			if (mkdir_full(dest)!=0) return "Could not create directory ("+dest+").";
			ret=copy_file(title, source.c_str(), dest.c_str(), filename.c_str(), copy_currentsize, copy_totalsize, numfiles_current, numfiles_total,0);
			if (ret != "") return ret;
		}
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

int make_menu_to_array(int whatmenu, string vers, string type)
{
	int ifw=0, iapp=0, ibackup=0;
	DIR *dp, *dp2;
	struct dirent *dirp, *dirp2;
	string direct, direct2;

	if (whatmenu==1 || whatmenu==2 || whatmenu==0)
	{
		iapp=0;
		direct=mainfolder+"/apps";
		dp = opendir (direct.c_str());
		if (dp == NULL) return -1;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0 && dirp->d_type == DT_DIR)
			{
				//second menu
				ifw=0;
				direct2=direct+"/"+dirp->d_name;
				dp2 = opendir (direct2.c_str());
				if (dp2 == NULL) return -1;
				while ( (dirp2 = readdir(dp2) ) )
				{
					if ( strcmp(dirp2->d_name, ".") != 0 && strcmp(dirp2->d_name, "..") != 0 && strcmp(dirp2->d_name, "") != 0 && dirp2->d_type == DT_DIR)
					{
						string fwfolder=(string)dirp2->d_name;
						string app_fwv=fwfolder.substr(0,fwfolder.find("-"));
						string app_fwt=fwfolder.substr(app_fwv.size()+1,fwfolder.rfind("-")-app_fwv.size()-1);
						string app_fwc=fwfolder.substr(app_fwv.size()+1+app_fwt.size()+1);
						//Mess.Dialog(MSG_OK,(app_fwv+"|"+app_fwt+"|"+app_fwc).c_str());
						if ((strcmp(app_fwv.c_str(), vers.c_str())==0 || strcmp(app_fwv.c_str(), "All")==0) && (strcmp(app_fwt.c_str(), type.c_str())==0 || strcmp(app_fwt.c_str(), "All")==0))
						{
							menu2[iapp][ifw]=app_fwc;
							menu2_path[iapp][ifw]=dirp2->d_name;
							ifw++;
						}
					}
				}
				closedir(dp2);
				if (ifw>0) //has apps for the current firmware version
				{
					menu2[iapp][ifw]="Back to main menu";
					ifw++;
					menu1[iapp]=dirp->d_name;
					iapp++;
				}
			}
		}
		closedir(dp);
		//print(("iapp:"+int_to_string(iapp)+"\n").c_str());
		if (iapp>0)
		{
			menu1[iapp]="RESTORE a backup";
			iapp++;
			menu1[iapp]="Exit to XMB";
			iapp++;
		}
	}
	if (whatmenu==3 || whatmenu==0)
	{
		ibackup=0;
		direct=mainfolder+"/backups";
		if (exists_backups(mainfolder)==0)
		{
			dp = opendir(direct.c_str());
			if (dp == NULL) return -1;
			while ( (dirp = readdir(dp) ) )
			{
				if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0 && dirp->d_type == DT_DIR)
				{
					menu3[ibackup]=dirp->d_name;
					ibackup++;
				}
			}
			closedir(dp);
			if (ibackup>0)
			{
				menu3[ibackup]="DELETE all backups";
				ibackup++;
				menu3[ibackup]="Back to main menu";
				ibackup++;
			}
			else recursiveDelete(direct);
		}
	}
	return 0;
}

void draw_menu(int menu_id, int msize, int selected, int choosed, int menu1_pos, int menu1_restore)
{
	int j, posy=260, sizeTitleFont = 40, sizeFont = 30;
	int menu_color=COLOR_WHITE;
	string menu1_text;

	BMap.DrawBitmap(&Precalculated_Layer);
	if (menu_id==1)
	{
		F1.Printf(center_text_x(sizeTitleFont, "MAIN MENU"),220, 0xd38900, sizeTitleFont, "MAIN MENU");
		for(j=0;j<msize;j++)
		{
			if (j<msize-2) posy=posy+sizeFont+4;
			else posy=posy+(2*(sizeFont+4));
			if (j==selected)
			{
				if (choosed==1) menu_color=COLOR_RED;
				else menu_color=COLOR_YELLOW;
			}
			else menu_color=COLOR_WHITE;
			if (j==msize-2 && menu1_restore!=0) menu_color=COLOR_GREY;
			if (j<msize-2) menu1_text="INSTALL "+menu1[j];
			else menu1_text=menu1[j];
			F2.Printf(center_text_x(sizeFont, menu1_text.c_str()),posy,menu_color,sizeFont, "%s",menu1_text.c_str());
		}
	}
	else if (menu_id==2)
	{
		F1.Printf(center_text_x(sizeTitleFont, "CHOOSE A FIRMWARE"),220,	0xd38900, sizeTitleFont, "CHOOSE A FIRMWARE");
		for(j=0;j<msize;j++)
		{
			if (j<msize-1) posy=posy+sizeFont+4;
			else posy=posy+(2*(sizeFont+4));
			if (j==selected)
			{
				if (choosed==1) menu_color=COLOR_RED;
				else menu_color=COLOR_YELLOW;
			}
			else menu_color=COLOR_WHITE;
			F2.Printf(center_text_x(sizeFont, menu2[menu1_pos][j].c_str()),posy,menu_color,sizeFont, "%s",menu2[menu1_pos][j].c_str());
		}
	}
	else if (menu_id==3)
	{
		F1.Printf(center_text_x(sizeTitleFont, "CHOOSE A BACKUP TO RESTORE"),220,	0xd38900, sizeTitleFont, "CHOOSE A BACKUP TO RESTORE");
		for(j=0;j<msize;j++)
		{
			if (j<msize-2) posy=posy+sizeFont+4;
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
	//For testing porposes
	//F1.Printf(50,150,COLOR_RED,30,"Current menu: %d", menu_id);
	//F1.Printf(50,200,COLOR_RED,30,"Current menu position: %d/%d", selected, msize-1);
	//F1.Printf(50,250,COLOR_RED,30,"Menu 1 position: %d", menu1_pos);
	//F1.Printf(50,300,COLOR_RED,30,"Restore option: %d", menu1_restore);
	Graphics->Flip();
	if (menu_color==COLOR_RED) sleep(0.7);
	//else if (menu_color==COLOR_YELLOW) sleep(0.2);
}

s32 main(s32 argc, char* argv[])
{
	padInfo2 padinfo2;
	padData paddata;
	int menu_restore=-1, menu1_position=0, menu2_position=0, menu3_position=0, mpos=0, reboot=0, temp=0, current_menu=1;
	string fw_version, ttype;
	int msize=0;

	init_print("/dev_usb000/xmbmanpls_log.txt"); //this will initiate the NoRSX log
	ioPadInit(MAX_PORT_NUM); //this will initialize the controller (7= seven controllers)
	print("Getting main folder\r\n");
	mainfolder=get_app_folder(argv[0]);
	menu_restore=exists_backups(mainfolder);
	print("Show terms\r\n");
	if (show_terms(mainfolder)!=0) goto end;
	print("Detecting firmware changes\r\n");
	check_firmware_changes(mainfolder);
	print("Getting firmware info\r\n");
	fw_version=get_firmware_info("version");
	ttype=get_firmware_info("type");
	print("Construct menu\r\n");
	if (make_menu_to_array(0,fw_version, ttype)!=0) { Mess.Dialog(MSG_ERROR,"Problem reading folder!"); goto end; }
	print("Test if firmware is supported\r\n");
	if (string_array_size(menu1)==0) { Mess.Dialog(MSG_ERROR,"Your firmware version is not supported."); goto end; }
	make_background(fw_version, ttype, mainfolder);
	print("Start menu\r\n");
	Graphics->AppStart();
	while (Graphics->GetAppStatus())
	{
		if (current_menu==1)
		{
			msize=string_array_size(menu1);
			mpos=menu1_position;
		}
		else if (current_menu==2)
		{
			msize=string_array_size(menu2[menu1_position]);
			mpos=menu2_position;
		}
		else if (current_menu==3)
		{
			msize=string_array_size(menu3);
			mpos=menu3_position;
		}
		draw_menu(current_menu,msize,mpos,0,menu1_position,menu_restore);
		if (ioPadGetInfo2(&padinfo2)==0)
		{
			for(int i=0;i<MAX_PORT_NUM;i++)
			{
				if (padinfo2.port_status[i])
				{
					ioPadGetData(i, &paddata);
					if (current_menu==1)
					{
						if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
						{
							if (menu1_position<msize-1)
							{
								menu1_position++;
								if (menu1_position==msize-2 && menu_restore!=0) { menu1_position++; }
							}
							else menu1_position=0;
						}
						else if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
						{
							if (menu1_position>0)
							{
								menu1_position--;
								if (menu1_position==msize-2 && menu_restore!=0) { menu1_position--; }
							}
							else menu1_position=msize-1;
						}
						else if (paddata.BTN_CROSS) //Install an app
						{
							draw_menu(current_menu,msize,mpos,1,menu1_position,menu_restore);
							if (menu1_position<msize-2)
							{
								if (menu2[menu1_position][0]=="All" && msize==2)
								{
									temp=install(menu2_path[menu1_position][0], menu1[menu1_position]);
									if (temp==2)
									{
										reboot=1;
										Graphics->AppExit();
									}
									else if (temp==1)
									{ 
										if (make_menu_to_array(3,fw_version, ttype)!=0)
										{
											Mess.Dialog(MSG_ERROR,"Problem reading folder!");
											Graphics->AppExit();
										}
										menu_restore=exists_backups(mainfolder);
									}
								}
								else current_menu=2;
							}
							else if (menu1_position<msize-1) current_menu=3;
							else if (menu1_position<msize) Graphics->AppExit();
						}
						else if (paddata.BTN_SQUARE) //Delete an app
						{
							if (menu1_position<msize-2)
							{
								draw_menu(current_menu,msize,mpos,1,menu1_position,menu_restore);
								if (delete_one(menu1[menu1_position], "app")==1)
								{
									if (make_menu_to_array(1,fw_version, ttype)!=0)
									{
										Mess.Dialog(MSG_ERROR,"Problem reading folder!");
										Graphics->AppExit();
									}
								}
							}
						}
					}
					else if (current_menu==2)
					{
						if (paddata.BTN_CIRCLE) { current_menu=1; }
						else if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
						{
							if (menu2_position<msize-1) { menu2_position++; }
							else menu2_position=0;
						}
						else if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
						{
							if (menu2_position>0) { menu2_position--; }
							else menu2_position=msize-1;
						}
						else if (paddata.BTN_CROSS) //Install an app
						{
							draw_menu(current_menu,msize,mpos,1,menu1_position,menu_restore);
							if (menu2_position<msize-1)
							{
								temp=install(menu2_path[menu1_position][menu2_position], menu1[menu1_position]);
								if (temp==2)
								{
									reboot=1;
									Graphics->AppExit();
								}
								else if (temp==1)
								{
									if (make_menu_to_array(3,fw_version, ttype)!=0)
									{
										Mess.Dialog(MSG_ERROR,"Problem reading folder!");
										Graphics->AppExit();
									}
									menu_restore=exists_backups(mainfolder);
									current_menu=1;
								}
							}
							else current_menu=1;
						}
					}
					else if (current_menu==3)
					{
						if (paddata.BTN_CIRCLE) { current_menu=1; }
						else if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
						{
							if (menu3_position<msize-1) { menu3_position++; }
							else menu3_position=0;
						}
						else if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
						{
							if (menu3_position>0) { menu3_position--; }
							else menu3_position=msize-1;
						}
						else if (paddata.BTN_CROSS)
						{
							draw_menu(current_menu,msize,mpos,1,menu1_position,menu_restore);
							if (menu3_position<msize-2) //Restore a backup
							{
								if (restore(menu3[menu3_position])==2)
								{
									reboot=1;
									Graphics->AppExit();
								}
							}
							else if (menu3_position<msize-1) //Delete all backups
							{
								if (delete_all()==1)
								{
									make_menu_to_array(3,fw_version, ttype);
									menu_restore=-1;
									current_menu=1;
									menu1_position++;
								}
							}
							else current_menu=1;
						}
						else if (paddata.BTN_SQUARE)
						{
							if (menu3_position<msize-2) //Delete a backup
							{
								draw_menu(current_menu,msize,mpos,1,menu1_position,menu_restore);
								if (delete_one(menu3[menu3_position], "backup")==1)
								{
									if (make_menu_to_array(3,fw_version, ttype)!=0)
									{
										Mess.Dialog(MSG_ERROR,"Problem reading folder!");
										Graphics->AppExit();
									}
									menu_restore=exists_backups(mainfolder);
									if (menu_restore!=0)
									{
										current_menu=1;
										menu1_position++;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	goto end;

	end:
	{
		if (current_menu==1 && mpos==msize-1) draw_menu(current_menu,msize,mpos,0,menu1_position,menu_restore);
		BMap.ClearBitmap(&Precalculated_Layer);
		print("End\r\n");
		if (is_dev_blind_mounted()==0) unmount_dev_blind();
		end_print(); //close log
		Graphics->NoRSX_Exit(); //This will uninit the NoRSX lib
		ioPadEnd(); //this will uninitialize the controllers
		if (reboot==1) reboot_sys(); //reboot
	}

	return 0;
}