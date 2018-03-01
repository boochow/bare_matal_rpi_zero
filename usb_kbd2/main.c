#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "emb-stdio.h"
#include "rpi-SmartStart.h"
#include "rpi-USB.h"

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

void uart_putchar(unsigned char c) {
    while (!(MU_LSR & MU_LSR_TX_IDLE) && !(MU_LSR & MU_LSR_TX_EMPTY));
    MU_IO = 0xffU & c;
}

void uart_puthex(unsigned char c) {
    static const char hex[16] = "0123456789ABCDEF";
    uart_putchar(hex[c >> 4]);
    uart_putchar(hex[c & 0x0fU]);
}

int uart_printf (const char *fmt, ...)
{
	char printf_buf[512];
	va_list args;
	int printed = 0;

	va_start(args, fmt);
	printed = emb_vsprintf(printf_buf, fmt, args);
	va_end(args);
	for (int i = 0; i < printed; i++){
		uart_putchar(printf_buf[i]);
        if (printf_buf[i] == 0x0A) {
            uart_putchar(0x0D);
        }
    }

	return printed;
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
    timer_wait(100000);
  }

  MU_IIR = 0xC6; // enable FIFO(0xC0), clear FIFO(0x06)

  // initialize usb
  UsbInitialise(uart_printf, NULL); // arg: console and debug msg handler

  uint8_t firstKbd = 0;
  uint8_t data[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t key = 0;

  while (1) {

      if (firstKbd == 0) {
          UsbCheckForChange();
          timer_wait(100000);
          for (int i = 1; i <= MaximumDevices; i++) {
              if (IsKeyboard(i)) {
                  firstKbd = i;
                  break;
              }
          }
      } else if (HIDReadReport(firstKbd, 0, (uint16_t) USB_HID_REPORT_TYPE_INPUT << 8 | 0, &data[0], 8) == OK) {
          if ((key != data[2]) && (data[2] != 0)) {
              uart_puthex(data[2]);
              uart_putchar(' ');
          }
          key = data[2];
      } else {
          firstKbd = 0;
      }

}

  return 0;
}

