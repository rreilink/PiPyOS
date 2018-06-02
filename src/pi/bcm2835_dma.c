#include "bcm2835.h"
#include "bcm2835_dma.h"


static unsigned int dma_channels_alloced = 0; //Bitmask of dma channels in use

/* Allocate a DMA channel
 *
 * lite: if != 0, signal that this may be a lite channel
 * TODO: use the lite flag. Currently only returns full channels
 *
 * returns: 0 if failure (no channel available); channel if succes
 *
 * Thread-safety: this function is not thread-safe
 */
bcm2835_dma_channel_t bcm2835_dma_alloc(unsigned int lite) {
    for (int i = 1, mask = 2; i<=7; i++, mask<<=1) {
        if ((dma_channels_alloced & mask) == 0) {
            dma_channels_alloced |= mask;    // mark channel in use
            BCM2835_DMA_ENABLE |= mask; // enable channel
            return i;
        }
    }

    return 0; // No free channel
}


/* Fill a conblk
 *
 *
 * destination, source:
 *   address in ARM memory or ARM I/O space. If given in I/O space (0x20xx_xxxx),
 *   this is remapped to 0x7Exx_xxxx (the address in VC memory space as used by 
 *   the DMA controller. If given in memory space (other cases), this is remapped
 *   to 0x4xxx_xxxx (L2 cached)
 *
 * This method defines the ti (transfer information) field according to the
 * following rules:
 *
 *   if destination / source is NULL; data is not written resp. read
 *
 *   if destination / source is in I/O space, the address is not increased 
 *   for each transfer; if it is in memory it is increased. Additionally,
 *   if permap != 0, DREQ is used to gate writes resp. reads
 *
 *   if destination / source is in memory, 128-bit destination resp. source
 *   transfer width is used; 32 bits otherwise
 *
 *   if destination is I/O, WAIT_RESP is set
 *
 *   permap: peripheral DREQ that gates writes / reads
 */

void bcm2835_dma_fill_conblk(bcm2835_dma_conblk *conblk, 
    void *destination, const void *source, unsigned long length,
    unsigned long permap) {
 
    unsigned long ti = 0;
    
    if (!destination) {
        ti |= BCM2835_DMA_TI_DEST_IGNORE;
    } else if (((uint32_t)destination & 0xFF000000) == 0x20000000) {
        // To I/O
        if (permap != 0) ti |= BCM2835_DMA_TI_DEST_DREQ | BCM2835_DMA_TI_WAIT_RESP;
        destination = (void*) ((uint32_t) destination | 0x7E000000);
    } else {
        // To memory
        ti |= BCM2835_DMA_TI_DEST_INC | BCM2835_DMA_TI_DEST_WIDTH;
        destination = (void*) ((uint32_t) destination | 0x40000000);
    }
    
    if (!source) {
        ti |= BCM2835_DMA_TI_SRC_IGNORE;
    } else if (((uint32_t)source & 0xFF000000) == 0x20000000) {
        // To I/O
        if (permap != 0) ti |= BCM2835_DMA_TI_SRC_DREQ;
        source = (void*) ((uint32_t) source | 0x7E000000);
    } else {
        // To memory
        ti |= BCM2835_DMA_TI_SRC_INC | BCM2835_DMA_TI_SRC_WIDTH;
        source = (void*) ((uint32_t) source | 0x40000000);
    }    
    
    
    ti |= BCM2835_DMA_TI_PERMAP(permap);
    
    conblk->ti = ti;
    conblk->source_ad = (uint32_t) source;
    conblk->dest_ad = (uint32_t) destination;
    conblk->txfr_len = length;
    conblk->stride = 0;
    conblk->nextconblk = 0;
    conblk->reserved[0] = 0; 
    conblk->reserved[1] = 0; 
    
}

/* Fill a conblk for a 2D-mode transfer
 *
 * conblk, destination, source, permap: see bcm2835_dma_fill_conblk
 * ylength: how many xlength-transfers are transferred
 * xlength: the length of one x-transfer, in bytes
 * destination_stride: offset to add to the destination address after each x-transfer
 * source_stride: offset to add to the source address after each x-transfer
 */
void bcm2835_dma_fill_conblk_2d(bcm2835_dma_conblk *conblk, 
    void *destination, const void *source, 
    long destination_stride, long source_stride,
    unsigned long xlength, unsigned long ylength,
    unsigned long permap) {

    bcm2835_dma_fill_conblk(conblk, destination, source, 0, permap);
    
    conblk->txfr_len = ((ylength << 16) & 0x3fff0000) | (xlength & 0xffff);
    conblk->stride = ((destination_stride << 16) & 0xffff0000) | (source_stride & 0xffff);
    conblk->ti |= BCM2835_DMA_TI_TDMODE;
}


void bcm2835_dma_reset(bcm2835_dma_channel_t channel) {
    bcm2835_dma_regs_t *regs = BCM2835_DMA(channel);
    regs->cs = (1<<31);
}

/*
 * Start DMA transfer
 * returns 0 on success, !0 on failure
 */
int bcm2835_dma_start(bcm2835_dma_channel_t channel, bcm2835_dma_conblk *conblk) {
    bcm2835_dma_regs_t *regs = BCM2835_DMA(channel);
    if (regs->cs & BCM2835_DMA_CS_ACTIVE) {
        return -1; // Error: channel is already active
    }
    
    // Flush cache
    int cr = 0;
    __asm volatile ("mcr p15, 0, %0, c7, c14, 0" :: "r" (cr));
    
    // Start transfer
    regs->conblk_ad = ((unsigned long) conblk) | 0x40000000; // L1 cached alias
    regs->cs = BCM2835_DMA_CS_ACTIVE;
    
    return 0; // Succes
}



/*
 * Poll whether DMA channel is ready
 * returns 1 when it is ready, 0 when it is busy
 */

int bcm2835_dma_isready(bcm2835_dma_channel_t channel) {
    bcm2835_dma_regs_t *regs = BCM2835_DMA(channel);
    
    return (!(regs->cs & BCM2835_DMA_CS_ACTIVE));
}


/*
 * Register DMA complete interrupt handler for the given channel
 *
 * handler( ) is called with the provided closure as its argument
 */

void bcm2835_dma_register_interrupt(bcm2835_dma_channel_t channel, void (*handler) (void *), void *closure) {
    if (channel<=12) {
        hal_register_interrupt(channel+16, handler, closure);
    }
}

void bcm2835_dma_acknowledge_interrupt(bcm2835_dma_channel_t channel) {
    bcm2835_dma_regs_t *regs = BCM2835_DMA(channel);
    regs->cs = (regs->cs & ~BCM2835_DMA_CS_ACTIVE) | BCM2835_DMA_CS_INT; // clear ACTIVE, write 1 to clear INT
}
