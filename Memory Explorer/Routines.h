#pragma once

#include "stdafx.h"



NTSTATUS CreateCloseDeviceRoutine(PDEVICE_OBJECT pDevice, PIRP pIrp);
NTSTATUS DeviceIoControlRoutine(PDEVICE_OBJECT pDevice, PIRP pIrp);