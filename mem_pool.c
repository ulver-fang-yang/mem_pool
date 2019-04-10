
#include <stdio.h>
#include <stdlib.h>

#include "mem_pool.h"

//#define HH_MEMPOOL_TEST
//#define MULTI_THREAD

#ifdef HH_MEMPOOL_TEST
#include <sys/time.h>
#endif

int hh_mempool_init(uint32_t size)
{
    pthread_mutex_init(&g_mempool_mutex, NULL); 
    do
    {
        pthread_mutex_lock(&g_mempool_mutex);
        g_mem_pool.buffer = malloc(size);
        if (NULL == g_mem_pool.buffer)
            break;

        g_mem_pool.aval_root = (mem_object_t *)malloc(sizeof(mem_object_t));
        if (NULL == g_mem_pool.aval_root)
            break;

/*
        g_mem_pool.used_root = (mem_object_t *)malloc(sizeof(mem_object_t));
        if (NULL == g_mem_pool.used_root)
            break;*/
    
        g_mem_pool.len = size;
        g_mem_pool.aval_root->ptr = g_mem_pool.buffer;
        g_mem_pool.aval_root->len = size;
        g_mem_pool.aval_root->next = NULL;
/*
        g_mem_pool.used_root->ptr = NULL;
        g_mem_pool.used_root->len = 0;
        g_mem_pool.used_root->next = NULL;*/

        pthread_mutex_unlock(&g_mempool_mutex);

        return 0;
    } while (0);
    if (g_mem_pool.buffer)
    {
        if (g_mem_pool.aval_root)
            free(g_mem_pool.aval_root);

/*
        if (g_mem_pool.used_root)
            free(g_mem_pool.used_root);*/

        free(g_mem_pool.buffer);
        g_mem_pool.len = 0;
    }
    pthread_mutex_unlock(&g_mempool_mutex);

    return -1;
}

/*
void used_list_update(mem_object_t *used_ptr)
{
    mem_object_t *ptr = g_mem_pool.used_root;

    if (NULL == g_mem_pool.used_root)
        return;

    if (NULL == used_ptr)
        return;

    while (ptr->next != NULL)
        ptr = ptr->next;

    ptr->
}*/

mem_object_t *update_mempool(uint32_t size)
{
    if (NULL == g_mem_pool.buffer)
        return NULL;

    if (NULL == g_mem_pool.aval_root)
        return NULL;

    FILE *back_mem_file = fopen("/system/tmp/hh_mempool", "a+");
        return NULL;

    fwrite(g_mem_pool.buffer, g_mem_pool.len, 1, back_mem_file);
    mem_object_t *used_ptr = g_mem_pool.used_root, *aval_ptr;
    if (NULL == used_ptr)
        //  如果调用此函数，而可用的list为空，则说明内存池出问题了，直接返回NULL
        return NULL;

    uint8_t *buf = g_mem_pool.buffer;
    uint32_t len = 0;
    while (used_ptr->next != NULL)
    {
        fseek(back_mem_file, used_ptr->ptr - g_mem_pool.buffer, SEEK_SET);
        fread(buf + len, used_ptr->len, 1, back_mem_file);
        used_ptr->ptr = buf + len;
        len += used_ptr->len;
        used_ptr = used_ptr->next;
    }

    while (aval_ptr->next != NULL)
    {
        mem_object_t *ptr = aval_ptr->next;
        free(aval_ptr);
        aval_ptr = ptr;
    }
    aval_ptr->ptr = buf + len;
    aval_ptr->len = g_mem_pool.len - len;
    aval_ptr->next = NULL;

    if (aval_ptr->len >= size)
        return used_ptr;

    return NULL;
}

void merge_list(mem_object_t *ptr)
{
    if ((ptr->next != NULL) && (ptr->next->ptr == ptr->ptr + ptr->len))
    {
        merge_list(ptr->next);
        ptr->next = ptr->next->next;
        ptr->len += ptr->next->len;
        free(ptr->next);
    }
}

mem_object_t *merge_mem(uint32_t size)
{
    mem_object_t *aval_ptr = g_mem_pool.aval_root;

    if (NULL == aval_ptr)
        return NULL;

    while (aval_ptr != NULL)
    {
        merge_list(aval_ptr);
        if (aval_ptr->len >= size)
            return aval_ptr;
        aval_ptr = aval_ptr->next;
    }

    return NULL;
}

