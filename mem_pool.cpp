

#include "mem_pool.h"
#include <iostream>
#include <algorithm>


mem_pool::mem_pool(uint32_t size)
{
    mem_pool_buf = (uint8_t *)malloc(size);
    mem_pool_size = size;
    last_used_offset = (uint32_t *)mem_pool_buf;
}

mem_pool::mem_pool(uint8_t *pt, uint32_t size)
{
    mem_pool_buf = pt;
    mem_pool_size = size;
    last_used_offset = (uint32_t *)pt;
}

mem_pool::~mem_pool()
{
    free(mem_pool_buf);
    used_map.clear();
    avail_map.clear();
    avail_size_map.clear();
}

void mem_pool::clear()
{
    memset(mem_pool_buf, 0, mem_pool_size);
    used_map.clear();
    avail_map.clear();
    avail_size_map.clear();
    avail_map[mem_pool_buf + sizeof(uint32_t)] = mem_pool_size - sizeof(uint32_t);
    avail_size_map[mem_pool_size - sizeof(uint32_t)].push_back(mem_pool_buf + sizeof(uint32_t));
    *last_used_offset = sizeof(uint32_t);
}

uint8_t *mem_pool::mem_pool_alloc(uint32_t size)
{
    if ((size == 0) || (size > mem_pool_size) || ((!used_map.empty()) && (avail_map.empty())))
        return NULL;

    if (used_map.empty())
    {
        used_map[mem_pool_buf + sizeof(uint32_t)] = size;
        avail_map.clear();
        avail_map[mem_pool_buf + sizeof(uint32_t) + size] = mem_pool_size - size - sizeof(uint32_t);
        avail_size_map.clear();
        avail_size_map[mem_pool_size - size - sizeof(uint32_t)].push_back(mem_pool_buf + sizeof(uint32_t) + size);
        *last_used_offset = sizeof(uint32_t) + size;
        return mem_pool_buf + sizeof(uint32_t);
    }
    else
    {
        for (map<uint32_t, vector<uint8_t *> >::iterator size_it = avail_size_map.begin(); size_it != avail_size_map.end(); size_it++)
        {
            if (size_it->first < size)
                continue;
            if (size_it->second.size() == 0)
            {
                cout << "avail_size_map ERROR" << endl;
                while (size_it != avail_size_map.end())
                {
                    cout << "avail_size_map, size: " << size_it->first << ", count: " << size_it->second.size() << endl;
                    size_it++;
                }
                break;
            }
            map<uint8_t *, uint32_t>::iterator avail_it = avail_map.find(size_it->second[0]);
            if (avail_it == avail_map.end())
                return NULL;
            uint8_t *used_buf = avail_it->first;
            used_map[used_buf] = size;
            if (avail_it->second > size)
            {
                avail_map[avail_it->first + size] = avail_it->second - size;
                avail_size_map[avail_it->second - size].push_back(avail_it->first + size);
            }
            avail_map.erase(avail_it++);
            size_it->second.erase(size_it->second.begin());
            if (size_it->second.empty())
                avail_size_map.erase(size_it);
            if (used_buf + size - mem_pool_buf > *last_used_offset)
                *last_used_offset = used_buf + size - mem_pool_buf;
            return used_buf;
        }
        //  merge
        cout << "alloc size: " << size << " fail, merge" << endl;
        for (map<uint32_t, vector<uint8_t *> >::iterator size_it = avail_size_map.begin(); size_it != avail_size_map.end(); size_it++)
        {
            cout << size_it->first << endl;
        }
#if 0
        mem_pool_merge();
        if (avail_map.begin()->second >= size)
        {
            uint8_t *used_buf = avail_map.begin()->first;
            uint32_t last_avail_size = avail_map.begin()->second;
            used_map[used_buf] = size;
            avail_map.clear();
            avail_size_map.clear();
            if (last_avail_size > size)
            {
                avail_map[used_buf + size] = last_avail_size - size;
                avail_size_map[last_avail_size - size].push_back(used_buf + size);
            }
            *last_used_offset = used_buf + size - mem_pool_buf;
            return used_buf;
        }
#endif
        return NULL;
    }
}

