// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
 
#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include <string.h>
 
//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------
 
static void
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}
 
#ifdef VM
 
static bool
PageOverlapsCode(unsigned int vpn, NoffHeader *noffH)
{
    if (noffH->code.size == 0) return false;
    unsigned int pageStart = vpn * PageSize, pageEnd = pageStart + PageSize;
    unsigned int segStart  = noffH->code.virtualAddr;
    unsigned int segEnd    = segStart + noffH->code.size;
    return pageStart < segEnd && segStart < pageEnd;
}

static bool
PageOverlapsData(unsigned int vpn, NoffHeader *noffH)
{
    if (noffH->initData.size == 0) return false;
    unsigned int pageStart = vpn * PageSize, pageEnd = pageStart + PageSize;
    unsigned int segStart  = noffH->initData.virtualAddr;
    unsigned int segEnd    = segStart + noffH->initData.size;
    return pageStart < segEnd && segStart < pageEnd;
}

static PageType
ClassifyPage(unsigned int vpn, NoffHeader *noffH, unsigned int numPages)
{
    unsigned int stackPages = divRoundUp(UserStackSize, PageSize);
    unsigned int stackStart = numPages - stackPages;
 
    if (vpn >= stackStart)
        return PAGE_STACK;
 
    if (noffH->uninitData.size > 0) {
        unsigned int bssStart = noffH->uninitData.virtualAddr / PageSize;
        if (vpn >= bssStart)
            return PAGE_UNINIT_DATA;
    }
 
    if (PageOverlapsData(vpn, noffH))
        return PAGE_INIT_DATA;
 
    if (PageOverlapsCode(vpn, noffH))
        return PAGE_CODE;
 
    return PAGE_INIT_DATA;
}
 
static void
FillPageContent(char *dest, unsigned int vpn,
                 NoffHeader *noffH, OpenFile *executableFile)
{
    unsigned int pageStart = vpn * PageSize, pageEnd = pageStart + PageSize;
 
    if (PageOverlapsCode(vpn, noffH)) {
        unsigned int segStart = noffH->code.virtualAddr;
        unsigned int segEnd   = segStart + noffH->code.size;
        unsigned int lo = (pageStart > segStart) ? pageStart : segStart;
        unsigned int hi = (pageEnd   < segEnd)   ? pageEnd   : segEnd;
        int fileAddr = noffH->code.inFileAddr + (lo - segStart);
        executableFile->ReadAt(dest + (lo - pageStart), hi - lo, fileAddr);
    }
 
    if (PageOverlapsData(vpn, noffH)) {
        unsigned int segStart = noffH->initData.virtualAddr;
        unsigned int segEnd   = segStart + noffH->initData.size;
        unsigned int lo = (pageStart > segStart) ? pageStart : segStart;
        unsigned int hi = (pageEnd   < segEnd)   ? pageEnd   : segEnd;
        int fileAddr = noffH->initData.inFileAddr + (lo - segStart);
        executableFile->ReadAt(dest + (lo - pageStart), hi - lo, fileAddr);
    }
}
#endif // VM
 
 
//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------
 
AddrSpace::AddrSpace(OpenFile *executable)
{
    unsigned int i, size;
#ifdef VM
    NoffHeader &hdr = this->noffH;//se guarda directo en el miembro
#else
    NoffHeader hdr;
#endif
 
    executable->ReadAt((char *)&hdr, sizeof(hdr), 0);
    if ((hdr.noffMagic != NOFFMAGIC) &&
        (WordToHost(hdr.noffMagic) == NOFFMAGIC))
        SwapHeader(&hdr);
    ASSERT(hdr.noffMagic == NOFFMAGIC);
 
    // how big is address space?
    size = hdr.code.size + hdr.initData.size + hdr.uninitData.size
           + UserStackSize;
 
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
 
#ifdef VM
    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
          numPages, size);
 
    executableFile = executable;//se guarda para poder leer paginas bajo demanda durante toda la vida del proceso
    pageTable = new TranslationEntry[numPages];
    pageType  = new PageType[numPages];
    swapSlot  = new int[numPages];
 
    for (i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
        pageTable[i].physicalPage = -1;
        pageTable[i].valid        = false;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
 
        pageType[i] = ClassifyPage(i, &hdr, numPages);
        swapSlot[i] = NoSwapPage;
    }
