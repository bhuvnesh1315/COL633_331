<<<<<<< HEAD
#include "pageswap.h"

// extract page directory entry
#define PDE_EXTRACT(va) ((uint)(va) & ~0x3FFFFF)
#define SWAP_START 2

void swap_write(pte_t *va, uint slot_no) {
    uint swap_start = 2;
    for(uint i = 0; i < 8; i++) {
        uint blockno = swap_start + slot_no * 8 + i;
        struct buf* buffer = bread(ROOTDEV, blockno); 
        memmove(buffer, va, BSIZE);
        bwrite(buffer);
        brelse(buffer);
        va += BSIZE;
    }
}

void *
swap_out()
{
    // find victim process

    struct proc *p = find_victim_process();
    // find victim page
    // uint pt_index;
    // pt_index = find_victim_page(p);
    // uint pg_va;
    // pg_va = PDE_EXTRACT(p->pgdir); // extract starting 10 bits of page's VA
    // pg_va |= pt_index; // extract next 10 bits of page's VA 

    // find victim page
    uint swap_slot_index;
    pte_t pg_va = find_victim_page(p);
    // uint pa = PTE_ADDR(*pte);
    pte_t *pte = walkpgdir(p->pgdir, (void *) pg_va, 0); 

    // swap
    struct swap_slot_m* free_slot = arrange_swap_slot(&swap_slot_index);
    free_slot->is_free = 0;
    free_slot->page_perm = PTE_FLAGS(*pte);

    swap_write(&pg_va, swap_slot_index); 
    swap_slot_index = swap_slot_index << 12;
    
    // update PTE, mark process not present
    *pte ^= PTE_P;
    *pte &= 0xFFF;
    *pte |= swap_slot_index; 

    //update rss value of process
    p->rss -= BSIZE;

    //return physical address
    return (void *)(*pte & ~0xFFF);
}

void pagefault_handler() 
{
    uint pg_va;
    pte_t *pte;
    // uint cnt=0;
    uint swap_slot_base, blockno, swap_slot_index;
    struct  proc *p;
    // char* pg;
    char* new_page;
    uint page_perm;

    pg_va= rcr2();                                      // VA of page with page fault
    p = mycpu()->proc;                                  // current process
    pte = walkpgdir(p->pgdir, (void *) pg_va, 0);              // pte of page with page fault
    swap_slot_index = PTE_ADDR(*pte);
    swap_slot_base = swap_slot_index + SWAP_START;      // address of swap slot
    new_page= kalloc();                                 // allocate new page in physical memory
    
    for(int i = 0; i < 8; ++i) 
    {
        blockno = swap_slot_base + i;                   // disk block in a swap slot
        struct buf* buffer = bread(ROOTDEV, blockno);   //read into buffer cache
        memmove(new_page, buffer, BSIZE);
        brelse(buffer);
        new_page += BSIZE;
    }

    //inc rss
    p->rss += BSIZE;

    // update PTE of process (also restore PTE_P flag)
    page_perm = free_swap_slot(swap_slot_index);
    *pte &= ~0xFFF;                                     // setting  12 LSBs zero  
    *pte = *pte | page_perm;                            // restore permissions
    
}


struct swap_slot_m* arrange_swap_slot(uint *index) 
{
  uint i;
  for(i = 0; i < NSWAPSLOTS; i++) 
  {
    if(swap_slots_buffer[i].is_free)
      break;
  }
  *index = i;
  return &swap_slots_buffer[i]; // watch out
}

//return page permission and free swap slot
uint free_swap_slot(uint index)
{
  uint perm = swap_slots_buffer[index].page_perm;
  swap_slots_buffer[index].is_free = 1;

  return perm;
}

void init_swap_slots()
{

  for(uint i=0;i<NSWAPSLOTS;i++)
  {
    swap_slots_buffer[i].is_free=1;
    // cprintf("\ni= %d", swap_slots_buffer[i].is_free);
  }
=======
#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "buf.h"
#include "file.h"
#include "swap.h"
#include "x86.h"

struct swapinfo
{
    int page_perm;
    int is_free;
};

struct swapinfo swp[20];

  // buffer 

void swapInit(void) {
    for (int i = 0; i < 20; i++) {
        swp[i].page_perm = 0;
        swp[i].is_free = 1;
    }
}

void swapOut()
{
    struct superblock sb;
    readsb(ROOTDEV, &sb);
    // find victim page
    struct proc *p = find_victim_process();
    uint victim_page_VA = find_victim_page(p);
    pte_t *victim_pte = walkpgdir(p->pgdir, (void*)victim_page_VA, 0);

    int i;
    // find free swap slot
    for (i = 0; i < 20; ++i)
    {
        if (swp[i].is_free) {
            break;
        }
    }
    cprintf("[SWAPOUT] Current slot : %d\n", i);
    swp[i].page_perm = 0;
    swp[i].is_free = 0;


    // update swap slot permissions to match victim page permissions 
    swp[i].page_perm = PTE_FLAGS(*victim_pte);

    // cprintf("Victim page addr : %p\n", PTE_ADDR(*victim_page_VA));
    *victim_pte &= ~PTE_P;
    char *pa = (char*)P2V(PTE_ADDR(*victim_pte));
    uint addressOffset;
    for (int j = 0; j < 8; ++j)
    {
        // cprintf("Victim page copied Block no : %d\n", sb.swapstart + (8 * i) + j);
        addressOffset = PTE_ADDR(*victim_pte) + (j * BSIZE);
        struct buf *b = bread(ROOTDEV, sb.swapstart + (8*i) + j);
        memmove(b->data, (void *)P2V(addressOffset), BSIZE);
        bwrite(b);
        brelse(b);
    }
    cprintf("[SWAPOUT] Swap slot used : %d\n", sb.swapstart + 8 * i);
    *victim_pte = ((sb.swapstart + (8 * i)) << 12) & (~0xFFF);
    // lcr3(V2P(myproc()->pgdir));
    cprintf("[SWAPOUT] Swap slot block no : %d\n", *victim_pte >> 12);
    cprintf("[SWAPOUT] VA : %p\n", pa);
    kfree(pa);
    cprintf("[SWAPOUT] Page freed by swapOut\n");
    // return (char *)victim_page_VA;
}

void swapIn(char *memory)
{
    struct proc *p = myproc();
    uint addr = rcr2();
    cprintf("[SWAPIN] Process is requesting page : %p, pid %d\n", V2P(addr), p->pid);
    pde_t *pd = p->pgdir;
    pte_t *pg = walkpgdir(pd, (void *)(addr), 0);

    uint swapSlot = (*pg >> 12);    // physical address of swap block
    cprintf("[SWAPIN] Swap slot block no : %p\n", swapSlot);
    // int swapSlot = swap;         // swap block number (convert it to integer)
    for(int i = 0; i < 8; i++)      // writes page into physical memory
    {
        struct buf *b = bread(ROOTDEV, swapSlot+i);
        memmove(memory, b->data, BSIZE);
        brelse(b);
        memory += BSIZE;
    }

    swapSlot = (swapSlot - 2) / 8;
    *pg = (*memory) | PTE_P | swp[swapSlot].page_perm;
    swp[swapSlot].is_free = 1;
    cprintf("[SWAPIN] Swap slot freed : %d\n", swapSlot);
    // swp[swapSlot].page_perm = 0;
>>>>>>> 3c1f7b3 (lab4)
}