void mem_pool::mem_pool_free(uint8_t *used_buf)
{
    if ((used_buf == NULL) || (used_map.empty()))
        return;

    map<uint8_t *, uint32_t>::iterator used_it = used_map.find(used_buf);
    if (used_it == used_map.end())
        return;

    do
    {
        pair<map<uint8_t *, uint32_t>::iterator, bool> avail_it = avail_map.insert(pair<uint8_t *, uint32_t>(used_buf, used_it->second));
        if (!avail_it.second)
            break;

        map<uint8_t *, uint32_t>::iterator avail_it1 = avail_it.first;
        if (avail_it1 != avail_map.begin())
        {
            avail_it1--;
            if (avail_it1->first + avail_it1->second == used_buf)
            {
                cout << "merge before" << endl;
                uint8_t *last_avail_buf = avail_it1->first;
                uint32_t last_avail_buf_len = avail_it1->second;
                avail_it1 = avail_it.first;
                avail_it1++;
                if ((avail_it1 != avail_map.end()) && (used_buf + used_it->second == avail_it1->first))
                {
                    cout << "merge before and after" << endl;
                    avail_map[last_avail_buf] = last_avail_buf_len + used_it->second + avail_it1->second;
                    avail_map.erase(avail_it.first);
                    map<uint32_t, vector<uint8_t *> >::iterator size_it = avail_size_map.find(last_avail_buf_len);
                    if (size_it == avail_size_map.end())
                    {
                        //   error
                        cout << "not find avail_size_map erase it" << endl;
                    }
                    else
                    {
                        vector<uint8_t *>::iterator erase_it = find(size_it->second.begin(), size_it->second.end(), last_avail_buf);
                        if (erase_it == size_it->second.end())
                        {
                            cout << "not find avail_size_map erase it" << endl;
                        }
                        else
                            size_it->second.erase(erase_it);
                        if (size_it->second.empty())
                            avail_size_map.erase(size_it);
                    }
                    size_it = avail_size_map.find(avail_it1->second);
                    if (size_it == avail_size_map.end())
                    {
                        //   error
                        cout << "not find avail_size_map erase it" << endl;
                    }
                    else
                    {
                        vector<uint8_t *>::iterator erase_it = find(size_it->second.begin(), size_it->second.end(), avail_it1->first);
                        if (erase_it == size_it->second.end())
                        {
                            cout << "not find avail_size_map erase it" << endl;
                        }
                        else
                            size_it->second.erase(erase_it);
                        if (size_it->second.empty())
                            avail_size_map.erase(size_it);
                    }
                    avail_map.erase(avail_it1);
                }
                else
                {
                    cout << "merge before but not after" << endl;
                    avail_map[last_avail_buf] = last_avail_buf_len + used_it->second;
                    avail_map.erase(avail_it.first);
                    map<uint32_t, vector<uint8_t *> >::iterator size_it = avail_size_map.find(last_avail_buf_len);
                    if (size_it == avail_size_map.end())
                    {
                        //   error
                        cout << "not find avail_size_map erase it" << endl;
                    }
                    else
                    {
                        vector<uint8_t *>::iterator erase_it = find(size_it->second.begin(), size_it->second.end(), last_avail_buf);
                        if (erase_it == avail_size_map[last_avail_buf_len].end())
                        {
                            cout << "not find avail_size_map erase it" << endl;
                        }
                        else
                            size_it->second.erase(erase_it);
                        if (avail_size_map[last_avail_buf_len].empty())
                            avail_size_map.erase(last_avail_buf_len);
                    }
                }
                avail_size_map[avail_map[last_avail_buf]].push_back(last_avail_buf);
                break;
            }
        }
        avail_it1++;
        if ((avail_it1 != avail_map.end()) && (used_buf + used_it->second == avail_it1->first))
        {
            cout << "merge after" << endl;
            avail_it.first->second += avail_it1->second;
            map<uint32_t, vector<uint8_t *> >::iterator size_it = avail_size_map.find(avail_it1->second);
            if (size_it == avail_size_map.end())
            {
                //   error
                cout << "not find avail_size_map erase it" << endl;
            }
            else
            {
                vector<uint8_t *>::iterator erase_it = find(size_it->second.begin(), size_it->second.end(), avail_it1->first);
                if (erase_it == size_it->second.end())
                {
                    cout << "not find avail_size_map erase it" << endl;
                }
                else
                    size_it->second.erase(erase_it);
                if (size_it->second.empty())
                    avail_size_map.erase(size_it);
            }
            avail_map.erase(avail_it1);
        }
        avail_size_map[avail_it.first->second].push_back(used_buf);
    } while (0);

    memset(used_buf, 0, used_it->second);
    if (used_buf + used_it->second - mem_pool_buf == *last_used_offset)
    {
        if (used_it == used_map.begin())
        {
            clear();
            return;
        }
        map<uint8_t *, uint32_t>::iterator used_it1 = used_it;
        used_it1--;
        *last_used_offset = used_it1->first + used_it1->second - mem_pool_buf;
    }
    used_map.erase(used_it);
}

void mem_pool::insert_used(uint8_t *used_buf, uint32_t size)
{
    if ((used_buf == NULL) || (used_buf < mem_pool_buf) || (used_buf + size - mem_pool_buf > mem_pool_size) || (size == 0))
        return;

    used_map[used_buf] = size;
}

