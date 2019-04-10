
#ifndef MEM_POOL_H
#define MEM_POOL_H

#include <stdint.h>
#include <pthread.h>

typedef struct mem_object_s
{
    uint8_t *ptr;
    uint32_t len;
    struct mem_object_s *next;
} mem_object_t;


typedef struct mem_pool_s
{
   uint8_t *buffer;
   uint32_t len;
   mem_object_t *aval_root;
   mem_object_t *used_root;
} mem_pool_t;

static mem_pool_t g_mem_pool;
static pthread_mutex_t g_mempool_mutex;

int hh_mempool_init();
mem_object_t *hh_mempool_malloc(uint32_t size);
int hh_mempool_free(mem_object_t *ptr);
void hh_mempool_exit();


#endif  // MEM_POOL_H


