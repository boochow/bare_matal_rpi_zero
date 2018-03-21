#include <stdint.h>
#include <stdio.h>

#define IOREG(X)  (*(volatile uint32_t *) (X))

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

typedef volatile struct                                          \
__attribute__((aligned(16))) _fb_info_t {
    uint32_t display_w;  //write display width
    uint32_t display_h;  //write display height
    uint32_t w;          //write framebuffer width
    uint32_t h;          //write framebuffer height
    uint32_t row_bytes;  //write 0 to get value
    uint32_t bpp;        //write bits per pixel
    uint32_t ofs_x;      //write x offset of framebuffer
    uint32_t ofs_y;      //write y offset of framebuffer
    uint32_t buf_addr;   //write 0 to get value
    uint32_t buf_size;   //write 0 to get value
} fb_info_t;

void fb_init(fb_info_t *fb_info) {
    fb_info->buf_addr = 0;
    fb_info->buf_size = 0;
    fb_info->row_bytes = 0;
    while(fb_info->buf_addr == 0) {
        mailbox_write(1, (uint32_t) (fb_info + 0x40000000));
        mailbox_read(1);
    }
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
