

#ifndef MEM_POOL_H
#define MEM_POOL_H


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <memory.h>
#include <map>
#include <vector>


using namespace std;



class mem_pool
{
public:
    mem_pool(uint32_t size);
    mem_pool(uint8_t *pt, uint32_t size);
    ~mem_pool();

private:
    uint32_t mem_pool_size;
    uint8_t *mem_pool_buf;
    uint32_t *last_used_offset;
    map<uint32_t, vector<uint8_t *> > avail_size_map;
    map<uint8_t *, uint32_t> avail_map;
    map<uint8_t *, uint32_t> used_map;

public:
    uint8_t *get_mem_pool_buf() { return mem_pool_buf; }
    void clear();
    uint8_t *mem_pool_alloc(uint32_t size);
    void mem_pool_free(uint8_t *used_buf);
    void insert_used(uint8_t *avail_buf, uint32_t size);
    void insert_avail(uint8_t *avail_buf, uint32_t size);
    void insert_last_avail(uint8_t *avail_buf);
    uint8_t *mem_pool_realloc(uint8_t *used_buf, uint32_t new_size);

private:
    void mem_pool_merge();
};


#endif  //  MEM_POOL_H

