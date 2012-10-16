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
#include <zlib.h>

#define MAX_BUFFERS 2
#define CHUNK 16384
using namespace std;

//global vars
string mainfolder;
string fw_version="";
string ttype="";
string menu1[99];
int menu1_size=0;
int menu1_restore=1;
string menu2[99][99];
string menu2_path[99][99];
int menu2_size[99];
string menu3[99];
int menu3_size=0;
NoRSX *Graphics = new NoRSX(RESOLUTION_1920x1080);
Background B1(Graphics);
Font F1(LATIN2, Graphics);
Font F2(LATIN2, Graphics);
MsgDialog Mess(Graphics);
msgType MSG_OK = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_ERROR = (msgType)(MSG_DIALOG_ERROR | MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_DISABLE_CANCEL_ON);
msgType MSG_YESNO_DNO = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO | MSG_DIALOG_DEFAULT_CURSOR_NO);
msgType MSG_YESNO_DYES = (msgType)(MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO);

//headers
s32 lv2_get_platform_info(uint8_t platform_info[0x18]);
s32 sysFsMount(const char* MOUNT_POINT, const char* TYPE_OF_FILESYSTEM, const char* PATH_TO_MOUNT, int IF_READ_ONLY);
s32 sysFsUnmount(const char* PATH_TO_UNMOUNT);
u32 reboot_sys();
s32 lv2_get_target_type(uint64_t *type);
int is_dev_blind_mounted();
int mount_dev_blind();
int unmount_dev_blind();
void debug_print(string text);
string convert_size(double size, string format);
double get_free_space(const char *path);
double get_filesize(const char *path);
string create_file(const char* cpath);
int exists(const char *path);
const string fileCreatedDateTime(const char *path);
const string currentDateTime();
s32 create_dir(string dirtocreate);
string recursiveDelete(string direct);
s32 center_text_x(int fsize, const char* message);
string int_to_string(int number);
void draw_copy(string title, const char *dirfrom, const char *dirto, const char *filename, string cfrom, double copy_currentsize, double copy_totalsize, int numfiles_current, int numfiles_total, size_t countsize);
string copy_file(string title, const char *dirfrom, const char *dirto, const char *filename, double copy_currentsize, double copy_totalsize, int numfiles_current, int numfiles_total, int check_flag);
bool replace(string& str, const string& from, const string& to);
string correct_path(string dpath, int what);
string *recursiveListing(string direct);
string doit(string operation, string foldername, string fw, string app);
void draw_menu(int menu_id, int selected,int choosed, int menu1_position);
int restore(string foldername);
int install(string firmware_choice, string app_choice);
int delete_all();
int delete_one(string foldername, string type);
int show_terms();
int main(s32 argc, char* argv[]);

//functions
s32 lv2_get_platform_info(uint8_t platform_info[0x18])
{
	lv2syscall1(387, (uint64_t) platform_info);
	return_to_user_prog(s32);
}

s32 sysFsMount(const char* MOUNT_POINT, const char* TYPE_OF_FILESYSTEM, const char* PATH_TO_MOUNT, int IF_READ_ONLY)
{
	lv2syscall8(837, (u64)MOUNT_POINT, (u64)TYPE_OF_FILESYSTEM, (u64)PATH_TO_MOUNT, 0, IF_READ_ONLY, 0, 0, 0);
	return_to_user_prog(s32);
}

s32 sysFsUnmount(const char* PATH_TO_UNMOUNT)
{
	lv2syscall1(838, (u64)PATH_TO_UNMOUNT);
	return_to_user_prog(s32);
}

u32 reboot_sys()
{ 
	lv2syscall4(379,0x200,0,0,0);
	return_to_user_prog(u32);
}

s32 lv2_get_target_type(uint64_t *type)
{
	lv2syscall1(985, (uint64_t) type);
	return_to_user_prog(s32);
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

	sysFsUnmount(MOUNT_POINT);

	return 0;
}

void debug_print(string text)
{
	B1.Mono(COLOR_BLACK);
	F1.Printf(100, 100,0xffffff,20, "%s", text.c_str());
	Graphics->Flip();
	sleep(10);
}

