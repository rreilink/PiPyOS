/*
  Raspberry Pi UART low level driver
  (C) Rob Reilink, 2018
*/


#include "ch.h"
#include "hal.h"
#include "bcmmailbox.h"

void PiPyOS_InterruptReceived(void);

#if HAL_USE_SERIAL || defined(__DOXYGEN__)

#define UART_DR         REG(0x20201000)
#define UART_RSRECR     REG(0x20201004)
#define UART_FR         REG(0x20201018)
#define UART_ILPR       REG(0x20201020)
#define UART_IBRD       REG(0x20201024)
#define UART_FBRD       REG(0x20201028)
#define UART_LCRH       REG(0x2020102C)
#define UART_CR         REG(0x20201030)
#define UART_IFLS       REG(0x20201034)
#define UART_IMSC       REG(0x20201038)
#define UART_RIS        REG(0x2020103C)
#define UART_MIS        REG(0x20201040)
#define UART_ICR        REG(0x20201044)
#define UART_DMACR      REG(0x20201048)
#define UART_ITCR       REG(0x20201080)
#define UART_ITIP       REG(0x20201084)
#define UART_ITOP       REG(0x20201088)
#define UART_TDR        REG(0x2020108C)


#define UART_INT_RT     BIT(6)
#define UART_INT_TX     BIT(5)
#define UART_INT_RX     BIT(4)
#define UART_INT_ALL    0x7F2;


#define UART_FR_RXFE    BIT(4)
#define UART_FR_TXFF    BIT(5)

#define UART_LCRH_WLEN_8 (BIT(6) | BIT(5))
#define UART_LCRH_FEN   BIT(4)

#define UART_CR_RXE     BIT(9)
#define UART_CR_TXE     BIT(8)
#define UART_CR_UARTEN  BIT(0)

#define UART_IFLS_RXIFLSEL_1_8 (0<<3)
#define UART_IFLS_RXIFLSEL_1_4 (1<<3)
#define UART_IFLS_RXIFLSEL_1_2 (2<<3)
#define UART_IFLS_RXIFLSEL_3_4 (3<<3)
#define UART_IFLS_RXIFLSEL_7_8 (4<<3)

#define UART_IFLS_TXIFLSEL_1_8 (0)
#define UART_IFLS_TXIFLSEL_1_4 (1)
#define UART_IFLS_TXIFLSEL_1_2 (2)
#define UART_IFLS_TXIFLSEL_3_4 (3)
#define UART_IFLS_TXIFLSEL_7_8 (4)


//#define TRACE(msg)  PiPyOS_bcm_framebuffer_putstring(msg, -1)
#define TRACE(msg)


/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

void uart_send ( uint32_t c );
void uart_sendstr (const char *s);

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

SerialDriver SD1;

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/**
 * @brief   Driver default configuration.
 */
static const SerialConfig default_config = {
  115200 /* default baud rate */
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void output_notify(GenericQueue *qp) {
  (void)(qp);  
  /* Enable tx interrupts.*/
  UART_IMSC |= UART_INT_TX; 
}

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/


void sd_lld_serve_interrupt( SerialDriver *sdp ) { // TODO: could also be static

  if (UART_MIS & (UART_INT_RX | UART_INT_RT)) { // RX or RX timeout interrupt
    chSysLockFromIsr();
    while(!(UART_FR & UART_FR_RXFE)) {
      unsigned int data;
      data = UART_DR;
      if ((data & 0xf00)==0) {
        // No errors
        if ((data & 0xff) == 3) PiPyOS_InterruptReceived(); // catch ctrl+c
        
        sdIncomingDataI(sdp, data & 0xFF);
      }
    };
    chSysUnlockFromIsr();
  }

  if (UART_MIS & UART_INT_TX) {
    TRACE("TX");
    chSysLockFromIsr();
    while(!(UART_FR & UART_FR_TXFF)) {
        msg_t data = sdRequestDataI(sdp);
        if (data < Q_OK) {
            TRACE("!");

            /* Disable tx interrupts.*/
            UART_IMSC &= ~UART_INT_TX; 
            break;
        }
        else {
            UART_DR = data;
            TRACE(".");

        }
    }
    chSysUnlockFromIsr();
  }
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level serial driver initialization.
 *
 * @notapi
 */
void sd_lld_init(void) {
  sdObjectInit(&SD1, NULL, output_notify);
}

/**
 * @brief   Low level serial driver configuration and (re)start.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 * @param[in] config    the architecture-dependent serial driver configuration.
 *                      If this parameter is set to @p NULL then a default
 *                      configuration is used.
 *
 * @notapi
 */
void sd_lld_start(SerialDriver *sdp, const SerialConfig *config) {
  (void)(sdp);

  unsigned int base_clock_rate, divider;

  if (config == NULL)
    config = &default_config;
  
  base_clock_rate = 48000000;
    
  DataMemBarrier();

  IRQ_DISABLE2 = BIT(57-32);

  DataMemBarrier();

  UART_CR = 0;
  UART_IMSC = 0;
  UART_LCRH = 0;

  DataMemBarrier();
  
  bcm2835_gpio_fnsel(14, GPFN_ALT0);
  bcm2835_gpio_fnsel(15, GPFN_ALT0);

  GPPUD = 0;
  bcm2835_delay(150);
  GPPUDCLK0 = (1<<14)|(1<<15);
  bcm2835_delay(150);
  GPPUDCLK0 = 0;
  
  DataMemBarrier();

    
  // Calculate divider using 6 fractional bits
  divider = (base_clock_rate << 6) / (16*1000000); //TODO: use config baud rate
  
  UART_IBRD = divider >> 6;
  UART_FBRD = divider & 0x3F;

  UART_LCRH = UART_LCRH_WLEN_8 | UART_LCRH_FEN;

  // TODO: select appropriate FIFO interrupt levels
  UART_IFLS = UART_IFLS_RXIFLSEL_1_2 | UART_IFLS_TXIFLSEL_1_2;

  UART_ICR = UART_INT_ALL;

  UART_CR = UART_CR_RXE | UART_CR_TXE | UART_CR_UARTEN;
  
  UART_IMSC = UART_INT_RT | UART_INT_RX;
  
  // The UART needs its TX FIFO above threshold once to work
  // This is a work-around, TODO: fix
  uart_sendstr("TEST123456\n"); 

  hal_register_interrupt(57, (void (*)(void *))sd_lld_serve_interrupt, &SD1);


  IRQ_ENABLE2 = BIT(57-32);
}

/**
 * @brief   Low level serial driver stop.
 * @details De-initializes the USART, stops the associated clock, resets the
 *          interrupt vector.
 *
 * @param[in] sdp       pointer to a @p SerialDriver object
 *
 * @notapi
 */
void sd_lld_stop(SerialDriver *sdp) {
  (void)(sdp);

  IRQ_DISABLE2 = BIT(57-32);
  bcm2835_gpio_fnsel(14, GPFN_IN);
  bcm2835_gpio_fnsel(15, GPFN_IN);
}


uint32_t uart_recv ( void )
{
  while((UART_FR & UART_FR_RXFE));
  return(UART_DR & 0xFF);
}

void uart_send ( uint32_t c )
{
  while((UART_FR & UART_FR_TXFF));
  UART_DR = c;
}

void uart_sendstr (const char *s)
{
  char c;
  while((c = *s++)) uart_send(c);
}


#endif /* HAL_USE_SERIAL */

/** @} */
