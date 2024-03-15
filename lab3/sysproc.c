#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

/* System Call Definitions */
int 
set_sched_policy(struct proc *curproc, int policy) //(struct proc *curproc, int policy)
{
    //struct proc *curproc=myproc();
    
    curproc->policy = policy;

    cprintf("\n----------------------------------------------------------------\n\n\n");
    cprintf("set_sched_policy, curr_proc->pid=%d, policy=%d\n", curproc->pid, policy);

    if(policy != 0 && policy != 1)
     return -22;

    return 0;
}

int 
get_sched_policy(void)
{
    struct proc *curproc = myproc();

    cprintf("\n----------------------------------------------------------------\n\n\n");
    cprintf("get_sched_policy, curr_proc->pid=%d, policy=%d\n", curproc->pid, curproc->policy);

    return curproc->policy;
}
