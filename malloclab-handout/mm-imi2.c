/*
 * mm.c
 * Hailun Zhu
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a full description of your solution.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "contracts.h"

#include "mm.h"
#include "memlib.h"


// Create aliases for driver tests
// DO NOT CHANGE THE FOLLOWING!
#ifdef DRIVER
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif

/*
 *  Logging Functions
 *  -----------------
 *  - dbg_printf acts like printf, but will not be run in a release build.
 *  - checkheap acts like mm_checkheap, but prints the line it failed on and
 *    exits if it fails.
 */

#ifndef NDEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#define checkheap(verbose) do {if (mm_checkheap(verbose)) {  \
                             printf("Checkheap failed on line %d\n", __LINE__);\
                             exit(-1);  \
                        }}while(0)
#else
#define dbg_printf(...)
#define checkheap(...)
#endif

/*
 *  Helper functions
 *  ----------------
 */

// Align p to a multiple of w bytes
static inline void* align(const void const* p, unsigned char w) {
    return (void*)(((uintptr_t)(p) + (w-1)) & ~(w-1));
}

// Check if the given pointer is 8-byte aligned
static inline int aligned(const void const* p) {
    return align(p, 8) == p;
}

// Return whether the pointer is in the heap.
static int in_heap(const void* p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}


/*
 *  Block Functions
 *  ---------------
 *  TODO: Add your comment describing block functions here.
 *  The functions below act similar to the macros in the book, but calculate
 *  size in multiples of 4 bytes.
 */

// Return the size of the given block in multiples of the word size
static inline unsigned int block_size(const uint32_t* block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    //return (block[0] & 0x3FFFFFFF);
	return (*block)&(~0x7);

}

// Return true if the block is free, false otherwise
static inline int block_free(const uint32_t* block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    return !(block[0] & 0x40000000);
}

// Mark the given block as free(1)/alloced(0) by marking the header and footer.
static inline void block_mark(uint32_t* block, int free) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    unsigned int next = block_size(block) + 1;
    block[0] = free ? block[0] & (int) 0xBFFFFFFF : block[0] | 0x40000000;
    block[next] = block[0];
}

// Return a pointer to the memory malloc should return
static inline uint32_t* block_mem(uint32_t* const block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));
    REQUIRES(aligned(block + 1));

    return block + 1;
}

// Return the header to the previous block
static inline uint32_t* block_prev(uint32_t* const block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    return block - block_size(block - 1) - 2;
}

// Return the header to the next block
static inline uint32_t* block_next(uint32_t* const block) {
    REQUIRES(block != NULL);
    REQUIRES(in_heap(block));

    return block + block_size(block) + 2;
}


/*
 * If NEXT_FIT defined use next fit search, else use first fit search 
 */
#define NEXT_FITx

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ //line:vm:mm:beginconst
#define DSIZE       8       /* Doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  //line:vm:mm:endconst 

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
static inline size_t PACK(size_t size,size_t alloc){
 return ((size) | (alloc)); //line:vm:mm:pack
}
/* Read and write a word at address p */
static inline unsigned int GET(void *p){
	return  (*(unsigned int *)(p));
}            //line:vm:mm:get
static inline void PUT(void *p, size_t val) {
 (*(unsigned int *)(p) = (val));
}    //line:vm:mm:put

/* Read the size and allocated fields from address p */
static inline size_t GET_SIZE(void *p){
	return  (GET(p) & ~0x7) ;
}                  //line:vm:mm:getsizes
static inline size_t GET_ALLOC(void *p){
	return (GET(p) & 0x1);
}                    //line:vm:mm:getalloc

/* Given block ptr bp, compute address of its header and footer */
static inline void *HDRP(void *bp){
     return (void *)  ((char *)(bp) - WSIZE) ;
}                     //line:vm:mm:hdrp
static inline void *FTRP(void *bp){
return  (void *)((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE);
} //line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
static inline void *NEXT_BLKP(void *bp){
return (void *) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)));
} //line:vm:mm:nextblkp
static inline void *PREV_BLKP(void *bp){
return (void *) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)));
} //line:vm:mm:prevblkp
/* $end mallocmacros */

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  
#ifdef NEXT_FIT
static char *rover;           /* Next fit rover */
#endif

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp); 
//static void checkheap(int verbose);
static void checkblock(void *bp);



/*
 *  Malloc Implementation
 *  ---------------------
 *  The following functions deal with the user-facing malloc implementation.
 */

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
 /* Create the initial empty heap */
if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) //line:vm:mm:begininit
	return -1;
    PUT(heap_listp, 0);                          /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2*WSIZE);                     //line:vm:mm:endinit  
/* $end mminit */

#ifdef NEXT_FIT
    rover = heap_listp;
#endif
/* $begin mminit */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
	return -1;
    return 0;
}

/*
 * malloc
 */
void *malloc (size_t size) {
   size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;      

/* $end mmmalloc */
    if (heap_listp == 0){
	mm_init();
    }
/* $begin mmmalloc */
    /* Ignore spurious requests */
    if (size == 0)
	return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)                                          //line:vm:mm:sizeadjust1
	asize = 2*DSIZE;                                        //line:vm:mm:sizeadjust2
    else
	asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); //line:vm:mm:sizeadjust3

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {  //line:vm:mm:findfitcall
	place(bp, asize);                  //line:vm:mm:findfitplace
	return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);                 //line:vm:mm:growheap1
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)  
	return NULL;                                  //line:vm:mm:growheap2
    place(bp, asize);                                 //line:vm:mm:growheap3
    return bp;
    
}

