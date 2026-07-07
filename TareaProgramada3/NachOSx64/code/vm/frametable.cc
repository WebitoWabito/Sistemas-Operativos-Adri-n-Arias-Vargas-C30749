//frametable.cc
//Implementacion de FrameTable
 
#include "copyright.h"
#include "system.h"
#include "frametable.h"
#include "addrspace.h"
 
 
FrameTable::FrameTable()
{
    for (int i = 0; i < NumPhysPages; i++) {
        frames[i].free     = true;
        frames[i].owner    = NULL;
        frames[i].vpn      = -1;
        frames[i].lastUsed = 0;
    }
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
            frames[i].free     = false;
            frames[i].owner    = owner;
            frames[i].vpn      = vpn;
            frames[i].lastUsed = lruClock;
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
 
void
FrameTable::Touch(int frame)
{
    ASSERT(frame >= 0 && frame < NumPhysPages);
    frames[frame].lastUsed = lruClock;
}
 
int
FrameTable::PickVictim()
{
    int victim = -1;
 
    for (int i = 0; i < NumPhysPages; i++) {
        if (frames[i].free)
            continue;
        if (victim == -1 || frames[i].lastUsed < frames[victim].lastUsed)
            victim = i;
    }
 
    ASSERT(victim != -1);//solo se llama cuando la memoria esta llena
    return victim;
}