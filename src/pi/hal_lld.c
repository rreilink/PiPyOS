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

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/


/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

#define NUM_INTERRUPTS 72 // 64 GPU interrupts + 8 BASIC IRQs
static void (*interrupt_handler[NUM_INTERRUPTS]) (void *);
static void *interrupt_handler_closure[NUM_INTERRUPTS];



/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/


// Use ARM timer
static void systimer_serve_interrupt( void * dummy )
{
  // Update the system time
  chSysLockFromIsr();
  chSysTimerHandlerI();
  chSysUnlockFromIsr();
  
  app_systick();
  
  ARM_TIMER_CLI = 0;

}

static void systimer_init( void )
{
    // Configure 1MHz clock with 1kHz reload cycle
    ARM_TIMER_CTL = 0x003E0000;
    ARM_TIMER_LOD = 1000-1;
    ARM_TIMER_RLD = 1000-1;
    ARM_TIMER_DIV = 0x000000F9; //divide by 250
    ARM_TIMER_CLI = 0;
    ARM_TIMER_CTL = 0x003E00A2;
    

    IRQ_ENABLE_BASIC = (1<<0);
    hal_register_interrupt(64, systimer_serve_interrupt, NULL);
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
  uint32_t basic;
  do {
    pending = IRQ_PEND1 | ((uint64_t) IRQ_PEND2) << 32; // check GPU interrupts
    
    if (pending) {
    
        for (int i = 63; i>=0; i--, pending<<=1) {
            if (pending & (1LL<<63)) {
                interrupt_handler[i](interrupt_handler_closure[i]);
                break;
            }
        }
        continue; // check for more interrupts in IRQ_PEND1 or IRQ_PEND2
    }
    
    basic = IRQ_BASIC & 0xff;
    if (basic) {
        for (int i = 7; i>=0; i--, basic<<=1) {
            if (basic & (1<<7)) {
                interrupt_handler[i+64](interrupt_handler_closure[i+64]);
                break;
            }
        }
        continue; // check for more interrupts in IRQ_PEND1 or IRQ_PEND2
    }
    
    break; // No GPU and no BASIC interrupts; we are done
    
    
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
    PiPyOS_bcm_framebuffer_putstring("FATAL: an unused interrupt was triggered", -1);    
    for(;;);
}

/**
 * @brief   Low level HAL driver initialization.
 *
 * @notapi
 */
void hal_lld_init(void) {
  for(int i=0;i<NUM_INTERRUPTS;i++) {
    hal_register_interrupt(i, (void (*)(void *))unused_interrupt, (void *)i);
  }

  systimer_init();
}

void hal_register_interrupt(unsigned int interrupt, void (*handler) (void *), void *closure) {
    if (interrupt<NUM_INTERRUPTS) {
        interrupt_handler[interrupt] = handler;
        interrupt_handler_closure[interrupt] = closure;
    }
}


/** @} */