#else
    // verificamos que hay suficiente memoria fṕisica para cargar el programa
    ASSERT(numPages <= NumPhysPages);
    ASSERT((int)numPages <= machine->freeMap->NumClear());
 
    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
          numPages, size);
 
    //se asignan frames libres desde freemap para la pagetable del proceso, y se inicializan los campos de cada entrada
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
        pageTable[i].physicalPage = machine->freeMap->Find(); // pide un frame libre
        pageTable[i].valid        = true;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
 
        //limpia el frame antes de usarlo
        bzero(machine->mainMemory + pageTable[i].physicalPage * PageSize, PageSize);
    }
 
    // se carga el segmento de código pagina por página
    if (hdr.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
          hdr.code.virtualAddr, hdr.code.size);
 
        int numCodePages = divRoundUp(hdr.code.size, PageSize);
        for (int i = 0; i < numCodePages; i++) {
            int vpn      = (hdr.code.virtualAddr / PageSize) + i;
            int physAddr = pageTable[vpn].physicalPage * PageSize;
            int fileAddr = hdr.code.inFileAddr + i * PageSize;
            executable->ReadAt(&(machine->mainMemory[physAddr]), PageSize, fileAddr);
        }
    }   
    //segmento de datos pagina por pagina
    if (hdr.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
          hdr.initData.virtualAddr, hdr.initData.size);
 
        int numDataPages = divRoundUp(hdr.initData.size, PageSize);
        for (int i = 0; i < numDataPages; i++) {
            int vpn      = (hdr.initData.virtualAddr / PageSize) + i;
            int physAddr = pageTable[vpn].physicalPage * PageSize;
            int fileAddr = hdr.initData.inFileAddr + i * PageSize;
            executable->ReadAt(&(machine->mainMemory[physAddr]), PageSize, fileAddr);
        }
    }
#endif
}
 
AddrSpace::AddrSpace(AddrSpace *other)
{
    numPages = other->numPages;
 
#ifdef VM
    noffH          = other->noffH;
    executableFile = other->executableFile;
    pageTable = new TranslationEntry[numPages];
    pageType  = new PageType[numPages];
    swapSlot  = new int[numPages];
 
    for (unsigned int i = 0; i < numPages; i++) {
        pageType[i] = other->pageType[i];
        swapSlot[i] = NoSwapPage;
 
        pageTable[i].virtualPage  = i;
        pageTable[i].physicalPage = -1;
        pageTable[i].valid        = false;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
 
        if (!other->pageTable[i].valid && other->swapSlot[i] == NoSwapPage)
            continue;
        int frame = frameTable->AllocFrame(this, i);
        while (frame == -1) {
            int victimFrame = frameTable->PickVictim();
            AddrSpace *victimOwner = frameTable->GetOwner(victimFrame);
            int victimVpn = frameTable->GetVPN(victimFrame);
            victimOwner->EvictPage(victimVpn);
            frameTable->FreeFrame(victimFrame);
            frame = frameTable->AllocFrame(this, i);
        }
 
        char *dest = &(machine->mainMemory[frame * PageSize]);
        if (other->pageTable[i].valid) {
            //el padre la tiene en memoria se copia directo
            int srcPhys = other->pageTable[i].physicalPage * PageSize;
            memcpy(dest, &(machine->mainMemory[srcPhys]), PageSize);
        } else {
            //el padre la tiene en swap se lee de ahi
            swapSpace->ReadPage(other->swapSlot[i], dest);
        }
 
        pageTable[i].physicalPage = frame;
        pageTable[i].valid        = true;
    }
#else
    ASSERT((int)numPages <= machine->freeMap->NumClear());
 
    pageTable = new TranslationEntry[numPages];
    for (unsigned int i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
        pageTable[i].physicalPage = machine->freeMap->Find();
        pageTable[i].valid        = true;
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
 
        //se copia el contenido del frame original al nuevo frame
        int srcPhys  = other->pageTable[i].physicalPage * PageSize;
        int destPhys = pageTable[i].physicalPage * PageSize;
        memcpy(machine->mainMemory + destPhys,
               machine->mainMemory + srcPhys,
               PageSize);
    }
#endif
}
 
