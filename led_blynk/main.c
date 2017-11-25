#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define GPFSEL0 0x20200000
#define GPFSEL1 0x20200004
#define GPFSEL2 0x20200008
#define GPFSEL3 0x2020000C
#define GPFSEL4 0x20200010
#define GPFSEL5 0x20200014

#define GPSET0  0x2020001C
#define GPSET1  0x20200020
#define GPCLR0  0x20200028
#define GPCLR1  0x2020002C

#define SYST_CLO 0x20003004
#define SYST_CHI 0x20003008

extern uint32_t __bss_start, __bss_end;

__attribute__((naked)) __attribute__((section(".startup"))) \
void Init_Machine(void) {
  // set CPSR
  __asm volatile("ldr r0, =0x000000d3");
  __asm volatile("msr cpsr, r0");
  // set stack pointer
  __asm volatile("ldr sp, =0x06400000");
  // zero out .bss section
  for (uint32_t *dest = &__bss_start; dest < &__bss_end;) {
    *dest++ = 0;
  }
  __asm volatile("bl _start");
  __asm volatile("b .");
}

volatile unsigned long long get_systime(void) {
  unsigned long long t;
  unsigned int chi;
  unsigned int clo;

  chi = *(volatile unsigned int *) SYST_CHI;
  clo = *(volatile unsigned int *) SYST_CLO;
  if (chi != *(volatile unsigned int *) SYST_CHI) {
    chi = *(volatile unsigned int *) SYST_CHI;
    clo = *(volatile unsigned int *) SYST_CLO;
  }
  t = chi;
  t = t << 32;
  t += clo;
  return t;
}

void delay_ms(unsigned int delay){
  unsigned long long alermTime;

  alermTime = get_systime() + delay * 1000;
  while(get_systime() < alermTime);
  
  return;
}

int main(int argc, char **argv) {
  // RPi zero LED_STATUS = GPIO47
  // Set GPIO47 mode to output mode
  *(volatile unsigned int *)GPFSEL4 = 0x01 << (3*7);

  while(1) {
    // Set GPIO47 to Low (LED on)
    *(volatile unsigned int *)GPCLR1 = 0x01 << 15;
    delay_ms(500);
    // Set GPIO47 to High (LED off)
    *(volatile unsigned int *)GPSET1 = 0x01 << 15;
    delay_ms(500);
  }

  return 0;
}


void _start(void) {
  // when we get here: stack is initialised, bss is clear, data is copied

  // initialise the cpu and peripherals

  // now that we have a basic system up and running we can call main
  main(0, NULL);

  // we must not return
  for (;;) {
  }
}
