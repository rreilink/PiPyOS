/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    templates/hal_lld.c
 * @brief   HAL Driver subsystem low level driver source template.
 *
 * @addtogroup HAL
 * @{
 */

#include "ch.h"
#include "hal.h"
#include "bcmframebuffer.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

void bcm2835_register_interrupt(unsigned int interrupt, void (*handler) (void *), void *closure);

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

static void (*interrupt_handler[64]) (void *);
static void *interrupt_handler_closure[64];

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/




void app_systick(void);

/**
 * @brief Process system timer interrupts, if present.
 *
 * @notapi
 */
static void systimer_serve_interrupt( void * dummy )
{
  SYSTIMER_CMP1 += (1000000/CH_FREQUENCY);

  // Update the system time
  chSysLockFromIsr();
  chSysTimerHandlerI();
  chSysUnlockFromIsr();

  SYSTIMER_CS = SYSTIMER_CS_MATCH1; //write to clear bit
  
  //app_systick();

}

/**
 * @brief Start the system timer
 *
 * @notapi
 */
static void systimer_init( void )
{
  SYSTIMER_CMP1 = SYSTIMER_CLO + (1000000/CH_FREQUENCY);
  SYSTIMER_CS = SYSTIMER_CS_MATCH1; //write to clear bit
  
  IRQ_ENABLE1 = (1<<1);
  bcm2835_register_interrupt(1, systimer_serve_interrupt, NULL);
}


/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/**
 * @brief Interrupt handler
 *
 */

CH_IRQ_HANDLER(IrqHandler)
{
  CH_IRQ_PROLOGUE();
  asm volatile ("stmfd    sp!, {r4-r11}" : : : "memory"); //  These are not saved by the IRQ PROLOGUE

  uint64_t pending;
  do {
    pending = IRQ_PEND1 | ((uint64_t) IRQ_PEND2) << 32;
    
    if (!pending) break;
    
    for (int i = 63; i>=0; i--, pending<<=1) {
        if (pending & (1LL<<63)) {
            interrupt_handler[i](interrupt_handler_closure[i]);
            break;
        }
    }
    
  } while (1);
  
  asm volatile ("ldmfd   sp!, {r4-r11}" : : : "memory"); //  These are not saved by the IRQ PROLOGUE

  CH_IRQ_EPILOGUE();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief Synchronize function for short delays.
 *
 */
void delayMicroseconds(uint32_t n)
{
  uint32_t compare = SYSTIMER_CLO + n;
  while (SYSTIMER_CLO < compare);
}

static void unused_interrupt(int interrupt) {
    // TODO: log / panic?
}

/**
 * @brief   Low level HAL driver initialization.
 *
 * @notapi
 */
void hal_lld_init(void) {
  for(int i=0;i<64;i++) {
    bcm2835_register_interrupt(i, (void (*)(void *))unused_interrupt, (void *)i);
  }
  //bcm2835_register_interrupt(54, (void (*)(void *))spi_lld_serve_interrupt, &SPI0);
  systimer_init();
}

void bcm2835_register_interrupt(unsigned int interrupt, void (*handler) (void *), void *closure) {
    if (interrupt<64) {
        interrupt_handler[interrupt] = handler;
        interrupt_handler_closure[interrupt] = closure;
    }
}

/**
 * @brief Start watchdog timer
 */
void watchdog_start ( uint32_t timeout )
{
    /* Setup watchdog for reset */
    uint32_t pm_rstc = PM_RSTC;

    //* watchdog timer = timer clock / 16; need password (31:16) + value (11:0) */
    uint32_t pm_wdog = PM_PASSWORD | (timeout & PM_WDOG_TIME_SET); 
    pm_rstc = PM_PASSWORD | (pm_rstc & PM_RSTC_WRCFG_CLR) | PM_RSTC_WRCFG_FULL_RESET;
    PM_WDOG = pm_wdog;
    PM_RSTC = pm_rstc;
}

/**
 * @brief Start watchdog timer
 */
void watchdog_stop ( void )
{
  PM_RSTC = PM_PASSWORD | PM_RSTC_RESET;
}

/**
 * @brief Get remaining watchdog time.
 */
uint32_t watchdog_get_remaining ( void )
{
  return PM_WDOG & PM_WDOG_TIME_SET;
}

/** @} */
