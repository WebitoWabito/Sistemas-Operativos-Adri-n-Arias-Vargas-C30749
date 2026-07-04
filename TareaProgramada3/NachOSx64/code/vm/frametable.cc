// frametable.cc
//	Implementacion de FrameTable
 
#include "copyright.h"
#include "system.h"
#include "frametable.h"
#include "addrspace.h"

FrameTable::FrameTable()
{
    for (int i = 0; i < NumPhysPages; i++) {
        frames[i].free  = true;
        frames[i].owner = NULL;
        frames[i].vpn   = -1;
    }
    clockHand = 0;
}
 
FrameTable::~FrameTable()
{
}
 
bool
FrameTable::HasFreeFrame()
{
    for (int i = 0; i < NumPhysPages; i++)
        if (frames[i].free)
            return true;
    return false;
}

int
FrameTable::AllocFrame(AddrSpace *owner, int vpn)
{
    for (int i = 0; i < NumPhysPages; i++) {
        if (frames[i].free) {
            frames[i].free  = false;
            frames[i].owner = owner;
            frames[i].vpn   = vpn;
            return i;
        }
    }
    return -1;// memoria fisica llena, hay que reemplazar
}

void
FrameTable::FreeFrame(int frame)
{
    ASSERT(frame >= 0 && frame < NumPhysPages);
    frames[frame].free  = true;
    frames[frame].owner = NULL;
    frames[frame].vpn   = -1;
}

int
FrameTable::PickVictim()
{
    for (;;) {
        FrameEntry &f = frames[clockHand];
        if (!f.free) {
            TranslationEntry *pte = f.owner->GetPageTableEntry(f.vpn);
            if (pte->use) {
                pte->use = false;//second chance
            } else {
                int victim = clockHand;
                clockHand = (clockHand + 1) % NumPhysPages;
                return victim;
            }
        }
        clockHand = (clockHand + 1) % NumPhysPages;
    }
}