/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "ch.h"
#include "hal.h"
#include "test.h"
#include "shell.h"
#include "chprintf.h"
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "adaptors/bcmmailbox.h"


char cmdline[1024];



static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *p) {
  (void)p;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palClearPad(ONBOARD_LED_PORT, ONBOARD_LED_PAD);
    palClearPad(GPIO18_PORT, GPIO18_PAD);
    chThdSleepMilliseconds(100);
    palSetPad(ONBOARD_LED_PORT, ONBOARD_LED_PAD);
    palSetPad(GPIO18_PORT, GPIO18_PAD);
    chThdSleepMilliseconds(900);
  }
  return 0;
}

static wchar_t *argv[] = { L"python", L"-B", L"-i", L"-c", L"import _rpi as r" };
extern const char *Py_FileSystemDefaultEncoding;




int Py_Main(int argc, wchar_t **argv);
void PiPyOS_initreadline(void);

static WORKING_AREA(waPythonThread, 1048576);
static msg_t PythonThread(void *p) {
  (void)p;
  chRegSetThreadName("python");
  
  setenv("PYTHONHASHSEED", "0", 1); // No random numbers available
  setenv("PYTHONHOME", "/", 1);
  setenv("HOME", "/", 1); // prevent import of pwdmodule in posixpath.expanduser

  Py_FileSystemDefaultEncoding = "latin-1";

  
  printf("GOGOGO!\n");
  
  PiPyOS_initreadline();
  
  Py_Main(5, argv);
  return 0;
}


static inline void dmb(void) {
    __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory");
}



void initcache(void)
{
/* Set up page tables so that data caches work. */    
    static volatile __attribute__ ((aligned(0x4000))) unsigned PageTable[4096];

    unsigned base;
    for (base = 0; base < 512; base++)
    {
        // outer and inner write back, write allocate, shareable
        PageTable[base] = (base << 20) | 0x1140E;
    }
    for (; base < 4096; base++)
    {
        // shared device, never execute
        PageTable[base] = (base << 20) | 0x10416;
    }

    // restrict cache size to 16K (no page coloring)
    unsigned auxctrl;
    asm volatile ("mrc p15, 0, %0, c1, c0,  1" : "=r" (auxctrl));
    auxctrl |= 1 << 6;
    asm volatile ("mcr p15, 0, %0, c1, c0,  1" ::"r" (auxctrl));

    // set domain 0 to client (use 3 for Manager)
    //    asm volatile ("mcr p15, 0, %0, c3, c0, 0" ::"r" (3));
    asm volatile ("mcr p15, 0, %0, c3, c0, 0" ::"r" (1));

    // always use TTBR0
    asm volatile ("mcr p15, 0, %0, c2, c0, 2" ::"r" (0));

    // set TTBR0 (page table walk inner cacheable, outer non-cacheable, shareable memory)
    asm volatile ("mcr p15, 0, %0, c2, c0, 0" ::"r" (3 | (unsigned) &PageTable));//Use this to isolate L2 cache when using GPU.

    // invalidate data cache and flush prefetch buffer
    asm volatile ("mcr p15, 0, %0, c7, c5,  4" ::"r" (0) : "memory");
    asm volatile ("mcr p15, 0, %0, c7, c6,  0" ::"r" (0) : "memory");
    // invalidate L2 instruction cache 
    //    asm volatile ("mcr p15, 1, %0, c7, c5,  0" ::"r" (0) : "memory");

    // enable MMU, L1 cache and instruction cache, L2 cache, write buffer,
    //   branch prediction and extended page table on
    unsigned mode;
    asm volatile ("mrc p15,0,%0,c1,c0,0" : "=r" (mode));
    mode |= 0x0005187D; //This gives 4 ns loop times.
    //vector processors enabled// 
    asm volatile ("mcr p15,0,%0,c1,c0,0" ::"r" (mode) : "memory");
    asm volatile ("mrc p15,0,%0,c1,c0,2" : "=r" (mode));//Coprocessor control
    mode |= 0x00F00000;
    asm volatile ("mcr p15,0,%0,c1,c0,2" :: "r" (mode));//Full access to VFP processors.
    
/*
    asm volatile ("vmrs %0,FPEXC":"=r" (mode));//Get current status
    mode |= 0x40000000;
    asm volatile ("vmsr FPEXC,%0"::"r" (mode));//Enable VFP coprocessors.  
    
    asm volatile ("vmrs %0,FPSCR":"=r" (mode));//Get VFP current status register.
    mode |= (BIT(9)|BIT(24)|BIT(25));
    asm volatile ("vmsr FPSCR,%0"::"r" (mode));//Enable divide by zero exceptions.  
    */
}

void FiqHandlerInit(void);

/*
 * Application entry point.
 */
int main(void) {
  const SerialConfig serialConfig ={921600};
  halInit();
  chSysInit();

  /*
   * Serial port initialization.
   */
  sdStart(&SD1, &serialConfig); 
  chprintf((BaseSequentialStream *)&SD1, "Main (SD1 started)\r\n");
  PiPyOS_bcm_framebuffer_init(0, 0);
  initcache();

  int cmdline_length = PiPyOS_bcm_get_property_tag(0x50001, cmdline, sizeof(cmdline)-1);
  if (cmdline_length <0) {
    *cmdline = '\0';
  } else {
    cmdline[cmdline_length] = '\0'; // Add trailing null
  }

  chprintf((BaseSequentialStream *)&SD1, "cmdline='%s'\r\n", cmdline);

//asm volatile ("mrs %0, cpsr" : "=rm" (cpsr));

  /*
   * Shell initialization.
   */
 /* shellInit();
  shellCreate(&shell_config, SHELL_WA_SIZE, NORMALPRIO + 1);
*/
  /*
   * Set mode of onboard LED
   */
  palSetPadMode(ONBOARD_LED_PORT, ONBOARD_LED_PAD, PAL_MODE_OUTPUT);
  palSetPadMode(GPIO18_PORT, GPIO18_PAD, PAL_MODE_OUTPUT);

  /*
  FiqHandlerInit();
  SYSTIMER_CS = SYSTIMER_CS_MATCH1; //write to clear bit
  IRQ_FIQ_CONTROL = 0x80|1;
  */
  
  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  
  /*
   * Creates the python thread.
   */
  chThdCreateStatic(waPythonThread, sizeof(waPythonThread), NORMALPRIO, PythonThread, NULL);

  /*
   * Events servicing loop.
   */
  chThdWait(chThdSelf());

  return 0;
}
