#include "xmbmp-file.h"
#include "xmbmp-graphics.h"

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

	if (sysFsStat(path, &info) >= 0) return 0;
	return -1;
}

int exists_backups(string folder)
{
	return exists((folder+"/backups").c_str());
}

int mkdir_one(string dirtocreate)
{
	return mkdir(dirtocreate.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

int mkdir_full(string fullpath)
{
	string pathtocreate;
	unsigned int pos = 0;

	if (exists(fullpath.c_str())!=0)
	{
		do
		{
			pos=fullpath.find_first_of("/", pos+1);
			pathtocreate=fullpath.substr(0, pos+1);
			//Mess.Dialog(MSG_OK,("folder: "+sourcefile+" "+int_to_string(pos)+" "+int_to_string((int)dest.size()-1)).c_str());
			if (exists(pathtocreate.c_str())!=0)
			{
				if (mkdir_one(pathtocreate)!=0) return -1;
			}
		}
		while (pos != fullpath.size()-1);
	}
	return 0;
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

string correct_path(string dpath, int what)
{
	string cpath;

	cpath=dpath;
	if (what==1 || what==2) if (cpath.find("PS3~")!=string::npos) cpath.replace( cpath.find("PS3~"), 4, "");
	if (what==1 || what==2) replace(cpath.begin(), cpath.end(), '~', '/');
	if (what==2) if (cpath.find("dev_flash")!=string::npos) cpath.replace( cpath.find("dev_flash"), 9, "dev_blind");

	return "/"+cpath;
}

string get_app_folder(char* path)
{
	string folder;
	char * pch;
	int mcount=0;

	pch = strtok(path,"/");
	while (pch != NULL)
	{
		if (mcount<4) 
		{
			if (pch==(string)DEV_TITLEID) folder=folder+"/"+(string)APP_TITLEID;
			else folder=folder+"/"+pch;
		}
		mcount++;
		pch = strtok (NULL,"/");
	}
	return folder;
}

void check_firmware_changes(string folder)
{
	if (exists("/dev_flash/vsh/resource/explore/xmb/xmbmp.cfg")!=0 && exists_backups(folder)==0)
	{
		Mess.Dialog(MSG_OK,"The system detected a firmware change. All previous backups will be deleted.");
		string ret=recursiveDelete(folder+"/backups");
		if (ret == "") Mess.Dialog(MSG_OK,"All backups deleted!\nPress OK to continue.");
		else Mess.Dialog(MSG_ERROR,("Problem with delete!\n\nError: "+ret).c_str());
	}
}

int show_terms(string folder)
{
	if (exists((folder+"/terms-accepted.cfg").c_str())!=0)//terms not yet accepted
	{
		Mess.Dialog(MSG_OK,"Permission is hereby granted, FREE of charge, to any person obtaining a copy of this software and associated configuration files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, publish, distribute, sublicense, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n\nThe above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.");
		Mess.Dialog(MSG_YESNO_DYES,"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\nDo you accept this terms?");
		if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
		{
			create_file((folder+"/terms-accepted.cfg").c_str());
			return 0;
		}
		else return -1;
	}
	return 0;
}

void draw_copy(string title, const char *dirfrom, const char *dirto, const char *filename, string cfrom, double copy_currentsize, double copy_totalsize, int numfiles_current, int numfiles_total, size_t countsize)
{
	int sizeTitleFont = 40, sizeFont = 25;
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
	char buf[CHUNK], buf2[CHUNK]; // BUFSIZE default is 8192 bytes
	size_t size=0, countsize=0;
	time_t start, now;
	string ret="";

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
		draw_copy(title, dirfrom, dirto, filename, cfrom, copy_currentsize, copy_totalsize, numfiles_current, numfiles_total, countsize);
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
		if (difftime(now,start)>=1)
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
