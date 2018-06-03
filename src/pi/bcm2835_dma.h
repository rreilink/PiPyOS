#ifndef _BCM2835_DMA_H_
#define _BCM2835_DMA_H_

#include "bcm2835.h"

/*
 * Registers
 */

typedef struct {
    volatile uint32_t cs;
    volatile uint32_t conblk_ad;
    volatile uint32_t ti;
    volatile uint32_t source_ad;
    volatile uint32_t dest_ad;
    volatile uint32_t txfr_len;
    volatile uint32_t stride;
    volatile uint32_t nextconblk;
    volatile uint32_t debug;
} bcm2835_dma_regs_t;


#define BCM2835_DMA(x)      ((bcm2835_dma_regs_t *) (0x20007000 + ((x)<<8)))
#define BCM2835_DMA_ENABLE  (* (volatile uint32_t *) (0x20007FF0))

/*
 * Register bits
 */


#define BCM2835_DMA_CS_ACTIVE           (1<<0)
#define BCM2835_DMA_CS_INT              (1<<2)

#define BCM2835_DMA_TI_PERMAP(permap)   (((permap) & 0x1f) << 16)
#define BCM2835_DMA_TI_SRC_IGNORE       (1<<11)
#define BCM2835_DMA_TI_SRC_DREQ         (1<<10)
#define BCM2835_DMA_TI_SRC_WIDTH        (1<<9)
#define BCM2835_DMA_TI_SRC_INC          (1<<8)
#define BCM2835_DMA_TI_DEST_IGNORE      (1<<7)
#define BCM2835_DMA_TI_DEST_DREQ        (1<<6)
#define BCM2835_DMA_TI_DEST_WIDTH       (1<<5)
#define BCM2835_DMA_TI_DEST_INC         (1<<4)
#define BCM2835_DMA_TI_WAIT_RESP        (1<<3)
#define BCM2835_DMA_TI_TDMODE           (1<<1)




#include <stdint.h>


typedef struct __attribute__ ((aligned (32))) {
    uint32_t ti;
    uint32_t source_ad;
    uint32_t dest_ad;
    uint32_t txfr_len;
    uint32_t stride;
    uint32_t nextconblk;
    uint32_t reserved[2];   
} bcm2835_dma_conblk;


typedef unsigned int bcm2835_dma_channel_t;

bcm2835_dma_channel_t bcm2835_dma_alloc(unsigned int lite);

void bcm2835_dma_fill_conblk(bcm2835_dma_conblk *conblk, 
    volatile void *destination, volatile const void *source, unsigned long length,
    unsigned long permap);

void bcm2835_dma_fill_conblk_2d(bcm2835_dma_conblk *conblk, 
    volatile void *destination, volatile const void *source, 
    long destination_stride, long source_stride,
    unsigned long xlength, unsigned long ylength,
    unsigned long permap);


static inline void bcm2835_dma_chain_conblk(bcm2835_dma_conblk *conblk, bcm2835_dma_conblk *next) {
    conblk->nextconblk = (uint32_t) next | 0x40000000; // L2 cached alias
}

void bcm2835_dma_reset(bcm2835_dma_channel_t channel);
int bcm2835_dma_start(bcm2835_dma_channel_t channel, bcm2835_dma_conblk *conblk);

int bcm2835_dma_isready(bcm2835_dma_channel_t channel);
void bcm2835_dma_register_interrupt(bcm2835_dma_channel_t channel, void (*handler) (void *), void *closure);
void bcm2835_dma_acknowledge_interrupt(bcm2835_dma_channel_t channel);

static inline void bcm2835_dma_enable_interrupt(bcm2835_dma_channel_t channel) {
    if (channel<=12) {
        IRQ_ENABLE1 = 1<<(channel+16);
    }
}

static inline void bcm2835_dma_disable_interrupt(bcm2835_dma_channel_t channel) {
    if (channel<=12) {
        IRQ_DISABLE1 = 1<<(channel+16);
    }
}

#endif
