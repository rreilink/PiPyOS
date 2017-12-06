/* Framebuffer and terminal emulator
 *
 * Only supports fixed font size (8x16) and 8-bit color
 * These were chosen for speed
 */


#include <sys/types.h>
#include <string.h>
#include <stdio.h>

#include "bcmmailbox.h"
#include "assert.h"
#include "deps/font.h"

static struct {
    uint32_t phys_width;
    uint32_t phys_height;
    uint32_t virt_width;
    uint32_t virt_height;
    uint32_t pitch;
    uint32_t depth;
    uint32_t offset_x;
    uint32_t offset_y;
    void*    buffer;
    uint32_t size;
    uint16_t palette[256];
    
} __attribute__ ((aligned (16))) fb_info;


static int _posx, _posy;
static uint8_t _cursorcopy[2*CHARWIDTH]; // copy of the data under the cursor
static int _fgcolor, _bgcolor, _cursorcolor;


static inline uint8_t *_ptr_for_char(int x, int y) {
    return fb_info.buffer + _posx * CHARWIDTH + _posy * CHARHEIGHT * fb_info.pitch;
}

/* Write a cursor to the screen, save the pixels that were overwritten to 
 * _cursorcopy so these can be restored by _erasecursor() when required
 */
static void _writecursor() {
    uint8_t *p = _ptr_for_char(_posx, _posy);
    
    p+=fb_info.pitch*(CHARHEIGHT-4);
    memcpy(_cursorcopy, p, CHARWIDTH);
    memcpy(_cursorcopy+CHARWIDTH, p+fb_info.pitch, CHARWIDTH);

    memset(p, _cursorcolor, CHARWIDTH);
    memset(p+fb_info.pitch, _cursorcolor, CHARWIDTH);
}

/* Erase the cursor to the screen, restore the pixels that were saved to 
 * _cursorcopy.
 */
static void _erasecursor() {
    uint8_t *p = _ptr_for_char(_posx, _posy);
    
    p+=fb_info.pitch*(CHARHEIGHT-4);
    memcpy(p, _cursorcopy, CHARWIDTH);
    memcpy(p+fb_info.pitch, _cursorcopy+CHARWIDTH, CHARWIDTH);
}



/* Initialize framebuffer
 *
 * width, height: dimensions of the framebuffer. If both are 0, these are
 * aquired from the VideoCore
 */

void PiPyOS_bcm_framebuffer_init(int width, int height) {
    int size[2];
    int i;
    
    if (width == 0 || height == 0) {
        memset(&fb_info, 0, sizeof(fb_info));
        PiPyOS_bcm_get_property_tag(0x40003, &size, 8);
        fb_info.phys_width = fb_info.virt_width = size[0];
        fb_info.phys_height = fb_info.virt_height = size[1];

    } else {
        fb_info.phys_width = fb_info.virt_width = width;
        fb_info.phys_height = fb_info.virt_height = height;
    }
    
    fb_info.phys_width = fb_info.virt_width = size[0];
    fb_info.phys_height = fb_info.virt_height = size[1];
    
    fb_info.depth = 8;
    
    for(i=0;i<8;i++) { // initialize a 4-bit palette
        fb_info.palette[i]= ((i&4) ? 0xa800 : 0 ) | ((i&2) ? 0x580 : 0) | ((i&1) ? 0x15 : 0);    // normal color
        fb_info.palette[i+8]= ((i&4) ? 0xf800 : 0 ) | ((i&2) ? 0x7e0 : 0) | ((i&1) ? 0x1f : 0); // extra bright color
    }
    

    PiPyOS_bcm_mailbox_write_read(1, &fb_info); 

    assert(fb_info.buffer);
    
    _bgcolor = 1;
    _cursorcolor = 7;
    _fgcolor = 7;
    
    memset(fb_info.buffer, _bgcolor, fb_info.size); // test: blue screen
    
    _posx = 0;
    _posy = 0;
    _writecursor();
}


