/*
 * details of methods of cache
 */
#include "cache.h"

/* initialize the cache with defined number of cachelines */
void cache_init(Cache *cache, int num)
{
    int i;

    cache->cacheline = malloc(num * sizeof(Cacheline));
    cache->num = num;
    Sem_init(&cache->mutex, 0, 1);
    Sem_init(&cache->w, 0, 1);

    for (i = 0; i < num; i++) {
        cache->cacheline[i].valid = 0;
        cache->cacheline[i].counter = 0;
    }
}

/* try to fetch object cached in the cache memory */
char *cache_fetch(Cache *cache, char *uri, int fd)
{
    int i;
    char *object = NULL;
    
    /* Reader and Writer Model with Reader Priority */
    P(&cache->mutex);
    (cache->readcnt)++;     // protect the global variable cache->readcnt
    if (cache->readcnt == 1)
        P(&cache->w);
    V(&cache->mutex);

    /* 
     * because of the special implementation of cache each reading operation is along with a writing operation on the cacheline counter(a global variable)
     * which means getting cache mutex when staring the tranversing of the cache and releasing the mutex after the end is necessary
     */
    P(&cache->mutex);
    for (i = 0; i < cache->num; i++) {
        cache->cacheline[i].counter++;  // each iteration needs to increment the cachline counter by 1 to maintain the order of cacheline

        /* send the object to the client if the target object is found out */
        if (cache->cacheline[i].valid && !strcmp(cache->cacheline[i].uri, uri)) {
            cache->cacheline[i].counter = 0;        // make the target cacheline the most recently used cachline
            object = cache->cacheline[i].object;
            Rio_writen(fd, object, strlen(object)); // send the object to the client
        }
    }
    V(&cache->mutex);

    P(&cache->mutex);
    (cache->readcnt)--;     // protect as mentioned
    if (cache->readcnt == 0)
        V(&cache->w);
    V(&cache->mutex);

    return object;
}

void object_insert(Cache *cache, char *uri, char *buf)
{
    int i;
    Cacheline *target;
    
    /* wait until the writer mutex is released and get the mutex again */
    P(&cache->w);
    
    target = &(cache->cacheline[0]);
    for (i = 0; i < cache->num; i++) {
        cache->cacheline[i].counter++;
        /* if cache has free memory for caching objects then break the loop */
        if (!cache->cacheline[i].valid) {
            target = &(cache->cacheline[i]);
            break;
        }
        
        /* keep iterating to find out the cacheline with the largest counter which is the least recently used one */
        if (cache->cacheline[i].counter > target->counter)
            target = &(cache->cacheline[i]);
    }

    /* initialize the cacheline which will be used to cache the new object */
    target->valid = 1;
    target->counter = 0;
    strcpy(target->uri, uri);
    strcpy(target->object, buf);
    
    /* release the writer mutex */
    V(&cache->w);
}