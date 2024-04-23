/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Mars",
    /* First member's full name */
    "Gavius Octavius",
    /* First member's email address */
    "eric6329@my.swjtu.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};
static char *heap_listp;

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

/* basic constants */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)
#define CLASSNUM 20
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* read and write a word at address p */
#define GET(p)      (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (unsigned int)(val))

/* read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* given block ptr bp, compute address of its next and previous blocks */
#define NEXT_BLKP(bp) ((char *)bp + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)bp - GET_SIZE(((char *)(bp) - DSIZE)))

/* given block ptr bp, compute address of address of its children*/
#define LEFT(bp)    (unsigned int *)((char *)(bp))
#define RIGHT(bp)   (unsigned int *)((char *)(bp) + WSIZE)
#define PARENT(bp)  (unsigned int *)((char *)(bp) + DSIZE)

/* given block ptr bp, read address of its children */
#define GET_LEFT(bp)    ((unsigned int *)(GET(LEFT(bp))))
#define GET_RIGHT(bp)   ((unsigned int *)(GET(RIGHT(bp))))
#define GET_PARENT(bp)   ((unsigned int *)(GET(PARENT(bp))))

/* given entry index, compute adress of the entry*/
#define GET_ENTRY(index)    (heap_listp + WSIZE * index)

#define CHECK 

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    unsigned int index;
    /* create the initial empty heap */
    if ((heap_listp = mem_sbrk(WSIZE * (4 + CLASSNUM))) == (void *)-1) 
        return -1;
    
    PUT(heap_listp, 0);
    PUT(heap_listp + WSIZE, PACK((DSIZE + CLASSNUM * WSIZE), 1));
    
    /* initialize empty binary search trees */
    heap_listp += DSIZE;
    for (index = 0; index < CLASSNUM; index++)
        PUT(GET_ENTRY(index), NULL);

    PUT(heap_listp + CLASSNUM * WSIZE, PACK((DSIZE + CLASSNUM * WSIZE), 1));
    PUT(heap_listp + (1 + CLASSNUM) * WSIZE, PACK(0, 1));
    

    /* extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * extend_heap - extend the heap with a new free block.
 */
void *extend_heap(size_t words)
{
    char *ptr;
    size_t size;

    /* allocate an even number to maintain alignment */ 
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(ptr = mem_sbrk(size)) == -1)
        return NULL;
    
    /* initialize free block header/footer and the epilogue header */
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));

    /* coalesce if the previous block or the next block is free */
    return coalesce(ptr);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t extendsize;
    size_t newsize;
    void *ptr;
    if (size == 0)
        return NULL;
    
    /* because a free block need one more word to store the address of its parent the minimum size of a block is 6 words(24bytes) */
    newsize = (size <= (2 * DSIZE)) ? (3 * DSIZE) : ALIGN(size + DSIZE);
    
    if ((ptr = find_fit(newsize)) != NULL) {
        place(ptr, newsize);
        return ptr;
    }
    extendsize = MAX(newsize, CHUNKSIZE);
    if ((ptr = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(ptr, newsize);
    CHECK
    return ptr;
}

/*
 * find_fit - find a free block fit for new block
 */
void *find_fit(size_t asize)
{
    void *entry = find_entry(asize);
    void *boundary = GET_ENTRY(CLASSNUM);
    void *ptr;
    void *best = NULL;
    
    while (entry < boundary) {
        ptr = (void *)(GET(entry));
        
        while (ptr != NULL) {
            /* case 1: current block size is larger than or equal to asize */
            if (GET_SIZE(HDRP(ptr)) >= asize) {
                best = ptr;
                ptr = GET_LEFT(ptr);
            }
            
            /* case 2: current block size is smaller than asize */
            else
                ptr = GET_RIGHT(ptr);
        }
        if (best != NULL) 
            break;
        entry += WSIZE;
    }
    
    return best;
}

/*
 * place - split the free block into an allocated block and a new free block
 */
void place(void *ptr, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(ptr)); 

    delete_node(ptr);

    if ((csize - asize) >= 3 * DSIZE) {
        PUT(HDRP(ptr), PACK(asize, 1)); 
        PUT(FTRP(ptr), PACK(asize, 1));

        ptr = NEXT_BLKP(ptr); 
        PUT(HDRP(ptr), PACK(csize-asize, 0)); 
        PUT(FTRP(ptr), PACK(csize-asize, 0));

        add_node(ptr); 
    } 
    
    else { 
        PUT(HDRP(ptr), PACK(csize, 1)); 
        PUT(FTRP(ptr), PACK(csize, 1)); 
    }
}

/*
 * delete_node - delete allocated block in the tree
 */
