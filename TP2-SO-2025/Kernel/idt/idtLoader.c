// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <idtLoader.h>
#include <interrupts.h> // Para las declaraciones de los handlers
#include <stdint.h>     // Necesario para uint64_t, etc.

#pragma pack(push) // save current alignment values into the compilers stack
#pragma pack(1)    // set alignment

// https://wiki.osdev.org/Interrupt_Descriptor_Table - InterruptDescriptor64
typedef struct {
  uint16_t offset_l; // offset bits 0..15
  uint16_t selector; // a code segment selector in GDT or LDT
  uint8_t
      zero; // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
  uint8_t access;      // gate type, dpl, and p fields
  uint16_t offset_m;   // offset bits 16..31
  uint32_t offset_h;   // offset bits 32..63
  uint32_t other_zero; // reserved
} DESCR_INT;


typedef struct {
  uint16_t limit;
  uint64_t base;
} IDTR;

static DESCR_INT idt[256];

static IDTR idtr;

static inline uint16_t get_cs(void) {
  uint16_t cs;
  __asm__ volatile("mov %%cs, %0" : "=r"(cs));
  return cs;
}

#pragma pack(pop) 

static void setup_IDT_entry(int index, uint64_t offset, uint16_t cs_selector);

void load_idt() {
  _cli();

  idtr.limit = sizeof(idt) - 1;
  idtr.base = (uint64_t)&idt;

  uint16_t kernel_cs = get_cs();

  setup_IDT_entry(0x00, (uint64_t)&_exceptionHandler00, kernel_cs);
  setup_IDT_entry(0x06, (uint64_t)&_exceptionHandler06, kernel_cs);
  setup_IDT_entry(0x08, (uint64_t)&_exceptionHandler08,
                  kernel_cs); 
  setup_IDT_entry(0x0D, (uint64_t)&_exceptionHandler0D,
                  kernel_cs); 
  setup_IDT_entry(0x0E, (uint64_t)&_exceptionHandler0E,
                  kernel_cs);

  // Load ISRs
  // https://wiki.osdev.org/Interrupts#General_IBM-PC_Compatible_Interrupt_Information
  setup_IDT_entry(0x20, (uint64_t)&_irq00Handler, kernel_cs);
  setup_IDT_entry(0x21, (uint64_t)&_irq01Handler, kernel_cs);
  setup_IDT_entry(0x80, (uint64_t)&_irq80Handler, kernel_cs);

  // Enable:
  // IRQ0 -> TimerTick
  // IRQ1 -> Keyboard
  picMasterMask(KEYBOARD_PIC_MASTER & TIMER_PIC_MASTER);
  picSlaveMask(NO_INTERRUPTS);

  extern void _load_idt_asm(IDTR * idtr);
  _load_idt_asm(&idtr);

  _sti();
}

static void setup_IDT_entry(int index, uint64_t offset, uint16_t cs_selector) {
  idt[index].offset_l = offset & 0xFFFF;
  idt[index].selector = cs_selector;
  idt[index].zero = 0;
  idt[index].access = ACS_INT;
  idt[index].offset_m = (offset >> 16) & 0xFFFF;
  idt[index].offset_h = (offset >> 32) & 0xFFFFFFFF;
  idt[index].other_zero = 0;
}