string convert_size(double size, string format)
{
	char str[100];

	if (format=="auto")
	{
		if (size >= 1073741824) format="GB";
		else if (size >= 1048576) format="MB";
		else format="KB";
	}
	if (format=="KB") size = size / 1024.00; // convert to KB
	else if (format=="MB") size = size / 1048576.00; // convert to MB
	else if (format=="GB") size = size / 1073741824.00; // convert to GB
	if (format=="KB") sprintf(str, "%.2fKB", size);
	else if (format=="MB") sprintf(str, "%.2fMB", size);
	else if (format=="GB") sprintf(str, "%.2fGB", size);

	return str;
}

double get_free_space(const char *path)
{
	uint32_t block_size;
	uint64_t free_block_count;

	sysFsGetFreeSize(path, &block_size, &free_block_count);
	return (((uint64_t) block_size * free_block_count));
}

double get_filesize(const char *path)
{
	sysFSStat info;

	if (sysFsStat(path, &info) >= 0) return (double)info.st_size;
	else return 0;
}

string create_file(const char* cpath)
{
  FILE *path;

  /* open destination file */
  if((path = fopen(cpath, "wb"))==NULL) return "Cannot open file ("+(string)cpath+") for writing!";
  if(fclose(path)==EOF) return "Cannot close file ("+(string)cpath+")!";

  return "";
}

int exists(const char *path)
{
	sysFSStat info;

	if (sysFsStat(path, &info) >= 0) return 1;
	return 0;
}

const string fileCreatedDateTime(const char *path)
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

