#ifndef _BCM2835_DMA_H_
#define _BCM2835_DMA_H_

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
    void *destination, const void *source, unsigned long length,
    unsigned long permap, bcm2835_dma_conblk *next);

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
