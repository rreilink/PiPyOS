#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "bcmmailbox.h"

#define MAILBOX_BASE		(0x20000000 + 0xB880)

#define MAILBOX0_READ  		   (MAILBOX_BASE + 0x00)
#define MAILBOX0_STATUS 	       (MAILBOX_BASE + 0x18)
#define MAILBOX_STATUS_EMPTY	   0x40000000
#define MAILBOX1_WRITE         (MAILBOX_BASE + 0x20)
#define MAILBOX1_STATUS 	       (MAILBOX_BASE + 0x38)
#define MAILBOX_STATUS_FULL	   0x80000000





static inline unsigned int _read(unsigned int address) {
    unsigned int ret;
    DataMemBarrier();
    ret = *((unsigned long *)address);
    DataMemBarrier();
    return ret;
}

static inline void _write(unsigned int address, unsigned int value) {
    DataMemBarrier();
    *((unsigned long *)address) = value;
    DataMemBarrier();
}

void PiPyOS_bcm_mailbox_write_read(int channel, void *data) {

    assert(((unsigned int) data & 0xf)==0); // must be 16-byte aligned

    CleanDataCache ();
    DataSyncBarrier ();
    
    while(_read(MAILBOX1_STATUS) & MAILBOX_STATUS_FULL);
    _write(MAILBOX1_WRITE, channel | 0x40000000 | (unsigned int) data); //0x40000000 = GPU_CACHED_BASE
    
    unsigned long read;
    do {
        while (_read(MAILBOX0_STATUS) & MAILBOX_STATUS_EMPTY);
        read = _read(MAILBOX0_READ);
    } while ((read & 0xf) != channel);
    
    InvalidateDataCache ();
}

/* id:   tag id
 * data: pointer to data to be sent; also where to store the reply
 * size: size of data area
 *
 * returns: size of response
 *          -1 for error
 */
int PiPyOS_bcm_get_set_property_tag(int tagid, void *data, int size) {
    int buffersize = size + 32;
    int responsesize;
    char* buffer;
    unsigned long *buffer_aligned;
    
    if ((size & 0x3) != 0) return -1; //size must be whole words
    
    // get 16-byte aligned buffer
    buffer = malloc(buffersize + 15);
    if (!buffer) return -1;
    
    buffer_aligned = (void*)((((unsigned int)buffer)+15) & ~0xf);
    
    buffer_aligned[0] = buffersize;
    buffer_aligned[1] = 0; // CODE_REQUEST
    buffer_aligned[2] = tagid;
    buffer_aligned[3] = size;
    buffer_aligned[4] = 0;
    
    memcpy(&buffer_aligned[5], data, size);
    memset(((char*)&buffer_aligned[5]) + size, 0, 12); // add sentinel tag (0,0,0)

    
    PiPyOS_bcm_mailbox_write_read(8, (void*)buffer_aligned);
    
    if (buffer_aligned[1] != 0x80000000) {
        free(buffer);
        return -1;
    }
    
    responsesize = buffer_aligned[4] & ~(1<<31);
    if (responsesize>size) {
        free(buffer);
        return -1;
    }
    
    memcpy(data, &buffer_aligned[5], size);
    
    free(buffer);
    
    return responsesize;

}


/* Short-cut for tags that only need reading; clears data and calls
 * PiPyOS_bcm_get_set_property_tag. See PiPyOS_bcm_get_set_property_tag.
 */
int PiPyOS_bcm_get_property_tag(int tagid, void *data, int size) {
    memset(data, 0, size); // clear initial data
    return PiPyOS_bcm_get_set_property_tag(tagid, data, size);
}


