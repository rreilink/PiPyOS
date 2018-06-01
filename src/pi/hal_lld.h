
#ifndef _HAL_LLD_H_
#define _HAL_LLD_H_

#define PLATFORM_NAME   "BCM2835"

#define HAL_IMPLEMENTS_COUNTERS 1
#define PAL_MODE_OUTPUT 0xFF

typedef uint32_t halrtcnt_t;

#define hal_lld_get_counter_value() SYSTIMER_CLO
#define hal_lld_get_counter_frequency() 1000000

#ifdef __cplusplus
extern "C" {
#endif
  void hal_lld_init(void);
  void hal_register_interrupt(unsigned int interrupt, void (*handler) (void *), void *closure);
  void delayMicroseconds(uint32_t n);


#ifdef __cplusplus
}
#endif

#endif /* _HAL_LLD_H_ */