void delete_node(void *ptr)
{
    void *entry;
    void *left_max;

    entry = find_entry(GET_SIZE(HDRP(ptr)));
    
    /* case 1: no left child */
    if (GET_LEFT(ptr) == NULL) {
        /* root */
        if (GET_PARENT(ptr) == NULL) {
            PUT(entry, GET_RIGHT(ptr));
            if (GET_RIGHT(ptr) != NULL)
                PUT(PARENT(GET_RIGHT(ptr)), NULL);
            return;
        }
        
        /* not root */
        else {           
            if (GET_LEFT(GET_PARENT(ptr)) == ptr) 
                PUT(LEFT(GET_PARENT(ptr)), GET_RIGHT(ptr));
            else 
                PUT(RIGHT(GET_PARENT(ptr)), GET_RIGHT(ptr));
            if (GET_RIGHT(ptr) != NULL)
                PUT(PARENT(GET_RIGHT(ptr)), GET_PARENT(ptr));
            return;
        }
    }

    /* case 2: have a left child*/
    else {
        left_max = GET_LEFT(ptr);
        
        /* case 2.1: the left child is the left_max node */
        if (GET_RIGHT(left_max) == NULL) {
            PUT(RIGHT(left_max), GET_RIGHT(ptr));
            if (GET_RIGHT(ptr) != NULL) 
                PUT(PARENT(GET_RIGHT(ptr)), left_max);

            /* root */
            if (GET_PARENT(ptr) == NULL) {
                PUT(entry, left_max);
                PUT(PARENT(left_max), NULL);
                return;
            }

            /* not root */
            else {
                if (GET_LEFT(GET_PARENT(ptr)) == ptr)
                    PUT(LEFT(GET_PARENT(ptr)), left_max);
                else
                    PUT(RIGHT(GET_PARENT(ptr)), left_max);
                PUT(PARENT(left_max), GET_PARENT(ptr));
                return;
            }
        }

        /* case 2.2: need to search for the left_max node */
        else {
            /* search */
            while (left_max != NULL) {
                if (GET_RIGHT(left_max) == NULL)
                    break;
                left_max = GET_RIGHT(left_max);
            }
            
            /* move the left_max node to the position of ptr node in the tree */
            PUT(RIGHT(left_max), GET_RIGHT(ptr));
            if (GET_RIGHT(ptr) != NULL)
                PUT(PARENT(GET_RIGHT(ptr)), left_max);
            PUT(RIGHT(GET_PARENT(left_max)), GET_LEFT(left_max));
            if (GET_LEFT(left_max) != NULL)
                PUT(PARENT(GET_LEFT(left_max)), GET_PARENT(left_max));     
            PUT(LEFT(left_max), GET_LEFT(ptr));
            PUT(PARENT(GET_LEFT(ptr)), left_max);

            /* root */
            if (GET_PARENT(ptr) == NULL) {
                PUT(entry, left_max);
                PUT(PARENT(left_max), NULL);
                return;
            }

            /* not root */
            else {
                if (GET_LEFT(GET_PARENT(ptr)) == ptr)
                    PUT(LEFT(GET_PARENT(ptr)), left_max);
                else
                    PUT(RIGHT(GET_PARENT(ptr)), left_max);
                PUT(PARENT(left_max), GET_PARENT(ptr));
                return;
            }
        }
    }
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == 0) 
        return;
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
    CHECK
}

/*
 * coalesce - merge free blocks
 */
void *coalesce(void *ptr)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));

    if (prev_alloc && next_alloc) {
        add_node(ptr);
        return ptr;
    }
    
    else if (!prev_alloc && next_alloc) {
        delete_node(PREV_BLKP(ptr));

        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }

    else if (prev_alloc && !next_alloc) {
        delete_node(NEXT_BLKP(ptr));

        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    }

    else {
        delete_node(PREV_BLKP(ptr));
        delete_node(NEXT_BLKP(ptr));

        size += (GET_SIZE(HDRP(PREV_BLKP(ptr)))) + (GET_SIZE(FTRP(NEXT_BLKP(ptr))));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }

    add_node(ptr);
    
    return ptr;
}

/*
 * add_node - add the new free block to the appropriate tree
 */
void add_node(void *ptr)
{
    void *entry;
    void *parent;
    size_t size;
    
    /* set the children and the parent NULL */
    PUT(LEFT(ptr), NULL);
    PUT(RIGHT(ptr), NULL);
    PUT(PARENT(ptr), NULL);

    /* get the entry of the appropriate tree */
    size = GET_SIZE(HDRP(ptr));
    entry = find_entry(size);
    parent = (void *)(GET(entry));

    /* the tree is empty */
    if (parent == NULL) {
        PUT(entry, ptr);
        return;
    }
    
    /* add the node */
    while(parent != NULL) {
        /* case 1: size of the current node is smaller than or equal to our node */
        if ((GET_SIZE(HDRP(parent))) <= size) {
            if ((GET_RIGHT(parent)) == NULL) {
                PUT(RIGHT(parent), ptr);
                PUT(PARENT(ptr), parent);
                return;
            }
            else
                parent = GET_RIGHT(parent);
        }
        
        /* case 2: size of the current node is larger than our node */
        else {
            if ((GET_LEFT(parent)) == NULL) {
                PUT(LEFT(parent), ptr);
                PUT(PARENT(ptr), parent);
                return;
            }
            else
                parent = GET_LEFT(parent);
        }
    }
}

/*
 * find_entry - find out the appropriate entry
 */
void *find_entry(size_t size)
{
    unsigned int index;
    for (index = 4; index <= 23; index++) {
        if ((size >> index) == 1)
            return GET_ENTRY((index - 4));
    }
    return GET_ENTRY((index - 4));
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    if (size == GET_SIZE(HDRP(ptr)))
        return ptr;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    
    copySize = GET_SIZE(HDRP(ptr)) - DSIZE;
    if (size < copySize)
        copySize = size;
    
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}