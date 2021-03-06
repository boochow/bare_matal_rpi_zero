#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "usbd/usbd.h"
#include "device/hid/keyboard.h"

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

void uart_putchar(unsigned char c) {
    while (!(MU_LSR & MU_LSR_TX_IDLE) && !(MU_LSR & MU_LSR_TX_EMPTY));
    MU_IO = 0xffU & c;
}

void uart_puthex(unsigned char c) {
    static const char hex[16] = "0123456789ABCDEF";
    uart_putchar(hex[c >> 4]);
    uart_putchar(hex[c & 0x0fU]);
}

void LogPrint(char* message, unsigned int messageLength) {
    for (int i = 0; i < messageLength; i++) {
        uart_putchar(*message);
        if (*message++ == 0x0A) {
            uart_putchar(0x0D);
        }
    }
}

void println(char *message) {
    while(*message != 0) {
        uart_putchar(*message++);
    }
    uart_putchar(0x0A);
    uart_putchar(0x0D);
}

int main(int argc, char **argv) {
  const char msg[] = "USB Key code read test\n\r";
  const int msglen = 24;
  
  // set GPIO14, GPIO15 to pull down, alternate function 0
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


  // type message
  for(int i = 0; i < msglen; i++) {
    uart_putchar(msg[i]);
    delay_ms(100);
  }

  MU_IIR = 0xC6; // enable FIFO(0xC0), clear FIFO(0x06)

  // initialize usb
  UsbInitialise();

  while (1) {
      unsigned int kbd_addr;
      unsigned int keys[6];
      unsigned int key;

      if (kbd_addr == 0) {
          // Is there a keyboard ?
          while(KeyboardCount() == 0) {
              UsbCheckForChange();
              delay_ms(100);
          }
          println("Found: keyboard");
          kbd_addr = KeyboardGetAddress(0);
      }

      if (kbd_addr != 0) {
          for(int i = 0; i < 6; i++) {
              // Read and print each keycode of pressed keys
              key = KeyboardGetKeyDown(kbd_addr, i);
              if (key != keys[0] && key != keys[1] && key != keys[2] && \
                  key != keys[3] && key != keys[4] && key != keys[5] && key) {
                  uart_puthex(key);
                  uart_putchar(' ');
              }
              keys[i] = key;
          }

          if (KeyboardPoll(kbd_addr) != 0) {
              kbd_addr = 0;
              println("Lost: keyboard");
          }
      }
      delay_ms(5);
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
