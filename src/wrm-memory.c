#include "wrm-common.h"
#include "wrm-memory.h"


void wrm_Pool_init(wrm_Pool *p, size_t cap, size_t element_size)
{
    if(!p) return;

    p->cap = cap;
    p->element_size = element_size;
    p->used = 0;

    p->data = calloc(cap, element_size);
    p->is_used = calloc(cap, sizeof(bool));
}

wrm_Option_Handle wrm_Pool_getSlot(wrm_Pool *p)
{
    if(p->used == p->cap) {
        if(!(realloc(p->data, p->cap * WRM_MEMORY_GROWTH_FACTOR * p->element_size ) && realloc(p->is_used, p->cap * WRM_MEMORY_GROWTH_FACTOR * p->element_size))) {
            return (wrm_Option_Handle){.exists = false};
        }
        p->cap *= WRM_MEMORY_GROWTH_FACTOR;
        p->is_used[p->used] = true;
        return (wrm_Option_Handle){.exists = true, .Handle_val = p->used++};
    }

    size_t i = 0;
    while(p->is_used[i]) { i++; }
    p->is_used[i] = true;
    p->used++;
    return (wrm_Option_Handle){.exists = true, .Handle_val = i};
}

void wrm_Pool_freeSlot(wrm_Pool *p, wrm_Handle idx)
{
    if(idx >= p->cap) return;
    p->is_used[idx] = false;
    p->used--;
}

void wrm_Pool_delete(wrm_Pool *p)
{
    free(p->data);
    free(p->is_used);

    p->data = NULL;
    p->is_used = NULL;
}