mem_object_t *real_malloc(mem_object_t *aval_ptr, uint32_t size)
{
    mem_object_t *used_ptr;
    mem_object_t *ret_ptr;
    ret_ptr = (mem_object_t *)malloc(sizeof(mem_object_t));
    if (NULL == ret_ptr)
        return NULL;
    ret_ptr->ptr = aval_ptr->ptr;
    ret_ptr->len = size;
    ret_ptr->next = NULL;

    aval_ptr->ptr += size;
    aval_ptr->len -= size;

//    used_list_update();
    if (NULL == g_mem_pool.used_root)
        g_mem_pool.used_root = ret_ptr;
    else
    {
        used_ptr = g_mem_pool.used_root;
        while (NULL != used_ptr->next)
            used_ptr = used_ptr->next;

        used_ptr->next = ret_ptr;                
    }
    return ret_ptr;
}

mem_object_t *hh_mempool_malloc(uint32_t size)
{
    mem_object_t *aval_ptr = g_mem_pool.aval_root, *used_ptr;
    mem_object_t *ret_ptr;

    pthread_mutex_lock(&g_mempool_mutex);
    if (NULL == g_mem_pool.aval_root)
    {
        pthread_mutex_unlock(&g_mempool_mutex);
        return NULL;
    }

    while (aval_ptr->next != NULL)
    {
        if (aval_ptr->len >= size)
        {
            ret_ptr = real_malloc(aval_ptr, size);
            pthread_mutex_unlock(&g_mempool_mutex);
            return ret_ptr;
        }
        aval_ptr = aval_ptr->next;
    }
    if (aval_ptr->len >= size)
    {
        ret_ptr = real_malloc(aval_ptr, size);
        pthread_mutex_unlock(&g_mempool_mutex);
        return ret_ptr;
    }

#ifdef NEED_UPDATE
    used_ptr = update_mempool(size);
    if (NULL == used_ptr)
    {
        pthread_mutex_unlock(&g_mempool_mutex);
        return NULL;
    }

    aval_ptr = g_mem_pool.aval_root;
    ret_ptr->ptr = aval_ptr->ptr;
    ret_ptr->len = size;
    ret_ptr->next = NULL;

    aval_ptr->ptr += size;
    aval_ptr->len -= size;

    used_ptr->next = ret_ptr;

    return ret_ptr;
#else
    aval_ptr = merge_mem(size);
    if (NULL == aval_ptr)
    {
        pthread_mutex_unlock(&g_mempool_mutex);
        return NULL;
    }

    ret_ptr = (mem_object_t *)malloc(sizeof(mem_object_t));
    if (NULL == ret_ptr)
    {
        pthread_mutex_unlock(&g_mempool_mutex);
        return NULL;
    }
    ret_ptr->ptr = aval_ptr->ptr;
    ret_ptr->len = size;
    ret_ptr->next = NULL;

    aval_ptr->ptr += size;
    aval_ptr->len -= size;

    used_ptr = g_mem_pool.used_root;
    while (NULL != used_ptr->next)
        used_ptr = used_ptr->next;
    used_ptr->next = ret_ptr;

    pthread_mutex_unlock(&g_mempool_mutex);
    return ret_ptr;
#endif
}

int hh_mempool_free(mem_object_t *ptr)
{
    mem_object_t *used_ptr = g_mem_pool.used_root;

    pthread_mutex_lock(&g_mempool_mutex);

    if (used_ptr == NULL)
    {
        pthread_mutex_unlock(&g_mempool_mutex);
        return -1;
    }

    if (used_ptr == ptr)
    {
        mem_object_t *aval_ptr = g_mem_pool.aval_root;
        if (NULL == aval_ptr)
        {
            pthread_mutex_unlock(&g_mempool_mutex);
            return -1;
        }
        while (aval_ptr->next != NULL)
            aval_ptr = aval_ptr->next;
        aval_ptr->next = ptr;
        g_mem_pool.used_root = used_ptr->next;
        ptr->next = NULL;
        pthread_mutex_unlock(&g_mempool_mutex);
        return 0;
    }

    while (used_ptr->next != NULL)
    {
        if (used_ptr->next == ptr)
        {
            mem_object_t *aval_ptr = g_mem_pool.aval_root;
            if (NULL == aval_ptr)
            {
                pthread_mutex_unlock(&g_mempool_mutex);
                return -1;
            }
            while (aval_ptr->next != NULL)
                aval_ptr = aval_ptr->next;
            aval_ptr->next = ptr;
            used_ptr->next = used_ptr->next->next;
            ptr->next = NULL;
            pthread_mutex_unlock(&g_mempool_mutex);
            return 0;
        }
        used_ptr = used_ptr->next;
    }

    pthread_mutex_unlock(&g_mempool_mutex);
    return -1;
}

