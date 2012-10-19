#include "xmbmp-syscalls.h"

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

string get_firmware_info(string what)
{
	uint8_t platform_info[0x18];
	lv2_get_platform_info(platform_info);
	uint32_t fw = platform_info[0]* (1 << 16) + platform_info[1] *(1<<8) + platform_info[2];
	uint64_t targettype;
	lv2_get_target_type(&targettype);
	string type, fwvers;

	//check if current version is supported
	if (targettype==1) type="CEX";
	else if (targettype==2) type="DEX";
	else if (targettype==3) type="DECR";
	else type="Unknown";
	if (fw==0x40250) fwvers="4.25";
	else if (fw==0x40210) fwvers="4.21";
	else if (fw==0x40200) fwvers="4.20";
	else if (fw==0x40110) fwvers="4.11";
	else if (fw==0x30560) fwvers="3.56";
	else if (fw==0x30550) fwvers="3.55";
	else if (fw==0x30410) fwvers="3.41";
	else if (fw==0x30150) fwvers="3.15";
	else fwvers="0.00";

	if (what=="version") return fwvers;
	else return type;
}