/*
 * free
 */
void free (void *ptr) {
    /* $end mmfree */
    if(ptr == 0) 
	return;

/* $begin mmfree */
    size_t size = GET_SIZE(HDRP(ptr));
/* $end mmfree */
    if (heap_listp == 0){
	mm_init();
    }
/* $begin mmfree */

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);

}


/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
/* $begin coalesce */
static void* coalesce(void *bp) //bp point to header
{
	 size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
	return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
	size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
	size += GET_SIZE(HDRP(PREV_BLKP(bp)));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
	bp = PREV_BLKP(bp);
    }

    else {                                     /* Case 4 */
	size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
	    GET_SIZE(FTRP(NEXT_BLKP(bp)));
	PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
	PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
	bp = PREV_BLKP(bp);
   }
/* $end mmfree */
#ifdef NEXT_FIT
    /* Make sure the rover isn't pointing into the free block */
    /* that we just coalesced */
    if ((rover > (char *)bp) && (rover < NEXT_BLKP(bp))) 
	rover = bp;
#endif
/* $begin mmfree */
    return bp;
	
}
/* $end coalecse */



/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *ptr, size_t size) {
	size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
	free(ptr);
	return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
	return malloc(size);
    }

    newptr = malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
	return 0;
    }

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(ptr));
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    free(ptr);

    return newptr;
}

/*
 * calloc - you may want to look at mm-naive.c
 */
void *calloc (size_t nmemb, size_t size) {
  size_t bytes = nmemb * size;
  void *newptr;

  newptr = malloc(bytes);
  memset(newptr, 0, bytes);

  return newptr;
}

// Returns 0 if no errors were found, otherwise returns the error
int mm_checkheap(int verbose) {

char *bp = heap_listp;

    if (verbose)
	printf("Heap (%p):\n", heap_listp);

    if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
	printf("Bad prologue header\n");
    checkblock(heap_listp);

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
	if (verbose) 
	    printblock(bp);
	checkblock(bp);
    }

    if (verbose)
	printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
	printf("Bad epilogue header\n");
in_heap(bp);
return 0;
}

/* 
 * The remaining routines are internal helper routines 
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
/* $begin mmextendheap */
static void *extend_heap(size_t words) 
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; //line:vm:mm:beginextend
    if ((long)(bp = mem_sbrk(size)) == -1)  
	return NULL;                                        //line:vm:mm:endextend

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   //line:vm:mm:freeblockhdr
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   //line:vm:mm:freeblockftr
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ //line:vm:mm:newepihdr

    /* Coalesce if the previous block was free */
    return coalesce(bp);                                          //line:vm:mm:returnblock
}
/* $end mmextendheap */

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
/* $begin mmplace */
/* $begin mmplace-proto */
static void place(void *bp, size_t asize)
     /* $end mmplace-proto */
{
    size_t csize = GET_SIZE(HDRP(bp));   

    if ((csize - asize) >= (2*DSIZE)) { 
	PUT(HDRP(bp), PACK(asize, 1));
	PUT(FTRP(bp), PACK(asize, 1));
	bp = NEXT_BLKP(bp);
	PUT(HDRP(bp), PACK(csize-asize, 0));
	PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else { 
	PUT(HDRP(bp), PACK(csize, 1));
	PUT(FTRP(bp), PACK(csize, 1));
    }
}
/* $end mmplace */

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
/* $begin mmfirstfit */
/* $begin mmfirstfit-proto */
static void *find_fit(size_t asize)
/* $end mmfirstfit-proto */
{
/* $end mmfirstfit */

#ifdef NEXT_FIT 
    /* Next fit search */
    char *oldrover = rover;

    /* Search from the rover to the end of list */
    for ( ; GET_SIZE(HDRP(rover)) > 0; rover = NEXT_BLKP(rover))
	if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover))))
	    return rover;

    /* search from start of list to old rover */
    for (rover = heap_listp; rover < oldrover; rover = NEXT_BLKP(rover))
	if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover))))
	    return rover;

    return NULL;  /* no fit found */
#else 
/* $begin mmfirstfit */
    /* First fit search */
    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
	if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
	    return bp;
	}
    }
    return NULL; /* No fit */
/* $end mmfirstfit */
#endif
}

static void printblock(void *bp) 
{
    size_t hsize, halloc, fsize, falloc;

    checkheap(0);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));  
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));  

    if (hsize == 0) {
	printf("%p: EOL\n", bp);
	return;
    }

    /*  printf("%p: header: [%p:%c] footer: [%p:%c]\n", bp, 
	hsize, (halloc ? 'a' : 'f'), 
	fsize, (falloc ? 'a' : 'f')); */
}

static void checkblock(void *bp) 
{
    if ((size_t)bp % 8)
	printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
	printf("Error: header does not match footer\n");
}

/* 
 * checkheap - Minimal check of the heap for consistency 
 */
/*void checkheap(int verbose) 
{
    char *bp = heap_listp;

    if (verbose)
	printf("Heap (%p):\n", heap_listp);

    if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
	printf("Bad prologue header\n");
    checkblock(heap_listp);

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
	if (verbose) 
	    printblock(bp);
	checkblock(bp);
    }

*/

