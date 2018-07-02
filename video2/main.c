#include <stdint.h>
#include <stdio.h>

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

void print_int(unsigned int i) {
    uart_put_hex(i >> 24);
    uart_put_hex((i >> 16) & 0xff);
    uart_put_hex((i >> 8) & 0xff);
    uart_put_hex(i & 0xff);
}

#define MAILBOX_READ   IOREG(0x2000B880)
#define MAILBOX_POLL   IOREG(0x2000B890)
#define MAILBOX_SENDER IOREG(0x2000B894)
#define MAILBOX_STATUS IOREG(0x2000B898)
#define MAILBOX_CONFIG IOREG(0x2000B89C)
#define MAILBOX_WRITE  IOREG(0x2000B8A0)

#define MAIL_FULL      0x80000000
#define MAIL_EMPTY     0x40000000

void mailbox_write(uint8_t chan, uint32_t msg) {
    if ((msg & 0xfU) == 0) {
        while ((MAILBOX_STATUS & MAIL_FULL) != 0) {
        }
        MAILBOX_WRITE = msg | chan;
    }
}

uint32_t mailbox_read(uint8_t chan) {
    uint32_t data;
    do {
        while (MAILBOX_STATUS & MAIL_EMPTY) {
        }
    } while (((data = MAILBOX_READ) & 0xfU) != chan);
    return data >> 4;
}

typedef struct _fb_info_t {
    uint32_t display_w;  // display width
    uint32_t display_h;  // display height
    uint32_t w;          // framebuffer width
    uint32_t h;          // framebuffer height
    uint32_t row_bytes;  // write 0 to get value
    uint32_t bpp;        // bits per pixel
    uint32_t ofs_x;      // x offset of framebuffer
    uint32_t ofs_y;      // y offset of framebuffer
    uint32_t buf_addr;   // pointer to framebuffer
    uint32_t buf_size;   // framebuffer size in bytes
} fb_info_t;

void fb_init(fb_info_t *fb_info) {
    unsigned int message[] = {
        112,                        // buffer is 112 bytes
        0,                          // This is a request
        0x00048003, 8, 0,           // Set the screen size to..
        fb_info->display_w,         // @5
        fb_info->display_h,         // @6
        0x00048004, 8, 0,           // Set the virtual screen size to..
        fb_info->w,                 // @10
        fb_info->h,                 // @11
        0x00048005, 4, 0,           // Set the depth to..
        fb_info->bpp,               // @15
        0x00040008, 4, 0,           // Get the pitch
        0,                          // @19
        0x00040001, 8, 0,           // Get the frame buffer address..
        16, 0,                      // @23 a 16 byte aligned
        0,                          // @25 the end tag
        0, 0,                       // padding; 16 byte aligned
    };
    uart_print("\r\nsending mailbox.\r\n");
    for (int i = 0; i < 28; i++) {
        print_int(message[i]);
        if (i % 8 == 7) {
            uart_print("\n\r");
        } else {
            uart_print(" ");
        }
    }
    uart_print("\n\r");

    mailbox_write(8, (uint32_t) message + 0x40000000);
    uart_print("done.\n\r");

    uart_print("\r\nreading mailbox.\r\n");
    mailbox_read(8);

    for (int i = 0; i < 28; i++) {
        print_int(message[i]);
        if (i % 8 == 7) {
            uart_print("\n\r");
        } else {
            uart_print(" ");
        }
    }
    uart_print("\n\r");
    uart_print("done.\n\r");

    fb_info->display_w = message[5];
    fb_info->display_h = message[6];
    fb_info->w = message[10];
    fb_info->h = message[11];
    fb_info->row_bytes = message[19];
    fb_info->bpp = message[15];
    fb_info->buf_addr = message[23] & 0x3ffffffff;
    fb_info->buf_size = message[24];
}

static fb_info_t fb_info = {1920, 1080, 480, 270, 0, 16, 0, 0, 0, 0};

static inline void *coord2ptr(int x, int y) {
    return (void *) (fb_info.buf_addr                   \
                     + (fb_info.bpp + 7 >> 3) * x       \
                     + fb_info.row_bytes * y);
}

void hline16(int x, int y, int l, uint32_t c) {
    uint16_t *p = (uint16_t *) coord2ptr(x, y);
    if (fb_info.w < l + x) {
        l = fb_info.w - x;
    }
    for(int i = 0; i < l; i++) {
        *p++ = c;
    }
}

void vline16(int x, int y, int l, uint32_t c) {
    uint16_t *p = (uint16_t *) coord2ptr(x, y);
    if (fb_info.h < l + y) {
        l = fb_info.h - y;
    }
    for(int i = 0; i < l; i++) {
        *p = c;
        p += fb_info.row_bytes >> 1;
    }
}

int main(int argc, char **argv) {
    int i;
    
    init_uart();
    uart_print("\r\nframe buffer test.\r\n");

    fb_init(&fb_info);
    for(i = 0; i < fb_info.h; i += 8) {
        hline16(0, i, fb_info.w, 0x5050 * i);
    }
    for(i = 0; i < fb_info.w; i += 8) {
        vline16(i, 0, fb_info.h, 0xa0a0 * i);
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
