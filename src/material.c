
#include <glib.h>

#include "material.h"

struct material* materials; 

void MATRL_Init(indexSize_t tableSize)   
{
  materials = (struct material*)g_malloc(sizeof(struct material*) * tableSize);
}