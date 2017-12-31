/* Adaptor that connects the FatFs library to the ChibiOS SDC driver */
#include <stdio.h>
#include "ch.h"
#include "hal.h"
#include "ff.h"
#include "diskio.h"


DSTATUS disk_initialize (BYTE pdrv) {
    if (pdrv!=0) return RES_PARERR;
    
    if (sdcConnect(&SDCD1)==CH_SUCCESS) {
        return 0;
    } else {
        return STA_NOINIT;
    }
}

DSTATUS disk_status (BYTE pdrv) {
    if (pdrv!=0) return STA_NOINIT;
    return STA_PROTECT;
}

DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count) {
    if (pdrv!=0) return RES_PARERR;
    if (sdcRead(&SDCD1, sector, buff, count)==CH_SUCCESS) {
        return RES_OK;
    } else {
        return RES_ERROR;
    }
}

DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count) {
    return RES_WRPRT;
}

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff) {
    printf("DISK IOCTL %d\n", cmd);
    return RES_ERROR;

}

DWORD get_fattime (void) {
    unsigned int year=2017, day=1, month=1;
    return ((year - 1980) << 25) | (month << 21) | (day << 16);
}
