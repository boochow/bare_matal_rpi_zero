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

// SPI registers

#define SPI0 (0x20204000)

typedef volatile struct _spi_t {
    uint32_t CS;
    uint32_t FIFO;
    uint32_t CLK;
    uint32_t DLEN;
    uint32_t LTOH;
    uint32_t DC;
} spi_t;

#define CS_LEN_LONG (1<<25)
#define CS_DMA_LEN  (1<<24)
#define CS_CSPOL2   (1<<23)
#define CS_CSPOL1   (1<<22)
#define CS_CSPOL0   (1<<21)
#define CS_RXF      (1<<20)
#define CS_RXR      (1<<19)
#define CS_TXD      (1<<18)
#define CS_RXD      (1<<17)
#define CS_DONE     (1<<16)
#define CS_TE_EN    (1<<15)
#define CS_LMONO    (1<<14)
#define CS_LEN      (1<<13)
#define CS_REN      (1<<12)
#define CS_ADCS     (1<<11)
#define CS_INTR     (1<<10)
#define CS_INTD     (1<<9)
#define CS_DMAEN    (1<<8)
#define CS_TA       (1<<7)
#define CS_CSPOL    (1<<6)
#define CS_CLEAR    (3<<4)
#define CS_CPOL     (1<<3)
#define CS_CPHA     (1<<2)
#define CS_CS       (3<<0)

#define CLEAR_TX    (1<<4)
#define CLEAR_RX    (2<<4)

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

// SPI functions

void spi_init(spi_t *spi, int polarity, int phase) {
    // set GPIO2, GPIO3 to alternate function 0
    GPFSEL0 |= (GPF_ALT_0 << (3*7)) | (GPF_ALT_0 << (3*8)); // GPIO7,8
    GPFSEL0 |= (GPF_ALT_0 << (3*9)) ; // GPIO9
    GPFSEL1 |= (GPF_ALT_0 << (3*0)) | (GPF_ALT_0 << (3*1)); // GPIO10,11
    uint32_t reg = spi->CS & ~(CS_CS | CS_CPOL | CS_CPHA | CS_REN);
    if (polarity != 0) {
        reg |= CS_CPOL;
    }
    if (phase != 0) {
        reg |= CS_CPHA;
    }
    spi->CS = reg | CLEAR_TX | CLEAR_RX;
    spi->CLK = 833; // 250MHz/833 = 300KHz
}

void spi_chip_select(spi_t *spi, int cs) {
    spi->CS = (spi->CS & ~(CS_CS)) | (cs & 0x3U);
}

int spi_write_read(spi_t *spi, uint8_t *buf_tx, const uint32_t wlen, uint8_t *buf_rx, const uint32_t rlen) {
    int result;
    int txlen = wlen;
    int rxlen = rlen;

    spi->CS = spi->CS | CS_TA;

    for(;;) {
        if (txlen > 0) {
            if (spi->CS & CS_TXD) {
                spi->FIFO = *buf_tx++;
                txlen--;
            }
        }

        if (rxlen > 0) {
            if (spi->CS & CS_RXD) {
                *buf_rx++ = spi->FIFO;
                rxlen--;
            }
        }

        if (spi->CS & CS_DONE) {
            if (wlen == 0) {
                result = rlen - rxlen;
            } else {
                result = wlen - txlen;
            }
            break;
        }

    }
    spi->CS = spi->CS & ~CS_TA;
    return result;
}

int main(int argc, char **argv) {
  spi_t* spi = (spi_t*) (SPI0);

  init_uart();
  uart_print("\r\nSPI echo back test.\r\n");
  delay_ms(1000);

  spi_init(spi, 0, 0);
  spi_chip_select(spi, 0);

  char send[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  char recv[37];
  char *p = send;
  int ret;
  int i;

  while(*p) {
      if (ret = spi_write_read(spi, p, 1, NULL, 0) > 0) {
          uart_print("\r\nsend ");
          uart_putc(*p);
          p++;
          if (ret = spi_write_read(spi, NULL, 0, recv, 1) > 0) {
              uart_print(" recv ");
              uart_putc(recv[0]);
          }
      }
  }

  ret = spi_write_read(spi, send, 37, recv, 37);
  uart_print("\r\nsend ");
  uart_print(send);
  uart_print("\r\nrecv ");
  uart_print(recv);
  uart_print("\r\n");

  while(1);
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
