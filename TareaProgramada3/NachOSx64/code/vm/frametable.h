// frametable.h
#ifndef FRAMETABLE_H
#define FRAMETABLE_H
 
#include "copyright.h"
#include "machine.h"//NumPhysPages, PageSize
#include "vm.h"
 
class AddrSpace;
 
struct FrameEntry {
    bool free;
    AddrSpace *owner; // proceso dueño del frame
    int vpn;// pagina virtual del due{o mapeada en este frame
};
 
class FrameTable {
  public:
    FrameTable();
    ~FrameTable();
 
    int  AllocFrame(AddrSpace *owner, int vpn);//busca un frame libre, lo marca como ocupado por owner, vpn y retorna su numero. Retorna -1 si no hay frames libres
    void FreeFrame(int frame);//se marca el frame como libre
    bool HasFreeFrame();
    AddrSpace *GetOwner(int frame) { return frames[frame].owner; }
    int        GetVPN(int frame)   { return frames[frame].vpn; }
    int NumFrames() { return NumPhysPages; }
 
  private:
    FrameEntry frames[NumPhysPages];
};
 
#endif