s32 create_dir(string dirtocreate)
{
	return mkdir(dirtocreate.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

string recursiveDelete(string direct)
{
	string dfile;
	DIR *dp;
	struct dirent *dirp;

	dp = opendir (direct.c_str());
	if (dp != NULL)
	{
		while ((dirp = readdir (dp)))
		{
			dfile = direct + "/" + dirp->d_name;
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				//Mess.Dialog(MSG_OK,("Testing: "+dfile).c_str());
				if (dirp->d_type == DT_DIR)
				{
					//Mess.Dialog(MSG_OK,("Is directory: "+dfile).c_str());
					recursiveDelete(dfile);
				}
				else
				{
					//Mess.Dialog(MSG_OK,("Deleting file: "+dfile).c_str());
					if ( sysFsUnlink(dfile.c_str()) != 0) return "Couldn't delete file "+dfile+"\n"+strerror(errno);
				}
			}
		}
		(void) closedir (dp);
	}
	else return "Couldn't open the directory";
	//Mess.Dialog(MSG_OK,("Deleting folder: "+direct).c_str());
	if ( rmdir(direct.c_str()) != 0) return "Couldn't delete directory "+direct+"\n"+strerror(errno);
	return "";
}

s32 center_text_x(int fsize, const char* message)
{
	return (Graphics->width-(strlen(message)*fsize/2))/2;
}

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

/*
string z_error(int ret, FILE *source, FILE *dest)
{
	switch (ret)
	{
		case Z_ERRNO:
			if (ferror(source)) return "Zlib error: Error reading source file";
			if (ferror(dest)) return "Zlib error: Error writing to destination directory";
			break;
		case Z_STREAM_ERROR: return "Zlib error: Invalid compression level"; break;
		case Z_DATA_ERROR: return "Zlib error: Invalid or incomplete deflate data"; break;
		case Z_MEM_ERROR: return "Zlib error: Out of memory"; break;
		case Z_VERSION_ERROR: return "Zlib error: zlib version mismatch!"; break;
    }
	return "";
}

string z_inflate(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
    
	// allocate inflate state
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
	if (ret != Z_OK) return z_error(ret, source, dest);
	// decompress until deflate stream ends or end of file
	do
    {
		strm.avail_in = fread(in, 1, CHUNK, source);
		if (ferror(source))
		{
			(void)inflateEnd(&strm);
			return z_error(Z_ERRNO, source, dest);
		}
		if (strm.avail_in == 0) break;
		strm.next_in = in;
		// run inflate() on input until output buffer not full
		do
		{
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  // state not clobbered
            switch (ret)
			{
				case Z_NEED_DICT: ret = Z_DATA_ERROR;     // and fall through
				case Z_DATA_ERROR:
				case Z_MEM_ERROR: (void)inflateEnd(&strm); return z_error(ret, source, dest);
			}
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest))
			{
				(void)inflateEnd(&strm);
				return z_error(Z_ERRNO, source, dest);
			}
		}
		while (strm.avail_out == 0);
		// done when inflate() says it's done
	}
	while (ret != Z_STREAM_END);
	// clean up and return 
	(void)inflateEnd(&strm);

	ret = Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
	if (ret != Z_OK) return z_error(ret, source, dest);

	return "";
}*/

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

string copy_file(string title, const char *dirfrom, const char *dirto, const char *filename, double copy_currentsize, double copy_totalsize, int numfiles_current, int numfiles_total, int check_flag)
{
	string cfrom=(string)dirfrom+(string)filename;
	string ctoo=(string)dirto+(string)filename;
	FILE *from, *to;
	char buf[8192], buf2[8192]; // BUFSIZE default is 8192 bytes
	string ret="";
	size_t size=0;
	size_t countsize=0;
	time_t start, now;

	if ((from = fopen(cfrom.c_str(), "rb"))==NULL) return "Cannot open source file ("+cfrom+") for reading!";
	if (check_flag==1)
	{
		if ((to = fopen(ctoo.c_str(), "rb"))==NULL) return "Cannot open destination file ("+ctoo+") for reading!";
	}
	else if ((to = fopen(ctoo.c_str(), "wb"))==NULL) return "Cannot open destination file ("+ctoo+") for writing!";
	draw_copy(title, dirfrom, dirto, filename, cfrom, copy_currentsize, copy_totalsize, numfiles_current, numfiles_total, countsize);
	start = time(NULL);
	time(&start);
	while(!feof(from))
	{
		size = fread(buf, 1, 8192, from);
		if(ferror(from))  return "Error reading source file ("+cfrom+")!";
		if (check_flag==1)
		{
			size = fread(buf2, 1, 8192, to);
			if (ferror(to)) return "Error reading destination file ("+ctoo+")!";
			if (memcmp(buf, buf2, 8192)!=0) return "Source and destination files are different!";
		}
		else
		{
			fwrite(buf, 1, size, to);
			if (ferror(to)) return "Error writing destination file ("+ctoo+")!";
		}
		countsize=countsize+size;
		now = time(NULL);
		time(&now);
		if (difftime(now,start)>=0.3)
		{
			draw_copy(title, dirfrom, dirto, filename, cfrom, copy_currentsize, copy_totalsize, numfiles_current, numfiles_total, countsize);
			start = time(NULL);
			time(&start);
		}
	}

	if (fclose(from)==EOF) return "Cannot close source file ("+cfrom+")!";
	if (fclose(to)==EOF) return "Cannot close destination file ("+ctoo+")!";

	return "";
}

bool replace(string& str, const string& from, const string& to)
{
    size_t start_pos = str.find(from);
    if(start_pos == string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

string correct_path(string dpath, int what)
{
	string cpath;

	cpath=dpath;
	if (what==1 || what==2) if (cpath.find("PS3~")!=string::npos) cpath.replace( cpath.find("PS3~"), 4, "");
	if (what==1 || what==2) replace(cpath.begin(), cpath.end(), '~', '/');
	if (what==2) if (cpath.find("dev_flash")!=string::npos) cpath.replace( cpath.find("dev_flash"), 9, "dev_blind");

	return "/"+cpath;
}

string *recursiveListing(string direct)
{
	string dfile;
	DIR *dp;
	struct dirent *dirp=NULL;
	string *listed_file_names = NULL;  //Pointer for an array to hold the filenames.
	string *sub_listed_file_names = NULL;  //Pointer for an array to hold the filenames.
	int aindex=0;

	listed_file_names = new string[5000];
	sub_listed_file_names = new string[5000];
	dp = opendir (direct.c_str());
	if (dp != NULL)
	{
		while ((dirp = readdir (dp)))
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				dfile = direct + "/" + dirp->d_name;
				if (dirp->d_type == DT_DIR)
				{
					sub_listed_file_names=recursiveListing(dfile);
					//Mess.Dialog(MSG_OK,("Dir: "+dfile+" "+int_to_string(sizeof(sub_listed_file_names)/sizeof(sub_listed_file_names[0]))).c_str());
					int i=0;
					while (strcmp(sub_listed_file_names[i].c_str(),"") != 0)
					{
						listed_file_names[aindex]=sub_listed_file_names[i];
						//Mess.Dialog(MSG_OK,("file: "+listed_file_names[aindex]).c_str());
						i++;
						aindex++;
					}
					//Mess.Dialog(MSG_OK,("Dir: "+dfile+" "+int_to_string(i)).c_str());
				}
				else
				{
					listed_file_names[aindex]=dfile;
					//Mess.Dialog(MSG_OK,("File: "+listed_file_names[aindex]).c_str());
					aindex++;
				}
			}
		}
		closedir(dp);
	}

	return listed_file_names;
}


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

void draw_menu(int menu_id, int selected,int choosed, int menu1_position)
{
	string IMAGE_PATH=mainfolder+"/data/images/logo.png";
	int posy=0;

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
	int sizeTitleFont = 40;
	int sizeFont = 30;
	int menu_color=COLOR_WHITE;
	string menu1_text;

	posy=260;
	if (menu_id==1)
	{
		F1.Printf(center_text_x(sizeTitleFont, "INSTALLER MENU"),220, 0xd38900, sizeTitleFont, "INSTALLER MENU");
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

			F2.Printf(center_text_x(sizeFont, menu1_text.c_str()),posy,menu_color,sizeFont, "%s",menu1_text.c_str());
		}
	}
	else if (menu_id==2)
	{
		F1.Printf(center_text_x(sizeTitleFont, "CHOOSE A FIRMWARE"),220,	0xd38900, sizeTitleFont, "CHOOSE A FIRMWARE");
		for(int j=0;j<menu2_size[menu1_position];j++)
		{
			if (j<menu2_size[menu1_position]-1) posy=posy+sizeFont+4;
			else posy=posy+(2*(sizeFont+4));
			if (j==choosed) menu_color=COLOR_RED;
			else if (j==selected) menu_color=COLOR_YELLOW;
			else menu_color=COLOR_WHITE;
			F2.Printf(center_text_x(sizeFont, menu2[menu1_position][j].c_str()),posy,menu_color,sizeFont, "%s",menu2[menu1_position][j].c_str());
		}
	}
	else if (menu_id==3)
	{
		F1.Printf(center_text_x(sizeTitleFont, "CHOOSE A BACKUP TO RESTORE"),220,	0xd38900, sizeTitleFont, "CHOOSE A BACKUP TO RESTORE");
		for(int j=0;j<menu3_size;j++)
		{
			if (j<menu3_size-2) posy=posy+sizeFont+4;
			else posy=posy+(2*(sizeFont+4));
			if (j==choosed) menu_color=COLOR_RED;
			else if (j==selected) menu_color=COLOR_YELLOW;
			else menu_color=COLOR_WHITE;
			F2.Printf(center_text_x(sizeFont, menu3[j].c_str()),posy,menu_color,sizeFont, "%s",menu3[j].c_str());
		}
	}
	F2.Printf(center_text_x(sizeFont, "Firmware: X.XX (CEX)"),Graphics->height-(sizeFont+20+(sizeFont-5)+10),0xc0c0c0,sizeFont, "Firmware: %s (%s)", fw_version.c_str(), ttype.c_str());
	F2.Printf(center_text_x(sizeFont-5, "Installer created by XMBM+ Team"),Graphics->height-((sizeFont-5)+10),0xd38900,sizeFont-5,     "Installer created by XMBM+ Team");
	
	Graphics->Flip();
	if (menu_color==COLOR_RED) sleep(0.02);
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

int main(s32 argc, char* argv[])
{
	//this is the structure for the pad controllers
	padInfo padinfo;
	padData paddata;
	string firmware_choice;
	string app_choice;
	string direct;
	string direct2;
	DIR *dp;
	DIR *dp2;
	struct dirent *dirp;
	struct dirent *dirp2;
	int mcount=0;
	char * pch;
	string ps3loadtid="PS3LOAD00";
	int i;
	int ifw=0;
	int iapp=0;
	string ret="";
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

	//Show terms and conditions
	if (show_terms()!=1) goto end;

	//DETECT FIRMWARE CHANGES WERE
	if (exists("/dev_flash/vsh/resource/explore/xmb/xmbmp.cfg")!=1 && exists((mainfolder+"/backups").c_str())==1)
	{
		Mess.Dialog(MSG_OK,"The system detected a firmware change. All previous backups will be deleted.");
		ret=recursiveDelete(mainfolder+"/backups");
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

	Graphics->AppStart();

	start:
	//make menus arrays
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
	draw_menu(1,menu1_position,-1,0);
	while (Graphics->GetAppStatus())
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
					}
					else menu1_position=0;
					goto menu_1;
				}
				if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
				{
					if (menu1_position>0)
					{
						menu1_position--;
						if (menu1_position==menu1_size-2 && menu1_restore==0) menu1_position--;
					}
					else menu1_position=menu1_size-1;
					goto menu_1;
				}
				if (paddata.BTN_CROSS) //Install an app
				{
					draw_menu(1,-1,menu1_position,0);
					if (menu1_position<menu1_size-2)
					{
						app_choice=menu1[menu1_position];
						if (menu2[menu1_position][0]=="All" && menu2_size[menu1_position]==2)
						{
							if (install(menu2_path[menu1_position][0], app_choice)==2) goto end_with_reboot;
							else goto menu_1;
						}
						else goto continue_to_menu2;
					}
					else if (menu1_position<menu1_size-1) goto continue_to_menu3;
					else if (menu1_position<menu1_size) goto end;
					else goto menu_1;
				}
				if (paddata.BTN_SQUARE) //Delete an app
				{
					if (menu1_position<menu1_size-2)
					{
						draw_menu(1,-1,menu1_position,0);
						if (delete_one(menu1[menu1_position], "app")==1) goto start;
						else goto menu_1;
					}
				}
			}
		}
	}

	continue_to_menu2:
	menu2_position=0;
	menu_2:
	draw_menu(2,menu2_position,-1,menu1_position);
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
					if (menu2_position<menu2_size[menu1_position]-1) menu2_position++;
					else menu2_position=0;
					goto menu_2;
				}
				if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
				{
					if (menu2_position>0) menu2_position--;
					else menu2_position=menu2_size[menu1_position]-1;
					goto menu_2;
				}
				if (paddata.BTN_CROSS)
				{
					draw_menu(2,-1,menu2_position,menu1_position);
					if (menu2_position<menu2_size[menu1_position]-1)
					{
						if (install(menu2_path[menu1_position][menu2_position], app_choice)==2) goto end_with_reboot;
						else goto menu_2;
					}
					else goto menu_1;
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
	menu3_position=0;
	menu_3:
	draw_menu(3,menu3_position,-1,0);
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
					if (menu3_position<menu3_size-1) menu3_position++;
					else menu3_position=0;
					goto menu_3;
				}
				if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
				{
					if (menu3_position>0) menu3_position--;
					else menu3_position=menu3_size-1;
					goto menu_3;
				}
				if (paddata.BTN_CROSS)
				{
					draw_menu(3,-1,menu3_position,0);
					if (menu3_position<menu3_size-2) //Restore a backup
					{
						if (restore(menu3[menu3_position])==2) goto end_with_reboot;
						else goto menu_3;
					}
					else if (menu3_position<menu3_size-1) //Delete all backups
					{
						if (delete_all()==1) goto menu_1;
						else goto menu_3;
					}
					else goto menu_1;
				}
				if (paddata.BTN_SQUARE)
				{
					if (menu3_position<menu3_size-2) //Delete a backup
					{
						draw_menu(3,-1,menu3_position,0);
						if (delete_one(menu3[menu3_position], "backup")==1) goto continue_to_menu3;
						else goto menu_3;
					}
				}
			}
		}
	}

	end_with_reboot:
	{
		Graphics->AppExit();
		if (is_dev_blind_mounted()==0) unmount_dev_blind();
		Graphics->NoRSX_Exit(); //This will uninit the NoRSX lib
		ioPadEnd(); //this will uninitialize the controllers
		reboot_sys(); //reboot
	}

	end:
	{
		Graphics->AppExit();
		if (is_dev_blind_mounted()==0) unmount_dev_blind();
		Graphics->NoRSX_Exit();
		ioPadEnd();
	}
	return 0;
}