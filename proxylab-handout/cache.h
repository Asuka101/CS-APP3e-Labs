#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cache_line {
    int valid;
    int counter;
    char uri[MAXLINE];
    char object[MAX_OBJECT_SIZE];
}Cacheline;

typedef struct cache {
    Cacheline *cacheline;
    int num;
    int readcnt;
    sem_t mutex, w;
}Cache;

void cache_init(Cache *cache, int num);
char *cache_fetch(Cache *cache, char *uri, int fd);
void object_insert(Cache *cache, char *uri, char *object_buf);