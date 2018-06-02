#ifndef _BCM2835_SPI_H_
#define _BCM2835_SPI_H_

/*
 * Registers
 */

typedef struct {
    volatile uint32_t cs;
    volatile uint32_t fifo;
    volatile uint32_t clk;
    volatile uint32_t dlen;
    volatile uint32_t ltoh;
    volatile uint32_t dc;
} bcm2835_spi_regs_t;

#define BCM2835_SPI      ((bcm2835_spi_regs_t *) (0x20204000))


/*
 * Register bits
 */


#define BCM2835_SPI_CS_CS(x)    ((x&3)<<0) // Chip select 0,1,2
#define BCM2835_SPI_CS_CPHA     (1<<2)
#define BCM2835_SPI_CS_CPOL     (1<<3)
#define BCM2835_SPI_CS_CLEAR_TX (1<<4)
#define BCM2835_SPI_CS_CLEAR_RX (1<<5)

#define BCM2835_SPI_CS_TA       (1<<7)
#define BCM2835_SPI_CS_DMAEN    (1<<8)
#define BCM2835_SPI_CS_INTD     (1<<9)
#define BCM2835_SPI_CS_ADCS     (1<<11)
#define BCM2835_SPI_CS_RXD      (1<<17)
#define BCM2835_SPI_CS_TXD      (1<<18)

#define BCM2835_DMA_TI_WAITS(x) ((x&0x1f)<<21)


#endif