// vm.h
//	Constantes y tipos compartidos por todo el subsistema de memoria
//	virtual (AddrSpace, FrameTable, SwapSpace, exception.cc).
#ifndef VM_H
#define VM_H
 
#include "copyright.h"
 
//Tipo de cada pagina virtual que usamos para decidir de donderecuperamos
// una pagina faltante y si esa pagina puede o no enviarse
// a swap
enum PageType {
    PAGE_CODE,
    PAGE_INIT_DATA,
    PAGE_UNINIT_DATA,
    PAGE_STACK          
};
const int SwapSize = 4096;
//con este valor idnicamos si la pagina nunca ha sido enviada a swap
const int NoSwapPage = -1;
 
#endif