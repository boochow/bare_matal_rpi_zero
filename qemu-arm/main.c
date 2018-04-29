#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct uart_t {
    uint32_t DR;
    uint32_t RSRECR;
    uint8_t reserved1[0x10];
    const uint32_t FR;
    uint8_t reserved2[0x4];
    uint32_t ILPR;
    uint32_t IBRD;
    uint32_t FBRD;
    uint32_t LCRH;
    uint32_t CR;
    uint32_t IFLS;
    uint32_t IMSC;
    const uint32_t RIS;
    const uint32_t MIS;
    uint32_t ICR;
    uint32_t DMACR;
} uart_t;

#define IS_RX_RDY (!(uart0->FR & (1 << 4)))
#define RX_CH     (uart0->DR)
#define IS_TX_RDY (!(uart0->FR & (1 << 5)))
#define TX_CH(c)  (uart0->DR = (c))

/// QEMU UART register address
volatile struct uart_t *uart0;

void uart0_init() {
    uart0 = (volatile struct uart_t *) 0x101f1000;
    
    uart0->CR = 0;
    // set spped to 115200 bps
    uart0->IBRD = 1;
    uart0->FBRD = 40;
    // parity none 8bits FIFO enable
    uart0->LCRH = 0x70;

    uart0->CR = 0x0301;
};

void uart0_putc(char c) {
    while(!IS_TX_RDY) {
    }
    TX_CH(c);
}

uint32_t uart0_getc(void) {
    uint32_t c;
  
    while (!IS_RX_RDY) {
    }
    c = RX_CH;
    return c & 0xffU;
}

void uart_print(const char *s) {
    while(*s) {
        uart0_putc(*s++);
    }
}

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


int main(int argc, char **argv) {
  const char msg[] = "-- Echo back test --\012\015\000";

  uart0_init();
  uart_print(msg);

  // echo back
  for(;;) {
    uint32_t c;
    c = uart0_getc();
    uart0_putc(c);
    if (c == 0x0D)
        uart0_putc(0X0A);
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
