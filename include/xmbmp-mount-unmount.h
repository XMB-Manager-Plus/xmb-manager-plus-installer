#ifndef __XMBMP_MOUNT_UNMOUNT_H__
#define __XMBMP_MOUNT_UNMOUNT_H__
#include "xmbmp-syscalls.h"

int is_dev_blind_mounted();
int mount_dev_blind();
int unmount_dev_blind();

#endif
