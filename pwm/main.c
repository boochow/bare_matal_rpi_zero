#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define IOREG(X)  (*(volatile uint32_t *) (X))

// GPIO registers
#define GPFSEL0 IOREG(0x20200000)
#define GPFSEL1 IOREG(0x20200004)
#define GPFSEL2 IOREG(0x20200008)
#define GPFSEL3 IOREG(0x2020000C)
#define GPFSEL4 IOREG(0x20200010)
#define GPFSEL5 IOREG(0x20200014)

#define GPF_INPUT  0U
#define GPF_OUTPUT 1U
#define GPF_ALT_0  4U
#define GPF_ALT_1  5U
#define GPF_ALT_2  6U
#define GPF_ALT_3  7U
#define GPF_ALT_4  3U
#define GPF_ALT_5  2U

// Mini UART registers
#define AUX_IRQ     IOREG(0x20215000)
#define AUX_ENABLES IOREG(0x20215004)

#define MU_IO   IOREG(0x20215040)
#define MU_IER  IOREG(0x20215044)
#define MU_IIR  IOREG(0x20215048)
#define MU_LCR  IOREG(0x2021504C)
#define MU_MCR  IOREG(0x20215050)
#define MU_LSR  IOREG(0x20215054)
#define MU_MSR  IOREG(0x20215058)
#define MU_SCRATCH  IOREG(0x2021505C)
#define MU_CNTL IOREG(0x20215060)
#define MU_STAT IOREG(0x20215064)
#define MU_BAUD IOREG(0x20215068)

#define MU_LSR_TX_IDLE  (1U << 6)
#define MU_LSR_TX_EMPTY (1U << 5)
#define MU_LSR_RX_RDY   (1U)

// System timer counter
#define SYST_CLO IOREG(0x20003004)
#define SYST_CHI IOREG(0x20003008)

// PWM registers

#define CM_PWMCTL  IOREG(0x201010a0)
#define CM_PWMDIV  IOREG(0x201010a4)

#define CM_PASSWD           (0x5a000000)
#define CM_PWMCTL_SRC_MASK  (0xfU)
#define CM_PWMCTL_SRC_OSC   (1U)
#define CM_PWMCTL_SRC_PLLA  (4U)
#define CM_PWMCTL_SRC_PLLC  (5U)
#define CM_PWMCTL_SRC_PLLD  (6U)
#define CM_PWMCTL_SRC_HDMI  (7U)
#define CM_PWMCTL_ENAB  (1U<<4)
#define CM_PWMCTL_KILL  (1U<<5)
#define CM_PWMCTL_BUSY  (1U<<7)
#define CM_PWMCTL_BUSYD (1U<<8)
#define CM_PWMCTL_MASH_NONE  (1U<<9)
#define CM_PWMCTL_MASH_2STG  (2U<<9)
#define CM_PWMCTL_MASH_3STG  (3U<<9)

#define PWM        (0x2020c000)
typedef volatile struct _pwm_t {
    uint32_t CTL;
    uint32_t STA;
    uint32_t DMAC;
    uint32_t undef1;
    uint32_t RNG1;
    uint32_t DAT1;
    uint32_t FIF1;
    uint32_t undef2;
    uint32_t RNG2;
    uint32_t DAT2;
} pwm_t;

#define CTL_MSEN2 (1<<15)
#define CTL_USEF2 (1<<13)
#define CTL_POLA2 (1<<12)
#define CTL_SBIT2 (1<<11)
#define CTL_RPTL2 (1<<10)
#define CTL_MODE2 (1<<9)
#define CTL_PWEN2 (1<<8)
#define CTL_MSEN1 (1<<7)
#define CTL_CLRF1 (1<<6)
#define CTL_USEF1 (1<<5)
#define CTL_POLA1 (1<<4)
#define CTL_SBIT1 (1<<3)
#define CTL_RPTL1 (1<<2)
#define CTL_MODE1 (1<<1)
#define CTL_PWEN1 (1)

#define STA_STA4  (1<<12)
#define STA_STA3  (1<<11)
#define STA_STA2  (1<<10)
#define STA_STA1  (1<<9)
#define STA_BERR  (1<<8)
#define STA_GAPO4 (1<<7)
#define STA_GAPO3 (1<<6)
#define STA_GAPO2 (1<<5)
#define STA_GAPO1 (1<<4)
#define STA_RERR1 (1<<3)
#define STA_WERR1 (1<<2)
#define STA_EMPT1 (1<<1)
#define STA_FULL1 (1)

