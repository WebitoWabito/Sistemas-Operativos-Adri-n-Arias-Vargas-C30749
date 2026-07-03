//swapspace.h
//SwapSpace administra el area de SWAP en disco.

#ifndef SWAPSPACE_H
#define SWAPSPACE_H
 
#include "copyright.h"
#include "bitmap.h"
#include "filesys.h"
#include "vm.h"
 
class SwapSpace {
  public:
    SwapSpace();
    ~SwapSpace();
 
    int  Allocate();// sereserva una pagina de swap libre y retorna su numero o -1 si no hay espacio
    void Free(int swapPage);//se libera una pagina de swap
    void ReadPage(int swapPage, char *into);//se lee PageSize bytes
    void WritePage(int swapPage, const char *from);//se escribe PageSize bytes
    int MaxUsed() { return maxUsed; }   // maxima posicion ocupada del swap

  private:
    OpenFile *swapFile;
    BitMap   *freeMap;
    int       maxUsed;
};
 
#endif