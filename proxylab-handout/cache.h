/*
 * data structures and methods of cache
 */
#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* one single cacheline */
typedef struct cache_line {
    int valid;
    int counter;    // a number used to mantain the order of cachelines
    char uri[MAXLINE];
    char object[MAX_OBJECT_SIZE];
}Cacheline;

/* cache memory structure */
typedef struct cache {
    Cacheline *cacheline;   // cachelines
    int num;                // number of cachelines
    int readcnt;            // number of readers
    sem_t mutex, w;         // cache mutex and writer mutex
}Cache;

void cache_init(Cache *cache, int num);
char *cache_fetch(Cache *cache, char *uri, int fd);
void object_insert(Cache *cache, char *uri, char *object_buf);