/* count = -1: write upto \n
 * count >=0: write count chars
 */

void PiPyOS_bcm_framebuffer_putstring(char *s, int count) {
    char c;
    uint8_t color;
    uint8_t *p, *p2;
    
    int escapestate = 0, escapeparam = 0;
    
    
    /* Some optimization: we only write the cursor when we are done writing 
     * the string. When the first char is written, it overwrites the cursor.
     * So we only need to erase any previous cursor, when the cursor is moved
     * (e.g. \r, \n) before any char is written. cursorvisible keeps track of
     * wether or not we need to remove the cursor in those cases.
     */
    int cursorvisible = 1;
    
    while(1) {
        c = *s++;
        
        if (count==-1) {
            if (!c) break;  // count == -1: write until \0
        } else {
            if (count == 0) break;
            count--;
        }

        p = _ptr_for_char(_posx, _posy);

        // minimal terminal emulator to support the codes emitted by _readline.py
        if (c=='\x1b') {
            escapestate = 1;
            continue;
        }
        
        switch (escapestate) {
        case 1:
            if (c == '[') {
                escapestate++;
                escapeparam = 0;
                continue;
            }
            break;
        case 2:
            if (c>='0' && c<='9') {
                escapeparam = escapeparam * 10 + (c-'0');
                continue;
            }
            if (c=='K') { // clear till end of line
                // no need to erase cursor, we will clear the entire line
                for (int i=0; i<CHARHEIGHT; i++) {
                    memset(p, _bgcolor, fb_info.pitch - (_posx*CHARWIDTH));
                    p+=fb_info.pitch;
                }
            } else if (c=='D') { // back x positions
                if (cursorvisible) _erasecursor();
                if (escapeparam == 0) escapeparam = 1;
                if (_posx > escapeparam) _posx-=escapeparam; else _posx = 0; 
            } else if (c=='C') { // forward x positions
                if (cursorvisible) _erasecursor();
                if (escapeparam == 0) escapeparam = 1;
                _posx += escapeparam;
                goto fixcursorpos; // correct passing end-of-line / end-of-screen
            }
            
            escapestate = 0;
            continue; 
        }
    

        if (c>=0x20) { 
            // 'Normal' char for which we have an image; font table starts at 0x20
            c-=0x20;
            
            // Loop over rows and columns, write character bitmap to framebuffer
            for(int y=0;y<CHARHEIGHT;y++) {
                p2 = p;
                for(int mask=(1<<CHARWIDTH); mask; mask>>=1) {
                    color = _bgcolor;
                    if (font_data[(int)c][y] & mask) {
                        color = _fgcolor;
                    }
                    *p2++ = color;
                }
                p += fb_info.pitch;
            }
            
            _posx+=1;
            cursorvisible = 0;
            
        } else {
            // 'Special' char handling
            switch(c) {
            case '\n':
                if (cursorvisible) _erasecursor();
                _posy += 1;
                _posx = 0;
                break;
            case '\r':
                if (cursorvisible) _erasecursor();
                _posx = 0;
                break;
            case '\t':
                if (cursorvisible) _erasecursor();
                _posx = (_posx+8) & ~7;
                break;
            default:
                continue; //ignore char
            }
        }

fixcursorpos:        
        // check end-of-line
        if ((_posx+1)*CHARWIDTH>fb_info.virt_width) {
            _posx=0;
            _posy+=1;
        }
        // check end-of-screen
        if ((_posy+1)*CHARHEIGHT>fb_info.virt_height) {
            // scroll screen
            _posy-=1;
            memcpy(fb_info.buffer, fb_info.buffer + fb_info.pitch * CHARHEIGHT, fb_info.pitch * _posy * CHARHEIGHT);
            memset(fb_info.buffer + fb_info.pitch * _posy * CHARHEIGHT, _bgcolor, fb_info.pitch*CHARHEIGHT);
        }
    }
    // Only write cursor to screen when we are done
    _writecursor();
}



