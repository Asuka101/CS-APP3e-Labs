#include "cachelab.h"
#include "getopt.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

typedef struct cache_line
{
    int valid;
    int tag;
    int counter;
}CacheLine;
CacheLine *cache;
int hit_count = 0;
int miss_count = 0;
int eviction_count = 0;
char location[100];
void initCache(int s, int E);
void replayTrace(int s, int E, int b);
void accessData(int index, int target_tag, int E);

/*initiate the cache*/
void initCache(int s, int E)
{
    int production = (1 << s) * E;
    cache = malloc(production* sizeof(CacheLine));
    for (int i = 0; i < production; i++) {
        cache[i].valid = 0;
        cache[i].tag = -1;
        cache[i].counter = 0;
    }
}

void replayTrace(int s, int E, int b)
{
    FILE* pFile;
    pFile = fopen(location, "r");
    char identifier;
    unsigned long address;
    int size;
    while (fscanf(pFile, " %c %lx,%d", &identifier, &address, &size) > 0) {
        if (identifier == 'I') continue;
        /*divide the address(64bits) into three parts to decide the target tag and index of set*/
        unsigned long long_target_tag = address >> (s + b);
        unsigned long long_index = (address >> b) ^ (long_target_tag << s);
        int target_tag = (int)long_target_tag;
        int index = (int)long_index;
        accessData(index, target_tag, E);
        if (identifier == 'M') hit_count++;
    }
    free(cache);
    fclose(pFile);
}

void accessData(int index, int target_tag, int E)
{
    int start = index * E;
    int end = start + E;
    int judge = 0;
    int evict = start;
    for (int i = start; i < end; i++)
    {
        cache[i].counter++;
        /*miss without eviction*/
        if (!cache[i].valid)
        {
            if (!judge) 
            {
                cache[i].valid = 1;
                cache[i].tag = target_tag;
                judge = 1;
                miss_count++;
            }
            cache[i].counter = 0;
            break;
        }
        /*hit*/
        if (cache[i].tag == target_tag) {
            cache[i].counter = 0;
            judge = 1;
            hit_count++;
        }
        evict = cache[i].counter > cache[evict].counter ? i : evict;
    }
    /*miss with eviction*/
    if (!judge)
    {
        cache[evict].tag = target_tag;
        cache[evict].counter = 0;
        miss_count++;
        eviction_count++;
    }
}

int main(int argc, char** argv)
{
    char opt;
    int s, E, b;
    /*get paramaters from command line by getopt*/
    while(-1 != (opt = getopt(argc, argv, "hvs:E:b:t:"))) {
        switch(opt) {
            case 'h':
                exit(0);
            case 'v':
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                strcpy(location, optarg);
                break;
            default:
                printf("wrong argument\n");
                exit(-1);
        }
    }
    /*initiate the cache simulator*/
    initCache(s, E);
    /*read command data and do operations on the cache*/
    replayTrace(s, E, b);
    /*print the output*/
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}