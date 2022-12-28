# Memory Explorer
***Functionality***
- Retrieve physical memory addresses  (IOCTL_LookupPage)
- Update page table entries           (IOCTL_ModifyPage)
- Read & write memory                 (IOCTL_ReadMemory & IOCTL_WriteMemory)
---------------------------
***Written and tested on Windows 10 1709***
- Previous versions should work, just update kernel structure offsets for whatever version used.
- Versions after 1709 will not work as Microsoft no longer allows mapping a PML4 into virtual memory.
---------------------------
***This driver does not attach to a target process, rather it walks a process's page table in order to read/write/modify memory.***
