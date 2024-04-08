#ifndef PAGESWAP_H
#define PAGESWAP_H

#include "proc.h"
#include "types.h"
#include "fs.h"
#include "mmu.h"
#include "memlayout.h"
#include "param.h"
#include "defs.h"
#include "x86.h"

#define NSWAPSLOTS 100

struct swap_slot_m swap_slots_buffer[NSWAPSLOTS];

void* swap_out();
void pagefault_handler();

struct swap_slot_m* arrange_swap_slot(uint *index) ;
uint free_swap_slot(uint index);
void init_swap_slots();

#endif