#define DMAC_ENAB  (1<<31)
#define DMAC_PANIC (255<<8)
#define DMAC_DREQ  (255)

// This function must be at the top of main.c !!
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

void init_uart() {
  // set GPIO14, GPIO15 to aternate function 5
  GPFSEL1 = (GPF_ALT_5 << (3*4)) | (GPF_ALT_5 << (3*5));

  // UART basic settings
  AUX_ENABLES = 1;
  MU_CNTL = 0;   // mini uart disable
  MU_IER = 0;    // disable receive/transmit interrupts
  MU_IIR = 0xC6; // enable FIFO(0xC0), clear FIFO(0x06)
  MU_MCR = 0;    // set RTS to High

  // data and speed (mini uart is always parity none, 1 start bit 1 stop bit)
  MU_LCR = 3;    // 8 bits
  MU_BAUD = 270; // 1115200 bps

  // enable transmit and receive
  MU_CNTL = 3;
}

volatile uint64_t systime(void) {
  uint64_t t;
  uint32_t chi;
  uint32_t clo;

  chi = SYST_CHI;
  clo = SYST_CLO;
  if (chi != SYST_CHI) {
    chi = SYST_CHI;
    clo = SYST_CLO;
  }
  t = chi;
  t = t << 32;
  t += clo;
  return t;
}

void delay_ms(uint32_t duration){
  uint64_t end_time;

  end_time = systime() + duration * 1000;
  while(systime() < end_time);
  
  return;
}

void uart_putc(const unsigned char c) {
    while (!(MU_LSR & MU_LSR_TX_IDLE) && !(MU_LSR & MU_LSR_TX_EMPTY));
    MU_IO = 0xffU & c;
}

void uart_print(const char *s) {
    while(*s) {
        uart_putc(*s++);
    }
}

#define TO_HEX(c)  (((c) < 10) ? (c) + 0x30 : (c) - 10 + 0x61)

void uart_put_hex(const unsigned char c) {
    uart_putc(TO_HEX(c >> 4));
    uart_putc(TO_HEX(c & 0xfU));
}

// PWM functions

void init_pwm(pwm_t *pwm, uint32_t div) {
    // set GPIO18, 19 to ALT5
    GPFSEL1 |= (GPF_ALT_5 << (3*8)) | (GPF_ALT_5 << (3*9));
    // set up pwm clock
    CM_PWMCTL = CM_PASSWD | (CM_PWMCTL & (~CM_PWMCTL_ENAB)); // disable
    do {} while(CM_PWMCTL & CM_PWMCTL_BUSY);
    CM_PWMCTL = CM_PASSWD | CM_PWMCTL_MASH_NONE;
    CM_PWMCTL |= CM_PASSWD | CM_PWMCTL_SRC_OSC; // clock source = osc (19.2MHz)
    CM_PWMDIV = CM_PASSWD | (div << 12); // clock = 19.2MHz/div
    CM_PWMCTL |= CM_PASSWD | CM_PWMCTL_ENAB;
}

int main(int argc, char **argv) {
  pwm_t* pwm = (pwm_t*) (PWM);

  init_uart();
  uart_print("\r\nPWM test. 50Hz signal is on GPIO18.\r\n");
  delay_ms(1000);

  init_pwm(pwm, 192); // 19.2MHz/192 = 10KHz clock frequency

  // initialize pwm
  pwm->CTL = 0; // disable
  delay_ms(1);
  pwm->RNG1 = 2000;   // 10KHz / 2000 = 50Hz output frequency
  pwm->CTL = CTL_MSEN1 | CTL_PWEN1; // pwm1 = mark:space mode, enabled

  // generate pwm output for servo (pulse width = 0.5ms .. 2.4ms, 50Hz)
  while(1) {
      for(int i = 60; i < 250; i++) {
          pwm->DAT1 = i;
          delay_ms(20);
      }
      for(int i = 250; i > 60; i--) {
          pwm->DAT1 = i;
          delay_ms(20);
      }
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