//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Deallocate an address space, including anything it loaded into memory.
//  Libera los frames físicos en el freeMap para que otros procesos
//  puedan usarlos.
//----------------------------------------------------------------------
 
AddrSpace::~AddrSpace()
{
#ifdef VM
    for (unsigned int i = 0; i < numPages; i++) {
        if (pageTable[i].valid)
            frameTable->FreeFrame(pageTable[i].physicalPage);
        if (swapSlot[i] != NoSwapPage)
            swapSpace->Free(swapSlot[i]);
    }
    delete [] pageType;
    delete [] swapSlot;

#else
    // se liberan las paginas asignadas al proceso en el freeMap
    for (unsigned int i = 0; i < numPages; i++) {
        machine->freeMap->Clear(pageTable[i].physicalPage);
    }
#endif
    delete [] pageTable;
}
 
//----------------------------------------------------------------------
// AddrSpace::InitRegisters
//	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------
 
void
AddrSpace::InitRegisters()
{
    int i;
 
    for (i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, 0);
 
    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);
 
    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);
 
    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we
    // don't accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}
 
//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//----------------------------------------------------------------------
 
void AddrSpace::SaveState()
{
}
 
//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------
 
void AddrSpace::RestoreState()
{
#ifdef VM
    for (int i = 0; i < TLBSize; i++)
        machine->tlb[i].valid = false;
#else
    machine->pageTable     = pageTable;
    machine->pageTableSize = numPages;
#endif
}
 
#ifdef VM

void
AddrSpace::LoadPage(unsigned int vpn)
{
    ASSERT(vpn < numPages);
    ASSERT(!pageTable[vpn].valid);
 
    int frame = frameTable->AllocFrame(this, vpn);
    while (frame == -1) {
        //la memoria fisica llena, hay que sacar una victima primero
        int victimFrame = frameTable->PickVictim();
        AddrSpace *victimOwner = frameTable->GetOwner(victimFrame);
        int victimVpn = frameTable->GetVPN(victimFrame);
 
        victimOwner->EvictPage(victimVpn);
        frameTable->FreeFrame(victimFrame);
 
        frame = frameTable->AllocFrame(this, vpn);
    }
 
    char *dest = &(machine->mainMemory[frame * PageSize]);
    PageType type = pageType[vpn];
 
    if (swapSlot[vpn] != NoSwapPage) {
        swapSpace->ReadPage(swapSlot[vpn], dest);
    } else {
        bzero(dest, PageSize);
        FillPageContent(dest, vpn, &noffH, executableFile);
    }
 
    pageTable[vpn].virtualPage  = vpn;
    pageTable[vpn].physicalPage = frame;
    pageTable[vpn].valid        = true;
    pageTable[vpn].use          = false;
    pageTable[vpn].dirty        = false;
    pageTable[vpn].readOnly     = (type == PAGE_CODE);
 
    stats->numPageFaults++;
}
 
void
AddrSpace::EvictPage(unsigned int vpn)
{
    TranslationEntry *pte = &pageTable[vpn];
    ASSERT(pte->valid);
    if (currentThread->space == this) {
        for (int i = 0; i < TLBSize; i++) {
            if (machine->tlb[i].valid && machine->tlb[i].virtualPage == (int)vpn) {
                pte->use   = machine->tlb[i].use;
                pte->dirty = machine->tlb[i].dirty;
                machine->tlb[i].valid = false;
            }
        }
    }
 
    PageType type = pageType[vpn];
    if (type != PAGE_CODE && pte->dirty) {
        if (swapSlot[vpn] == NoSwapPage)
            swapSlot[vpn] = swapSpace->Allocate();
        swapSpace->WritePage(swapSlot[vpn],
                              &(machine->mainMemory[pte->physicalPage * PageSize]));
    }
 
    pte->valid = false;
}
#endif // VM
