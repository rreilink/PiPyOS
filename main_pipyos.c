
#include "ch.h"
#include "hal.h"
#include "test.h"
#include "shell.h"
#include "chprintf.h"
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "bcmmailbox.h"
#include "bcmframebuffer.h"

#include "os.h"
#include "ff.h"
#include "bcm2835.h"

char cmdline[1025];



static WORKING_AREA(waThread1, 128);
static msg_t Thread1(void *p) {
  (void)p;
  chRegSetThreadName("blinker");
  while (TRUE) {
    palClearPad(GPIO47_PORT, GPIO47_PAD);
    chThdSleepMilliseconds(100);
    palSetPad(GPIO47_PORT, GPIO47_PAD);
    chThdSleepMilliseconds(900);
  }
  return 0;
}

//static wchar_t *argv[] = { L"python", L"-S", L"-B", L"-i", L"-c", L"import _rpi as r, app as a, zipimport; zipimport.zipimporter('/sd/python36.zip')" };

static wchar_t *argv[] = { L"python", L"-S", L"-B", L"-i", L"/boot/app/gcode.py" };
extern const char *Py_FileSystemDefaultEncoding;




int Py_Main(int argc, wchar_t **argv);
void PiPyOS_initreadline(void);
void Py_SetPath(const wchar_t *);

void pitft_test(void);

static WORKING_AREA(waPythonThread, 1048576);

static msg_t PythonThread(void *p) {
  (void)p;
  chRegSetThreadName("python");
  
  setenv("PYTHONHASHSEED", "0", 1); // No random numbers available
  setenv("PYTHONHOME", "/", 1);
  setenv("HOME", "/", 1); // prevent import of pwdmodule in posixpath.expanduser

  Py_FileSystemDefaultEncoding = "latin-1";
  Py_SetPath(L"/boot:/boot/app:/sd/python36z.zip");
  
  printf("GOGOGO!\n");

  FATFS fs;
  f_mount(&fs, "", 0);

  
  PiPyOS_initreadline();
  
  Py_Main(3, argv);
  

  
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
  const SerialConfig serialConfig ={1000000};
  const SDCConfig sdccfg = { 0 }; 
  
  AUX_MU_CNTL_REG = 0; // disable uart RX during setup of HAL. This prevents a random char to appear in the input buffer
  
  halInit();
  chSysInit();

  app_init();

  PiPyOS_bcm_framebuffer_init(0, 0);
  PiPyOS_bcm_framebuffer_putstring("init\n", -1);
  
  /*
   * Serial port initialization.
   */

  sdStart(&SD1, &serialConfig); 
  
  chprintf((BaseSequentialStream *)&SD1, "Main (SD1 started)\r\n");
  os_init_stdio();
  
  initcache();

  int cmdline_length = PiPyOS_bcm_get_property_tag(0x50001, cmdline, sizeof(cmdline)-1);
  if (cmdline_length <0) {
    *cmdline = '\0';
  } else {
    cmdline[cmdline_length] = '\0'; // Add trailing null
  }

  chprintf((BaseSequentialStream *)&SD1, "cmdline='%s'\r\n", cmdline);

  sdcStart(&SDCD1, &sdccfg);

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
  palSetPadMode(GPIO47_PORT, GPIO47_PAD, PAL_MODE_OUTPUT);

  
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
