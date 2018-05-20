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

// I2C registers

#define BSC1 (0x20804000)

typedef volatile struct _i2c_t {
    uint32_t C;
    uint32_t S;
    uint32_t DLEN;
    uint32_t A;
    uint32_t FIFO;
    uint32_t DIV;
    uint32_t DEL;
    uint32_t CLKT;
} i2c_t;

#define C_I2CEN (1<<15)
#define C_INTR  (1<<10)
#define C_INTT  (1<<9)
#define C_INTD  (1<<8)
#define C_ST    (1<<7)
#define C_CLEAR (3<<4)
#define C_READ  (1)

#define S_CLKT  (1<<9)
#define S_ERR   (1<<8)
#define S_RXF   (1<<7)
#define S_TXE   (1<<6)
#define S_RXD   (1<<5)
#define S_TXD   (1<<4)
#define S_RXR   (1<<3)
#define S_TXW   (1<<2)
#define S_DONE  (1<<1)
#define S_TA    (1)

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

// I2C functions

void i2c_init(i2c_t *i2c) {
    // set GPIO2, GPIO3 to alternate function 0
    GPFSEL0 = (GPF_ALT_0 << (3*2)) | (GPF_ALT_0 << (3*3));
    i2c->S |= S_CLKT | S_ERR | S_DONE;
    i2c->C |= C_I2CEN | C_CLEAR;
}

#define i2c_set_slave(i2c, addr)    ((i2c)->A = (addr) & 0x7FU)
#define i2c_get_slave(i2c)          ((i2c)->A & 0x7FU)

int i2c_write(i2c_t *i2c, const uint8_t *buf, const uint32_t buflen) {
    i2c->DLEN = buflen;
    i2c->S |= S_DONE | S_ERR | S_CLKT;
    i2c->C = i2c->C & ~C_READ;
    i2c->C |= C_CLEAR;
    i2c->C |= C_ST;
    int len = buflen;
    for(;;) {
        if (i2c->S & S_TA) {
            continue;
        } else if (i2c->S & S_ERR) {
            // No Ack Error
            i2c->S |= S_ERR;
            return -1;
        } else if (i2c->S & S_CLKT) {
            // Timeout Error
            i2c->S |= S_CLKT;
            return -2;
        } else if (i2c->S & S_DONE) {
            // Transfer Done
            i2c->S |= S_DONE;
            return buflen - len;
        }

        if ((i2c->S & S_TXD) && (len != 0)){
            // FIFO has a room
            i2c->FIFO = *buf++;
            len--;
        }
    }
}

int i2c_read(i2c_t *i2c, uint8_t *buf, const uint32_t readlen) {
    int len = readlen;
    i2c->DLEN = readlen;
    i2c->S |= S_DONE | S_ERR | S_CLKT;
    i2c->C |= C_READ;
    i2c->C |= C_CLEAR;
    i2c->C |= C_ST;
    for(;;) {
        if (i2c->S & S_TA) {
            continue;
        } else if (i2c->S & S_ERR) {
            // No Ack Error
            i2c->S |= S_ERR;
            return -1;
        } else if (i2c->S & S_CLKT) {
            // Timeout Error
            i2c->S |= S_CLKT;
            return -2;
        } else if (i2c->S & S_DONE) {
            // Transfer Done
            i2c->S |= S_DONE;
            return readlen - len;
        }

        if (i2c->S & S_RXD) {
            uint8_t data = i2c->FIFO;
            if (len != 0) {
                *buf++ = data;
                len--;
            } else {
                return readlen - len;
            }
        }
    }
}

void i2c_flush(i2c_t *i2c) {
    i2c->C |= C_CLEAR;
}

void i2c_set_clock_speed(i2c_t *i2c, uint32_t speed) {
    uint32_t cdiv = 250000000 / speed;
    cdiv = (cdiv < 2) ? 2 : cdiv;
    cdiv = (cdiv > 0xfffe) ? 0xfffe : cdiv;
    uint32_t fedl = (cdiv > 15) ? (cdiv / 16) : 1;
    uint32_t redl = (cdiv > 3) ? (cdiv / 4) : 1;
    i2c->DIV = cdiv;
    i2c->DEL = (fedl << 16) | redl;
}

uint32_t i2c_get_clock_speed(i2c_t *i2c) {
    return 250000000 / i2c->DIV;
}


int main(int argc, char **argv) {
  i2c_t* i2c = (i2c_t*) (BSC1);

  init_uart();
  uart_print("\r\nScanning I2C bus...\r\n");

  i2c_init(i2c);

  int start = 0x08;
  int end = 0x77;
  uart_print("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
  for (uint32_t addr = 0; addr <= 0x7f; addr++) {
      if ((addr % 16) == 0) {
          uart_put_hex(addr);
          uart_putc(':');
      }

      if ((addr >= start) && (addr <= end)) {
          i2c_flush(i2c);
          i2c_set_slave(i2c, addr);

          int buf = 0;
          int ret;
          if (((0x30 <= addr) && (addr <= 0x37))
              || ((0x50 <= addr) && (addr <= 0x5f))) {
              ret = i2c_read(i2c, &buf, 1);
          } else {
              ret = i2c_write(i2c, &buf, 0);
          }

          if (ret >= 0) {
              uart_putc(' ');
              uart_put_hex(i2c_get_slave(i2c));
          } else {
              uart_print(" --");
          }
      } else {
          // I2C reserved addresses
          uart_print("   ");
      }

      if ((addr % 16) == 15) {
          uart_print("\r\n");
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
