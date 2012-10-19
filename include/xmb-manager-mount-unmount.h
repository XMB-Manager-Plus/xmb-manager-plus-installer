#ifndef __XMB_MANAGER_MOUNT_UNMOUNT_H__
#define __XMB_MANAGER_MOUNT_UNMOUNT_H__
#include "xmb-manager-syscalls.h"
#include "xmb-manager-include.h"


int is_dev_blind_mounted();
int mount_dev_blind();
int unmount_dev_blind();

#endif
