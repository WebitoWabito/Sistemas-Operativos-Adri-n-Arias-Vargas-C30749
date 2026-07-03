// swapspace.cc
//Implementacion de SwapSpace
 
#include "copyright.h"
#include "system.h"
#include "swapspace.h"
 
// SwapSpace::SwapSpace
//se crea el archivo de swap y lo deja abierto durante toda la ejecucion de NachOS.
SwapSpace::SwapSpace()
{
    bool ok = fileSystem->Create("SWAP", 0);
    ASSERT(ok);
    swapFile = fileSystem->Open("SWAP");
    ASSERT(swapFile != NULL);
    freeMap = new BitMap(SwapSize);
    maxUsed = 0;
}

SwapSpace::~SwapSpace()
{
    delete swapFile;
    delete freeMap;
}
 
//se busca una pagina de swap libre y la marca como ocupada
int SwapSpace::Allocate()
{
    int page = freeMap->Find();
    ASSERT(page != -1);// si esto revienta hay que subir SwapSize
    if (page + 1 > maxUsed)
        maxUsed = page + 1;
 
    return page;
}
 
void SwapSpace::Free(int swapPage)
{
    ASSERT(swapPage >= 0 && swapPage < SwapSize);
    freeMap->Clear(swapPage);
}

//Se cuentan como accesos a disco (numDiskReads / numDiskWrites)
void SwapSpace::ReadPage(int swapPage, char *into)
{
    ASSERT(swapPage >= 0 && swapPage < SwapSize);
    swapFile->ReadAt(into, PageSize, swapPage * PageSize);
    stats->numDiskReads++;
}
 
void SwapSpace::WritePage(int swapPage, const char *from)
{
    ASSERT(swapPage >= 0 && swapPage < SwapSize);
    swapFile->WriteAt(from, PageSize, swapPage * PageSize);
    stats->numDiskWrites++;
}