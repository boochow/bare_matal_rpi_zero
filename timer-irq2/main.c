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

extern uint32_t __bss_start, __bss_end;

__attribute__((naked)) __attribute__((section(".startup"))) \
void Init_Machine(void) {
  // set CPSR (PSR_IRQ_MODE|PSR_FIQ_DIS|PSR_IRQ_DIS)
  __asm volatile("ldr r0, =0x000000d2 \n"
                 "msr cpsr_c, r0 \n");
  // set stack pointer
  __asm volatile("ldr sp, =0x8000");

  // set CPSR (PSR_FIQ_MODE|PSR_FIQ_DIS|PSR_IRQ_DIS)
  __asm volatile("ldr r0, =0x000000d1 \n"
                 "msr cpsr_c, r0 \n");
  // set stack pointer
  __asm volatile("ldr sp, =0x4000");

  // set CPSR (PSR_SVC_MODE|PSR_FIQ_DIS|PSR_IRQ_DIS)
  __asm volatile("ldr r0, =0x000000d3 \n"
                 "msr cpsr_c, r0 \n");
  // set stack pointer
  __asm volatile("ldr sp, =0x06400000");

  __asm volatile("bl main");
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

void uart_print(const char *message) {
    while(*message != 0) {
        uart_putchar(*message++);
    }
}


// ldr pc, [pc, #24]
#define	JMP_PC_24	0xe59ff018

typedef void (*exception_hander_t)(void);

typedef struct __attribute__((aligned(32))) _vector_table_t {
    const unsigned int vector[8]; // all elements shoud be JMP_PC_24
    exception_hander_t reset;
    exception_hander_t undef;
    exception_hander_t svc;
    exception_hander_t prefetch_abort;
    exception_hander_t data_abort;
    exception_hander_t hypervisor_trap;
    exception_hander_t irq;
    exception_hander_t fiq;
} vector_table_t;


static void __attribute__((interrupt("UNDEF"))) undef_handler(void);
static void __attribute__((interrupt("SWI"))) svc_handler(void);
static void __attribute__((interrupt("ABORT"))) abort_handler(void);
static void __attribute__((interrupt("IRQ"))) irq_handler(void);
static void __attribute__((interrupt("FIQ"))) fiq_handler(void);
static void __attribute__((naked)) hangup(void);

static vector_table_t exception_vector = { \
    .vector = { JMP_PC_24, JMP_PC_24, JMP_PC_24, JMP_PC_24, \
                JMP_PC_24, JMP_PC_24, JMP_PC_24, JMP_PC_24 },
    .reset = Init_Machine,
    .undef = undef_handler,
    .svc = svc_handler,
    .prefetch_abort = abort_handler,
    .data_abort = abort_handler,
    .hypervisor_trap = hangup,
    .irq = irq_handler,
    .fiq = hangup
};

void set_vbar(vector_table_t *base) {
    asm volatile ("mcr p15, 0, %[base], c12, c0, 0"
                  :: [base] "r" (base));
}


#define IRQ_BASIC         IOREG(0x2000B200)
#define IRQ_PEND1         IOREG(0x2000B204)
#define IRQ_PEND2         IOREG(0x2000B208)
#define IRQ_FIQ_CONTROL   IOREG(0x2000B20C)
#define IRQ_ENABLE1       IOREG(0x2000B210)
#define IRQ_ENABLE2       IOREG(0x2000B214)
#define IRQ_ENABLE_BASIC  IOREG(0x2000B218)
#define IRQ_DISABLE1      IOREG(0x2000B21C)
#define IRQ_DISABLE2      IOREG(0x2000B220)
#define IRQ_DISABLE_BASIC IOREG(0x2000B224)


#define SYS_TIMER_CS  IOREG(0x20003000)
#define SYS_TIMER_CLO IOREG(0x20003004)
#define SYS_TIMER_CHI IOREG(0x20003008)
#define SYS_TIMER_C0  IOREG(0x2000300C)
#define SYS_TIMER_C1  IOREG(0x20003010)
#define SYS_TIMER_C2  IOREG(0x20003014)
#define SYS_TIMER_C3  IOREG(0x20003018)

#define IRQ_TIMER_C1  (1 << 1)
#define IRQ_TIMER_C3  (1 << 3)
    
static volatile int counter1;
static volatile int counter3;
static volatile int changed1;
static volatile int changed3;

static void __attribute__((interrupt("UNDEF"))) undef_handler(void) {
}

static void __attribute__((interrupt("SWI"))) svc_handler(void) {
}

static void __attribute__((interrupt("ABORT"))) abort_handler(void) {
}

static void __attribute__((interrupt("IRQ"))) irq_handler(void) {
    if (IRQ_PEND1 & IRQ_TIMER_C1) {
        counter1++;
        changed1 = 1;
        SYS_TIMER_C1 = SYS_TIMER_C1 + 960000;
        SYS_TIMER_CS |= (1 << 1);
    }
    if (IRQ_PEND1 & IRQ_TIMER_C3) {
        counter3++;
        changed3 = 1;
        SYS_TIMER_C3 = SYS_TIMER_C3 + 720000;
        SYS_TIMER_CS |= (1 << 3);
    }
}

//static void __attribute__((interrupt("FIQ"))) fiq_handler(void) {
static void __attribute__((interrupt("IRQ"))) fiq_handler(void) {
}

static void __attribute__((naked)) hangup(void) {
  while(1) {
  }
}

static void print_4digit(int c) {
  uart_putchar(0x30 + c / 1000);
  c = c % 1000;
  uart_putchar(0x30 + c / 100);
  c = c % 100;
  uart_putchar(0x30 + c / 10);
  c = c % 10;
  uart_putchar(0x30 + c);
  uart_putchar(0x0A);
  uart_putchar(0x0D);
}

int main(int argc, char **argv) {
  const char msg[] = "Interrupt handler test.\012\015\000";

  // zero out .bss section
  for (uint32_t *dest = &__bss_start; dest < &__bss_end;) {
    *dest++ = 0;
  }

  // disable IRQ
  IRQ_DISABLE_BASIC = 1;
  
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
  for(int i = 0; msg[i]; i++) {
    uart_putchar(msg[i]);
    delay_ms(100);
  }

  MU_IIR = 0xC6; // enable FIFO(0xC0), clear FIFO(0x06)

  // set timer
  int now = SYS_TIMER_CLO;
  SYS_TIMER_C1 = now + 1000000;
  SYS_TIMER_C3 = now + 1000100;
  
  // enable IRQ
  set_vbar(&exception_vector);
  IRQ_ENABLE1 = (IRQ_TIMER_C1 | IRQ_TIMER_C3);
  __asm volatile("mrs r0, cpsr \n"
                 "bic r0, r0, #0x80 \n"
                 "msr cpsr_c, r0 \n");

  counter1 = 0;
  changed1 = 0;
  counter3 = 0;
  changed3 = 0;

  uart_print("TIM1 TIM3\012\015");
  while (1) {
    const char msg2[] = "     \000";
    if (changed1 != 0) {
      print_4digit(counter1);
      changed1 = 0;
    }
    if (changed3 != 0) {
      uart_print(msg2);
      print_4digit(counter3);
      changed3 = 0;
    }
  }

  return 0;
}