void mem_pool::insert_avail(uint8_t *avail_buf, uint32_t size)
{
    if ((avail_buf == NULL) || (avail_buf < mem_pool_buf) || (avail_buf + size - mem_pool_buf > mem_pool_size) || (size == 0))
        return;

    avail_map[avail_buf] = size;
    avail_size_map[size].push_back(avail_buf);
}

void mem_pool::insert_last_avail(uint8_t *avail_buf)
{
    if ((avail_buf == NULL) || (avail_buf < mem_pool_buf) || (avail_buf - mem_pool_buf > mem_pool_size))
        return;

    avail_map[avail_buf] = mem_pool_size - (avail_buf - mem_pool_buf);
    avail_size_map[mem_pool_size - (avail_buf - mem_pool_buf)].push_back(avail_buf);
}

uint8_t *mem_pool::mem_pool_realloc(uint8_t *used_buf, uint32_t new_size)
{
#ifdef NEED_SEARCH_LIST
    if ((used_buf == NULL) || (used_list_root == NULL))
        return NULL;

    mem_pool_obj_pt used_obj = used_list_root;
    while (used_obj != NULL)
    {
        if (used_obj->obj_buf == used_buf)
        {
            mem_pool_obj_pt avail_obj = avail_list_root;
            while (avail_obj != NULL)
            {
                if (used_obj->obj_buf + used_obj->obj_buf_len == avail_obj->obj_buf)
                {
                    avail_obj->obj_buf_len -= new_size - used_obj->obj_buf_len;
                    avail_obj->obj_buf += new_size - used_obj->obj_buf_len;
                    used_obj->obj_buf_len = new_size;
                    used_buf = used_obj->obj_buf;
                    return used_obj->obj_buf;
                }
                else if (avail_obj->obj_buf + avail_obj->obj_buf_len == used_obj->obj_buf)
                {
                    uint32_t inc_size = new_size - used_obj->obj_buf_len, old_buf_len = used_obj->obj_buf_len;
                    avail_obj->obj_buf_len -= inc_size;
                    used_obj->obj_buf -= inc_size;
                    used_obj->obj_buf_len += inc_size;
                    for (uint32_t i = 0; i < new_size; i++)
                    {
                        if (i < old_buf_len)
                            used_obj->obj_buf[i] = used_obj->obj_buf[i + inc_size];
                        else
                            used_obj->obj_buf[i] = 0;
                    }
                    used_buf = used_obj->obj_buf;
                    return used_obj->obj_buf;
                }
                avail_obj = avail_obj->pnext;
            }
            avail_obj = avail_list_root;
            while (avail_obj != NULL)
            {
                if (avail_obj->obj_buf_len > new_size)
                {
                    uint8_t *free_buf = used_obj->obj_buf;
                    uint32_t free_buf_len = used_obj->obj_buf_len;
                    used_obj->obj_buf = avail_obj->obj_buf;
                    used_obj->obj_buf_len += new_size;
                    avail_obj->obj_buf_len -= new_size;
                    avail_obj->obj_buf += new_size;
                    mem_pool_obj_pt free_obj = (mem_pool_obj_pt)malloc(sizeof(mem_pool_obj_t));
                    free_obj->obj_buf = free_buf;
                    free_obj->obj_buf_len = free_buf_len;
                    free_obj->pnext = avail_obj->pnext;
                    avail_obj->pnext = free_obj;
                    used_buf = used_obj->obj_buf;
                    return used_obj->obj_buf;
                }
                avail_obj = avail_obj->pnext;
            }
        }
    }
#endif

    return NULL;
}

typedef struct merge_copy_data_s
{
    uint32_t buf_len;
    uint8_t *buf;
} merge_copy_data_t;

void mem_pool::mem_pool_merge()
{
    vector<merge_copy_data_t> merge_copy_data;
    *last_used_offset = sizeof(uint32_t);
    for (map<uint8_t *, uint32_t>::iterator used_it = used_map.begin(); used_it != used_map.end(); used_it++)
    {
        merge_copy_data_t copy_data;
        copy_data.buf_len = used_it->second;
        copy_data.buf = (uint8_t *)malloc(copy_data.buf_len);
        memcpy(copy_data.buf, used_it->first, copy_data.buf_len);
        merge_copy_data.push_back(copy_data);
        *last_used_offset += copy_data.buf_len;
    }
    clear();
    uint32_t offset = sizeof(uint32_t);
    for (vector<merge_copy_data_t>::iterator merge_it = merge_copy_data.begin(); merge_it != merge_copy_data.end(); merge_it++)
    {
        memcpy(mem_pool_buf + offset, merge_it->buf, merge_it->buf_len);
        used_map[mem_pool_buf + offset] = merge_it->buf_len;
        offset += merge_it->buf_len;
        free(merge_it->buf);
    }
    avail_map[mem_pool_buf + offset] = mem_pool_size - offset;
    avail_size_map[mem_pool_size - offset].push_back(mem_pool_buf + offset);
}



