#include "types.h"
#include "defs.h"
#include "x86.h"
#include "mouse.h"
#include "traps.h"

// Wait until the mouse controller is ready for us to send a packet
void 
mousewait_send(void) 
{
    while( (inb(MSSTATP) & 0x02) != 0x00){}
    return;
}

// Wait until the mouse controller has data for us to receive
void 
mousewait_recv(void) 
{
    while( (inb(MSSTATP) & 0x01) == 0x00){}
    return;
}

// Send a one-byte command to the mouse controller, and wait for it
// to be properly acknowledged
void 
mousecmd(uchar cmd) 
{
    mousewait_send();
    outb(MSSTATP, PS2MS);

    mousewait_send();
    outb(MSDATAP, cmd);

    mousewait_recv();
    inb(MSDATAP);

    return;
}

void
mouseinit(void)
{
    mousewait_send();
    outb(MSSTATP, MSEN);

    mousewait_send();
    outb(MSSTATP, 0x20);

    mousewait_recv();

    uchar status_B = inb(MSDATAP) | 0x02;
    //status_B |= (0x02);
    
    mousewait_send();
    outb(MSSTATP, 0x60);

    mousewait_send();
    outb(MSDATAP, status_B);

    mousecmd(0xF6);
    
    mousecmd(0xF4);

    ioapicenable(IRQ_MOUSE, 0);
    cprintf("Mouse has been initialized");
    return;
}

void
mouseintr(void)
{
    uchar status;
    while(1) 
    {

        if( (inb(MSSTATP) & 0x01) == 0x00)
        {
            return;
        }
        status = inb(MSDATAP);
        inb(MSDATAP);
        inb(MSDATAP);
        if(status & 0x01)
            cprintf("LEFT\n");
        if( status& 0x02)
            cprintf("RIGHT\n");
        if(status & 0x04)
            cprintf("MID\n");

    }
    return;
}
