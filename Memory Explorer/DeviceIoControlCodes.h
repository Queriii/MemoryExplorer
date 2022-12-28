#pragma once

#include "stdafx.h"



#define DeviceId 0x8000

#define LookupPage 0x800
#define IOCTL_LookupPage CTL_CODE(DeviceId, LookupPage, METHOD_BUFFERED, FILE_ALL_ACCESS)

#define ModifyPage 0x801
#define IOCTL_ModifyPage CTL_CODE(DeviceId, ModifyPage, METHOD_BUFFERED, FILE_ALL_ACCESS)

#define ReadMemory 0x802
#define IOCTL_ReadMemory CTL_CODE(DeviceId, ReadMemory, METHOD_BUFFERED, FILE_ALL_ACCESS)

#define WriteMemory 0x803
#define IOCTL_WriteMemory CTL_CODE(DeviceId, WriteMemory, METHOD_BUFFERED, FILE_ALL_ACCESS)