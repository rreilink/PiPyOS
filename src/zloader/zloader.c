/*
 * This is the main code of the zloader
 *
 * zloader uncompresses a compressed image and then starts execution. 
 *
 * In order to allow transparent operation, the decompressed image must be 
 * stored at address 0x8000, which is where the Raspberry Pi start.elf loads
 * images and starts execution. However, obviously this is also where the 
 * loader is loaded at startup. To circumvent this problem, the startup code
 * in zloader_asm.s copies (relocates) the loader image including the compressed
 * image to a location at some other place in memory before calling the C
 * loader() function.
 *
 * Note that the linker script does not define a .bss section, which means
 * that no 0-initalized global variables (or local static variables) can 
 * exist in this file. (This would also require .bss zeroing code at startup,
 * which is unnecessary).
 *
 */


#include <stdint.h>
#include "zlib.h"


/*
 * Enable/disable debugging via UART. Note that there is no UART initialization
 * code in the loader; thus debugging only works when the UART is already
 * initialized by an even earlier loader (e.g. a serial bootloader). Leave
 * disabled for normal operation
 */
#define UART_DEBUG 0

#if UART_DEBUG

#define REG(x) (*(volatile uint32_t *)(x))
#define AUX_MU_IO_REG   REG(0x20215040)
#define AUX_MU_LSR_REG  REG(0x20215054)
#define SYSTIMER_CLO    REG(0x20003004)


void uart_putch(char c) {
    while(!(AUX_MU_LSR_REG & 0x20));
    AUX_MU_IO_REG = c;
}


void uart_puthex(unsigned long d) {
    int i;
    char c;
    for(i=0;i<8;i++) {
        c = (d>>28)&0xf;
        if (c>9) c+='A'- 10; else c+='0';
        uart_putch(c);
        d<<=4;
    }
    uart_putch('\n');
}

#endif

/* Memory locations defined by the linker script zloader.ld */
extern unsigned char _end,__reloc_base__, __reloc_target__, __reloc_end__; 

/* Minimalistic, 100% leaking malloc/free implementation suffices for the loader */
unsigned char *heap = (void*)0x4000000; //64 MB
void* zcalloc(voidpf ctx, uInt items, uInt size) {
    void *ret = heap;
    heap += (items*size);
    return ret;
}

void zcfree(voidpf ctx, void *ptr) {
}


/* This is the main code called by zloader_asm startup code. Upon return,
 * the startup code branches to 0x8000 to start execution.
 *
 * This is not called 'main' since the function signature is different.
 *
 * loader(size, t0)
 *   size: size of the compressed data in bytes
 *   t0:   value of the system timer SYSTIMER_CLO upon start of the
 *         startup code. Allows measureing startup and relocation time.
 */
int loader(unsigned long size, unsigned long t0) {
    int result;
    z_stream zst;

#if UART_DEBUG    
    unsigned long t1, t2;

    t1 = SYSTIMER_CLO;
    uart_putch('Z');
    
#endif

    // Set up z_stream struct with the compression information
    zst.opaque = NULL;
    zst.zalloc = zcalloc;
    zst.zfree = zcfree;
    zst.avail_in = size;
    zst.next_in = &_end+4; // Compressed data starts 1 word after the loader
    zst.next_out = &__reloc_base__; // Destination is where our loader was originally loaded
    zst.avail_out = 0x7000000;
    result = inflateInit2(&zst, 15);
    
    if (result) {
        for(;;); // Error, can do nothing but hang
    }
    
    result = inflate(&zst, Z_FINISH);
    
    if (result!=Z_STREAM_END) {
        for(;;); // Error, can do nothing but hang
    }

#if UART_DEBUG
    t2 = SYSTIMER_CLO;
    uart_putch('\n');
    uart_puthex(size);
    uart_puthex(t0);
        
    uart_puthex(t1-t0);
    uart_puthex(t2-t1);
    uart_putch('\n');
    
#endif
    
    return 0;
}
