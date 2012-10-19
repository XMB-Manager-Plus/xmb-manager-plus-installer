#ifndef __XMBMP_SYSCALLS_H__
#define __XMBMP_SYSCALLS_H__
#include <ppu-lv2.h>
#include <lv2/sysfs.h>

s32 lv2_get_platform_info(uint8_t platform_info[0x18]);
s32 sysFsMount(const char* MOUNT_POINT, const char* TYPE_OF_FILESYSTEM, const char* PATH_TO_MOUNT, int IF_READ_ONLY);
s32 sysFsUnmount(const char* PATH_TO_UNMOUNT);
u32 reboot_sys();
s32 lv2_get_target_type(uint64_t *type);

#endif // __XMBMP_SYSCALLS_H__
