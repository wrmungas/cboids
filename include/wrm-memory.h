#ifndef WRM_MEMORY_H
#define WRM_MEMORY_H

#include "wrm-common.h"

#define WRM_MEMORY_GROWTH_FACTOR 2

typedef struct wrm_Pool wrm_Pool;

struct wrm_Pool {
    size_t element_size;
    size_t cap;
    size_t used;
    void *data;
    bool *is_used;
};

void wrm_Pool_init(wrm_Pool *p, size_t cap, size_t element_size);

wrm_Option_Handle wrm_Pool_getSlot(wrm_Pool *p);

void wrm_Pool_freeSlot(wrm_Pool *p, wrm_Handle idx);

void wrm_Pool_delete(wrm_Pool *p);

// type-generic accessor macro

#define wrm_Pool_dataAs(pool, t) ((t*)pool.data)

// template-like macro

/*
#define DEFINE_POOL(t, t_name) typedef struct wrm_Pool_ ## t_name { \
    u32 cap; \
    u32 used; \ 
    t *data; \
    bool *is_used; \
} wrm_Pool_ ## t_name; 

#define DECLARE_POOL_FNS(t_name) \
void wrm_Pool_ ## t_name ## _init(wrm_Pool_ ## t_name *p, size_t cap); \
wrm_Option_Handle wrm_Pool_ ## t_name ## _getSlot(wrm_Pool_ ## t_name *p); \
void wrm_Pool_ ## t_name ## _freeSlot(wrm_Handle idx); \
void wrm_Pool_ ## t_name ## _delete(wrm_Pool_ ## t_name *p) 

#define DEFINE_POOL_FNS(t, t_name) \
void wrm_Pool_ ## t_name ## _init(wrm_Pool_ ## t_name *p, size_t cap) \
{ \
    if(!p) return; \
    p->cap = cap; \
    p->used = 0; \
    p->data = calloc(cap, sizeof(t)); \
    p->is_used = calloc(cap, sizeof(bool)); \
} \
\
wrm_Option_Handle wrm_Pool_ ## t_name ## _getSlot(wrm_Pool_ ## t_name *p) \
{ \
    if(p->used == p->cap) { \
        if(!(realloc(p->data, p->cap * WRM_MEMORY_GROWTH_FACTOR) && realloc(p->is_used, p->cap * WRM_MEMORY_GROWTH_FACTOR))) { \
            realloc(p->data, p->cap); \
            return (wrm_Option_Handle){.exists = false}; \
        } \
        p->cap *= WRM_MEMORY_GROWTH_FACTOR; \
        p->is_used[p->used] = true; \
        return (wrm_Option_Handle){.exists = true, .Handle_val = p->used++}; \
    } \
    u32 i = 0; \
    while(p->is_used[i++]) {} \
    p->is_used[i] = true; \
    p->used++; \
    return (wrm_Option_Handle){.exists = true, .Handle_val = i}; \
} \
\
void wrm_Pool_freeSlot(wrm_Pool_ ## t_name *p, wrm_Handle idx) \
{ \
    if(idx >= p->cap) return; \
    p->is_used[idx] = false; \
    p->used--; \
} \
\
void wrm_mem_deletePool(wrm_Pool_ ## t_name *p) \
{ \
    free(p->data); \
    free(p->is_used); \
    p->data = NULL; \
    p->is_used = NULL; \
}
*/


#endif