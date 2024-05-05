#include "cache.h"

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

char *cache_fetch(Cache *cache, char *uri, int fd)
{
    int i;
    char *object = NULL;
    
    P(&cache->mutex);
    (cache->readcnt)++;
    if (cache->readcnt == 1)
        P(&cache->w);
    V(&cache->mutex);

    P(&cache->mutex);
    for (i = 0; i < cache->num; i++) {
        cache->cacheline[i].counter++;
        if (cache->cacheline[i].valid && !strcmp(cache->cacheline[i].uri, uri)) {
            cache->cacheline[i].counter = 0;
            object = cache->cacheline[i].object;
            Rio_writen(fd, object, strlen(object));
        }
    }
    V(&cache->mutex);

    P(&cache->mutex);
    (cache->readcnt)--;
    if (cache->readcnt == 0)
        V(&cache->w);
    V(&cache->mutex);

    return object;
}

void object_insert(Cache *cache, char *uri, char *buf)
{
    int i;
    Cacheline *target;
    

    P(&cache->w);
    
    target = &(cache->cacheline[0]);
    for (i = 0; i < cache->num; i++) {
        cache->cacheline[i].counter++;
        if (!cache->cacheline[i].valid) {
            target = &(cache->cacheline[i]);
            break;
        }
        if (cache->cacheline[i].counter > target->counter)
            target = &(cache->cacheline[i]);
    }

    target->valid = 1;
    target->counter = 0;
    strcpy(target->uri, uri);
    strcpy(target->object, buf);
    
    V(&cache->w);
}