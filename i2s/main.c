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

// PCM registers

#define CM_PCMCTL  IOREG(0x20101098)
#define CM_PCMDIV  IOREG(0x2010109C)

#define CM_PASSWD           (0x5a000000)
#define CM_PCMCTL_SRC_MASK  (0xfU)
#define CM_PCMCTL_SRC_OSC   (1U)
#define CM_PCMCTL_SRC_PLLA  (4U)
#define CM_PCMCTL_SRC_PLLC  (5U)
#define CM_PCMCTL_SRC_PLLD  (6U)
#define CM_PCMCTL_SRC_HDMI  (7U)
#define CM_PCMCTL_ENAB  (1U<<4)
#define CM_PCMCTL_KILL  (1U<<5)
#define CM_PCMCTL_BUSY  (1U<<7)
#define CM_PCMCTL_BUSYD (1U<<8)
#define CM_PCMCTL_MASH_NONE  (1U<<9)
#define CM_PCMCTL_MASH_2STG  (2U<<9)
#define CM_PCMCTL_MASH_3STG  (3U<<9)

#define PCM        (0x20203000)
typedef volatile struct _pcm_t {
    uint32_t CS;
    uint32_t FIFO;
    uint32_t MODE;
    uint32_t RXC;
    uint32_t TXC;
    uint32_t DREQ;
    uint32_t INTEN;
    uint32_t INTSTC;
    uint32_t GRAY;
} pcm_t;

#define CS_STBY   (1<<25)
#define CS_SYNC   (1<<24)
#define CS_RXSEX  (1<<23)
#define CS_RXF    (1<<22)
#define CS_TXE    (1<<21)
#define CS_RXD    (1<<20)
#define CS_TXD    (1<<19)
#define CS_RXR    (1<<18)
#define CS_TXW    (1<<17)
#define CS_RXERR  (1<<16)
#define CS_TXERR  (1<<15)
#define CS_RXSYNC (1<<14)
#define CS_TXSYNC (1<<13)
#define CS_DMAEN  (1<<9)
#define CS_RXTHR  (3<<7)
#define CS_TXTHR  (3<<5)
#define CS_RXCLR  (1<<4)
#define CS_TXCLR  (1<<3)
#define CS_TXON   (1<<2)
#define CS_RXON   (1<<1)
#define CS_EN     (1)

#define CS_RXTHR_SHFT 7
#define CS_TXTHR_SHFT 5

#define MODE_CLK_DIS (1<<28)
#define MODE_PDMN  (1<<27)
#define MODE_PDME  (1<<26)
#define MODE_FRXP  (1<<25)
#define MODE_FTXP  (1<<24)
#define MODE_CLKM  (1<<23)
#define MODE_CLKI  (1<<22)
#define MODE_FSM   (1<<21)
#define MODE_FSI   (1<<20)
#define MODE_FLEN  (0x1ff<<10)
#define MODE_FSLEN (0x1ff)

#define MODE_FLEN_SHFT 10

#define CH1WEX     (1<<31)
#define CH1EN      (1<<30)
#define CH1POS     (0x1ff<<20)
#define CH1WID     (0xf<<16)
#define CH2WEX     (1<<15)
#define CH2EN      (1<<14)
#define CH2POS     (0x1ff<<4)
#define CH2WID     (0xf)

#define CH1POS_SHFT 20
#define CH1WID_SHFT 16
#define CH2POS_SHFT 4

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

// PCM functions

void init_pcm(uint32_t src, uint32_t div) {
    // set GPIO18, 19 to ALT0
    GPFSEL1 |= (GPF_ALT_0 << (3*8)) | (GPF_ALT_0 << (3*9));
    // set GPIO20, 21 to ALT0
    GPFSEL2 |= (GPF_ALT_0 << (3*0)) | (GPF_ALT_0 << (3*1));
    // set up pcm clock
    CM_PCMCTL = CM_PASSWD | (CM_PCMCTL & (~CM_PCMCTL_ENAB)); // disable
    do {} while(CM_PCMCTL & CM_PCMCTL_BUSY);
    CM_PCMCTL = CM_PASSWD | CM_PCMCTL_MASH_NONE;
    CM_PCMCTL |= CM_PASSWD | (src & CM_PCMCTL_SRC_MASK);
    CM_PCMDIV = CM_PASSWD | (div << 12);
    CM_PCMCTL |= CM_PASSWD | CM_PCMCTL_ENAB;
}

int main(int argc, char **argv) {
  pcm_t* pcm = (pcm_t*) (PCM);

  init_uart();
  uart_print("\r\nI2S test. Signal is on GPIO18(CLK)/19(FS)/21(DATA).\r\n");
  delay_ms(1000);

  init_pcm(CM_PCMCTL_SRC_OSC, 75); // 19.2MHz/75 = 256KHz clock frequency

  // initialize pcm

  // Set the EN bit to enable the PCM block. 
  pcm->CS = CS_EN;
  // Set all operational values to define the frame and channel settings. 
  pcm->MODE = MODE_FTXP | MODE_CLKI | MODE_FSI | 31<<MODE_FLEN_SHFT | 16;
  pcm->TXC = CH1EN | 1<<CH1POS_SHFT | 8<<CH1WID_SHFT | \
      CH2EN | 17<<CH2POS_SHFT | 8;
  // Assert RXCLR and/or TXCLR wait for 2 PCM clocks to ensure the FIFOs are reset. 
  pcm->CS |= CS_TXCLR;
  // The SYNC bit can be used to determine when 2 clocks have passed.
  pcm->CS |= CS_SYNC;
  do {} while ((pcm->CS & CS_SYNC) == 0);
  // Set RXTHR/TXTHR to determine the FIFO thresholds.
  pcm->CS |= 3<<CS_TXTHR_SHFT;
  // If transmitting, ensure that sufficient sample words have been written to PCMFIFO
  // before transmission is started. 
  for (int i = 0; i < 64; i++) {
      pcm->FIFO = 0;
  }
  // Set TXON and/or RXON to begin operation. 
  pcm->CS |= CS_TXERR;
  pcm->CS |= CS_TXON;

  // output audio PCM data
  uint32_t saw[16] = {0x80008000, 0x90009000, 0xa000a000, 0xb000b000, \
                      0xc000c000, 0xd000d000, 0xe000e000, 0xf000f000, \
                      0, 0x10001000, 0x20002000, 0x30003000,          \
                      0x40004000, 0x50005000, 0x60006000, 0x070007000};
  int i = 0;
  while(1) {
      // Poll TXW writing sample words to PCMFIFO and RXR reading sample words from PCMFIFO
      // until all data is transferred.
      do {} while ((pcm->CS & CS_TXW) == 0);
      pcm->FIFO = saw[i];
      i = ++i % 16;
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
