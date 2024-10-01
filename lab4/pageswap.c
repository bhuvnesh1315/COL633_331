//find victim process and victim page

// for victim process
    // iterate through page table and find processes with maximum rss(for this add another attribute to struct proc in proc.h and increase it when ever u alocate a page it)
    // if rss is same then choose lowest pid process.
// for vivtim page
    // iterate through process page table and find page this flag is set `PTE_P` and `PTE_A` not set.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "buf.h"

struct swap_slot swb[NSWAP];
uint swap_start;


void initpageswap(uint sb_swap_start){
    swap_start = sb_swap_start;
    for (uint ik = 0; ik < NSWAP; ik++) {
        swb[ik].is_free = 1; // Set all slots as free initially
        swb[ik].page_perm = 0; // Initialize page permissions to 0
    }
}
// Function to find a victim page
pte_t*
find_victim_page(struct proc *p) {
    pte_t *pte;
    for (pte = p->pgdir; pte < &p->pgdir[NPTENTRIES]; pte++) {
        if ((*pte & PTE_P) && !(*pte & PTE_A)) {
            return pte;
        }
    }
    return 0;
}

// Swapping-out procedure
char *
swapout(void) {
    struct proc *victim_process = find_victim_process();
    
    pte_t *victim_page = find_victim_page(victim_process);
    if (!victim_page) {
        unmark_accessed_pages(victim_process); // In implementation, unset 10 percent of page PTE_A flags
        victim_page = find_victim_page(victim_process);
    }
    
    char * page_pa =(char *) (*victim_page>>PTXSHIFT);
    
    uint sbn = write_to_swap(page_pa);
    *victim_page=*victim_page &(0x00000111);
    *victim_page = (*victim_page & ~PTE_P) | (sbn << PTXSHIFT);
    kfree(page_pa);
    struct proc *p=myproc();
    p->rss=p->rss-4096;
    return page_pa; 
}

void
swapin() {
    struct proc *curproc = myproc();
    curproc->rss=curproc->rss+4096;
    pte_t *pte = walkpgdir(curproc->pgdir, (void *)rcr2, 1);
   
    uint sbn = *pte>>PTXSHIFT;
    char *m = kalloc(); // Allocate memory for the page
    curproc->rss+=4096;
    // Read the page from the swap space
    read_from_swap(sbn, m);
    uint me=(uint)m<<PTXSHIFT;
    // Update page table entry to mark it as swapped in
    *pte = (uint)me | PTE_P ;
    *pte=*pte & ~PTE_A;
    swb[sbn].is_free=1;
    
}

void 
unmark_accessed_pages(struct proc *victim_process){
    pte_t *pte;
    struct proc *p=myproc();
    uint b=p->rss;
    uint k=b/4096;
    k=k/10;
    for (pte = p->pgdir; pte < &p->pgdir[NPTENTRIES]; pte++) {
        if(k==0)
        break;
        if ((*pte & PTE_P) && !(*pte & PTE_A)){
            *pte=(*pte)&(~PTE_A);
            k--;
        }
            
    }
}



uint write_to_swap(char *src)
{
    uint swap_slot_index=NSWAP;
    for(int i=0;i<NSWAP;i++){
      if(swb[i].is_free==1){
      swap_slot_index=i;
      break;
      }
    }
    uint block_no= swap_start + 8*swap_slot_index;
    uint bn=block_no;
    // memmove(//find address of swap slot block , src);
    for(uint i=0;i<8;i++){
      struct buf * b=bread(ROOTDEV,block_no);
      memmove(b,src,512);
      b->blockno=block_no+i;
      bwrite(b);
      brelse(b);
      src=src+512;
    }
    swb[swap_slot_index].page_perm = PTE_W | PTE_U; 

    swb[swap_slot_index].is_free = 0;

    return bn; 
}

void read_from_swap(uint swap_slot_index, char *dst)
{
    uint block_no= swap_start + 8*swap_slot_index;
    for(uint i=0;i<8;i++){
      struct buf* b=bread(ROOTDEV,block_no);
      memmove(dst,b,512);
      dst=dst+512;
      block_no++;
      brelse(b);
    }
    swb[swap_slot_index].page_perm = PTE_W | PTE_U; 

    swb[swap_slot_index].is_free = 1;
    
}