void hh_mempool_exit()
{
    if (g_mem_pool.buffer)
    {
        if (g_mem_pool.aval_root)
            free(g_mem_pool.aval_root);

        free(g_mem_pool.buffer);
        g_mem_pool.len = 0;
    }

    pthread_mutex_destroy(&g_mempool_mutex);
}

#ifdef HH_MEMPOOL_TEST

void hh_mempool_test_print(int print_detail)
{
    if (NULL == g_mem_pool.buffer)
    {
        printf("g_mem_pool.buffer null, had not init?\n");
        return;
    }

    if (NULL == g_mem_pool.aval_root)
    {
        printf("g_mem_pool.aval_root null, wrong mempool!\n");
        return;
    }

    printf("-------------------------------------------\n");
    printf("              hh_mempool_test_print        \n");
    printf("\n");
    printf("g_mem_pool.buffer: %08x\n", g_mem_pool.buffer);
    printf("g_mem_pool.len: %u\n", g_mem_pool.len);
    printf("\n");

    printf("g_mem_pool.aval_root:\n");
    printf("\n");
    mem_object_t *ptr = g_mem_pool.aval_root;
    int i = 0;
    if (print_detail)
{
    while (ptr != NULL)
    {
        printf("aval_ptr[%d], ptr: %08x, len: %u\n", i, ptr->ptr, ptr->len);
        ptr = ptr->next;
        i++;
//        if (i > 200)
//            break;
    }
}
    printf("\n");

    printf("g_mem_pool.used_root:\n");
    printf("\n");
    ptr = g_mem_pool.used_root;
    i = 0;
    if (print_detail)
{
    while (ptr != NULL)
    {
        printf("used_ptr[%d], ptr: %08x, len: %u\n", i, ptr->ptr, ptr->len);
        ptr = ptr->next;
        i++;
//        if (i > 200)
//            break;
    }
}
    printf("\n");
    printf("-------------------------------------------\n");
}

typedef struct test_mmo_list_s
{
    mem_object_t *ptr;
    struct test_mmo_list_s*next;
} test_mmo_list_t;

void *malloc_thread(void *arg)
{
    test_mmo_list_t *mmo_list = (test_mmo_list_t *)arg;
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    srand(tv.tv_usec);
    int n = (1 + (int) (30000.0*rand() / (RAND_MAX + 1.0)));
    mem_object_t *ptr = hh_mempool_malloc(n);

    int i;
    for (i = 0; i < 1000; i++)
    {
        if (mmo_list[i].ptr == NULL)
        {
            mmo_list[i].ptr = ptr;
            return NULL;
        }
    }

    return NULL;
}

void *free_thread(void *arg)
{
    test_mmo_list_t *mmo_list = (test_mmo_list_t *)arg;
    int i;
    for (i = 0; i < 1000; i++)
        hh_mempool_free(mmo_list[i].ptr);

    return NULL;
}

int main(int argc, char *argv[])
{
#ifdef MULTI_THREAD
    int i;
    pthread_t test_threads[2000];
    test_mmo_list_t mmo_list[1000];

    for (i = 0; i < 1000; i++)
        memset(&mmo_list[i], 0, sizeof(test_mmo_list_t));

    if (hh_mempool_init(1000 * 1000 * 1000))
    {
        printf("hh_mempool_init fail\n");
        return -1;
    }

    for (i = 0; i < 1000; i++)
    {
        pthread_create(&(test_threads[i]), NULL, malloc_thread, (void*)mmo_list);
//        usleep(10 * 1000);
    }

    hh_mempool_test_print(1);

    for (i = 0; i < 1000; i++)
    {
        pthread_create(&(test_threads[1000 + i]), NULL, free_thread, (void*)mmo_list);
//        usleep(10 * 1000);
    }

    hh_mempool_test_print(1);

    for (i = 0; i < 2000; i++)
        pthread_join(test_threads[i], NULL);
//    pthread_join(test_threads[999], NULL);
//    pthread_join(test_threads[1999], NULL);
    hh_mempool_test_print(0);

    hh_mempool_exit();
    hh_mempool_test_print(1);

#else
    if (hh_mempool_init(1000 * 1000 * 1000))
    {
        printf("hh_mempool_init fail\n");
        return -1;
    }

    mem_object_t *o1, *o2, *o3;

    o1 = hh_mempool_malloc(30000);
    o2 = hh_mempool_malloc(50000);
    o3 = hh_mempool_malloc(20000);

    hh_mempool_test_print(1);

    hh_mempool_free(o2);
    hh_mempool_free(o3);
    hh_mempool_free(o1);

    hh_mempool_test_print(1);


    hh_mempool_exit();
#endif

    return 0;
}

#endif

