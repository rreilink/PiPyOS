#include "ch.h"
#include "hal.h"
#include "bcm2835_dma.h"
#include "bcm2835_spi.h"


#if HAL_USE_SPI || defined(__DOXYGEN__)


/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

SPIDriver SPI0;

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

static bcm2835_dma_channel_t rx_dma_channel;
static bcm2835_dma_channel_t tx_dma_channel;

static bcm2835_dma_conblk rx_conblk;
static bcm2835_dma_conblk tx_conblk;

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*
 * DMA-based transfer ready
 */ 
static void spi_lld_serve_dma_interrupt(SPIDriver *spip) {
  bcm2835_dma_acknowledge_interrupt(rx_dma_channel);
  
  _spi_isr_code(spip);
}

/*
 * Non-DMA based transfer ready. Read data from FIFO
 */
static void spi_lld_serve_spi_interrupt(SPIDriver *spip) {
  
  uint8_t *rxbuf = spip->rxbuf;
  uint8_t data = 0;

  while(spip->rxcnt--) {
    data = BCM2835_SPI->fifo;
    if (rxbuf) *rxbuf++ = data;
  }
  
  BCM2835_SPI->cs = (BCM2835_SPI->cs & ~(BCM2835_SPI_CS_TA | BCM2835_SPI_CS_INTD));
 
  _spi_isr_code(spip);
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level SPI driver initialization.
 *
 * @notapi
 */
void spi_lld_init(void) {
  spiObjectInit(&SPI0);
  
  //allocate DMA
  rx_dma_channel = bcm2835_dma_alloc(0);
  tx_dma_channel = bcm2835_dma_alloc(0);
}

/* Give other code (e.g. pitft display driver) read-access to our DMA channels
 *
 * The signature uses void* to prevent requiring inclusion of bcm2835_dma.h in
 * all code (since spi_lld.h is included via hal.h)
 */

void spi_lld_get_dma_channels(void *rx, void *tx) {
    *((bcm2835_dma_channel_t *)rx) = rx_dma_channel;
    *((bcm2835_dma_channel_t *)tx) = tx_dma_channel;    
}


void spi_lld_start(SPIDriver *spip) {
  unsigned int divider;
  const SPIConfig *cfg = spip->config;  
  
  bcm2835_dma_disable_interrupt(rx_dma_channel);

  if (cfg->chip_select == 1) bcm2835_gpio_fnsel(7, GPFN_ALT0);   /* SPI0_CE1_N.*/
  if (cfg->chip_select == 0) bcm2835_gpio_fnsel(8, GPFN_ALT0);   /* SPI0_CE0_N.*/
  bcm2835_gpio_fnsel(9, GPFN_ALT0);   /* SPI0_MOSI.*/
  bcm2835_gpio_fnsel(10, GPFN_ALT0);  /* SPI0_MISO.*/
  bcm2835_gpio_fnsel(11, GPFN_ALT0);  /* SPIO_SCLK.*/
  

  // Chip select polarity is rather vague in the BCM2835 datasheet
  // So only negative CS polarity is implemented
    
  BCM2835_SPI->cs = BCM2835_SPI_CS_CS(cfg->chip_select) 
            | (cfg->clock_phase    ? BCM2835_SPI_CS_CPHA : 0)
            | (cfg->clock_polarity ? BCM2835_SPI_CS_CPOL : 0);


  divider = 250000000L / cfg->clock_frequency;
  if (divider == 0) divider = 2;  // ensure >0
  if (divider & 1) divider++;     // ensure even; round up        

  BCM2835_SPI->clk = divider;

  bcm2835_dma_register_interrupt(rx_dma_channel, (void (*)(void *))spi_lld_serve_dma_interrupt, spip);
  
  bcm2835_dma_enable_interrupt(rx_dma_channel);

  hal_register_interrupt(54, (void (*)(void *))spi_lld_serve_spi_interrupt, spip);
  
  IRQ_ENABLE2 = BIT(54-32);
}

/**
 * @brief   Deactivates the SPI peripheral.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 *
 * @notapi
 */
void spi_lld_stop(SPIDriver *spip) {
  bcm2835_gpio_fnsel(7, GPFN_IN);
  bcm2835_gpio_fnsel(8, GPFN_IN);
  bcm2835_gpio_fnsel(9, GPFN_IN);
  bcm2835_gpio_fnsel(10, GPFN_IN);
  bcm2835_gpio_fnsel(11, GPFN_IN);
}

void spi_lld_select(SPIDriver *spip) {
  // Done automatically in hardware
}


void spi_lld_unselect(SPIDriver *spip) {
  // Done automatically in hardware
}



/**
 * @brief   Exchanges data on the SPI bus.
 * @details This asynchronous function starts a simultaneous transmit/receive
 *          operation.
 * @post    At the end of the operation the configured callback is invoked.
 * @note    The buffers are organized as uint8_t arrays for data sizes below or
 *          equal to 8 bits else it is organized as uint16_t arrays.
 *
 * @param[in] spip      pointer to the @p SPIDriver object
 * @param[in] n         number of words to be exchanged
 * @param[in] txbuf     the pointer to the transmit buffer
 * @param[out] rxbuf    the pointer to the receive buffer
 *
 * @notapi
 */
void spi_lld_exchange(SPIDriver *spip, size_t n,
                      const void *txbuf, void *rxbuf) {

  // Beyond the max supported by the peripheral. TODO: work around it
  if (n>65532) n = 65532;

  if (n<=16) {
    // Small transfer: do not use DMA. Load the data into the FIFO and use the 
    // SPI interrupt to read the data from the FIFO when completed
    BCM2835_SPI->cs = (BCM2835_SPI->cs & ~BCM2835_SPI_CS_DMAEN) 
        | BCM2835_SPI_CS_CLEAR_TX | BCM2835_SPI_CS_CLEAR_RX | BCM2835_SPI_CS_TA;
    
    const uint8_t *txbuf_b = txbuf;
    spip->rxbuf = rxbuf;
    spip->rxcnt = n;
    uint8_t data = 0;
    
    for(unsigned int i=0;i<n; i++) {
        if (txbuf_b) {
            data = *txbuf_b++;
        }
        BCM2835_SPI->fifo = data;
    }
    
    BCM2835_SPI->cs |= BCM2835_SPI_CS_INTD;

    return;
  }

  bcm2835_dma_reset(rx_dma_channel);
  bcm2835_dma_reset(tx_dma_channel);

  bcm2835_dma_fill_conblk(&tx_conblk, (void*) &(BCM2835_SPI->fifo), txbuf, n, 6);
  bcm2835_dma_fill_conblk(&rx_conblk, rxbuf, (void*) &(BCM2835_SPI->fifo), n, 7);

  BCM2835_SPI->cs = (BCM2835_SPI->cs & ~BCM2835_SPI_CS_TA) | BCM2835_SPI_CS_DMAEN | BCM2835_SPI_CS_ADCS;
  
  // Write DLEN and CS low byte via FIFO register (copy current config from CS)
  BCM2835_SPI->fifo = (n<<16)
        | BCM2835_SPI_CS_CLEAR_TX | BCM2835_SPI_CS_CLEAR_RX | BCM2835_SPI_CS_TA
        | (BCM2835_SPI->cs & ( BCM2835_SPI_CS_CS(3)|  BCM2835_SPI_CS_CPHA | BCM2835_SPI_CS_CPOL ));

  rx_conblk.ti |= 0x1; // INTEN
    
  bcm2835_dma_start(rx_dma_channel, &rx_conblk);
  bcm2835_dma_start(tx_dma_channel, &tx_conblk);


}


void spi_lld_ignore(SPIDriver *spip, size_t n) {
  spi_lld_exchange(spip, n, NULL, NULL);
}
void spi_lld_send(SPIDriver *spip, size_t n, const void *txbuf) {
  spi_lld_exchange(spip, n, txbuf, NULL);
}

void spi_lld_receive(SPIDriver *spip, size_t n, void *rxbuf) {
  spi_lld_exchange(spip, n, NULL, rxbuf);
}


uint16_t spi_lld_polled_exchange(SPIDriver *spip, uint16_t frame) {
  // Not implemented
  return 0;
}

#endif /* HAL_USE_SPI */

/** @} */
