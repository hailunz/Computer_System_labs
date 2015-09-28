/*
 * mm.c
 * Hailun Zhu
 *
 *im2ex: explicit list+ first fit
 * 
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

// Return the size of the given block in bytes
static inline unsigned int block_size(const uint32_t* block) {
    return (*block)&(0xfffffffe);
}

// Return true if the block is free, false otherwise
static inline int block_free(const uint32_t* block) {
    return (*block)&1;
}

// Mark the given block as free(1)/alloced(0) by marking the header and footer.
static inline void block_mark(uint32_t* block, int free) {
    unsigned size=*block;
    *block|=free;
    void* footer=(char*)block+size-4;
    *((uint32_t*)footer)=size|free;
}

// Return a pointer to the memory malloc should return
static inline uint32_t* block_mem(uint32_t* const block) {
    return block+1;
}

// Return the header to the previous block
static inline uint32_t* block_prev(uint32_t* const block) {
    char* ptr=(char*)block;
    uint32_t* footer=block-1;
    return (uint32_t*)(ptr-block_size(footer));
}

// Return the header to the next block
static inline uint32_t* block_next(uint32_t* const block) {
    char* ptr=(char*)block;
    //uint32_t test=*block;
    size_t size=block_size(block);
    return (uint32_t*)(ptr+size);
}


/*
 *  Malloc Implementation
 *  ---------------------
 *  The following functions deal with the user-facing malloc implementation.
 */

/* point to the block next to the prologue block */
static void* heap_bottom;
/* point to the epilogue block */
static void* heap_top;
/* base address for offset in freelist */
static void* base_address;

/* extract next offset from a block */
static inline uint32_t extract_next_offset(void* block){
    uint32_t offset=*((uint32_t*)block+2);
    return  offset;
}

/* extract prev offset from a block */
static inline uint32_t extract_prev_offset(void* block){
    uint32_t offset=*((uint32_t*)block+1);
    return  offset;
}


/* get address offset relative to heap base */
static inline uint32_t get_offset(void* ptr){
    return (uint32_t)((char *)ptr-(char *)base_address);
}

/* convert offset to absolute address */
static inline void* get_address(uint32_t offset){
    return (void*)((char*)base_address+offset);
}

static inline void set_prev_offset(void* block
                                   ,uint32_t offset){
    *((uint32_t*)block+1)=offset;
}

static inline void set_next_offset(void* block
                                   ,uint32_t offset){
    *((uint32_t*)block+2)=offset;
}

static inline void* next_free_block(void* block){
    uint32_t next_offset=extract_next_offset(block);
    return get_address(next_offset);
}

static inline void* prev_free_block(void* block){
    uint32_t prev_offset=extract_prev_offset(block);
    return get_address(prev_offset);
}

/* round size up to 8 */
static inline size_t resize(size_t size){
    size_t new_size=(size+7)&(0xfffffff8);
    return new_size;
}

/* first fit, return NULL if no fit */
static void* find_fit(size_t size){
    uint32_t* block=next_free_block(base_address);
    while (block!=base_address) {
        if (block_size(block)>=size) {
            return block;
        }
        
        block=next_free_block(block);
    }
    
    return NULL;
}

static void update_free_list(void* block,uint32_t prev_offset
                      ,uint32_t next_offset){
    /*discard this block if it's size<=8*/
    if (block==NULL||block_size(block)<=8) {
        set_next_offset(get_address(prev_offset)
                            , next_offset);
        if (next_offset!=0) {
            set_prev_offset(get_address(next_offset)
                            , prev_offset);
        }
    }
    else{
        uint32_t block_offset=get_offset(block);
        set_next_offset(block, next_offset);
        set_prev_offset(block, prev_offset);
        set_next_offset(get_address(prev_offset)
                        , block_offset);
        if (next_offset) {
            set_prev_offset(get_address(next_offset)
                            , block_offset);
        }
    in_heap(block);}
}

static void* make_block(void* block,size_t size,int free){
    *((uint32_t*)block)=(uint32_t)size;
    block_mark(block, free);
    return block;
}

/*split*/
static void split(void* block,size_t size){
    size_t old_size=block_size(block);
    size_t remain_size=old_size-size;
    uint32_t prev_offset=extract_prev_offset(block);
    uint32_t next_offset=extract_next_offset(block);
    void* remain_block=((char*)block+size);
    if (remain_size>0) {
        make_block(remain_block,remain_size,1);
        update_free_list(remain_block, prev_offset
                         , next_offset);
    }
    else
        update_free_list(NULL, prev_offset
                         , next_offset);
}

/* expand the heap, return -1 if failed */
static void* expand(size_t size){
    void* result=mem_sbrk((int)size);
    if (result!=(void*)-1) {
        //move epilogue
        heap_top=(void*)((char*)heap_top+size);
        *((uint32_t*)heap_top)=4|0;
        result=(char*)result-4;
    }
    
    return result;
}

//delete this block from free list
static void delete_node(void* block){
    if (block_size(block)<=8) {
        return ;
    }
    
    void* prev=prev_free_block(block);
    void* next=next_free_block(block);
    
    set_next_offset(prev, get_offset(next));
    if (next!=base_address) {
        set_prev_offset(next, get_offset(prev));
    }
}
/* coalesce free blocks and update their free links */
static void coalesce(void *block){
    void *prev_block=block_prev(block);
    void *next_block=block_next(block);
    //int flag=0; /* set this flag to 1 when coalescing */
    
    if (block_free(prev_block)) {
        delete_node(prev_block);
        size_t new_size=block_size(prev_block)
                        +block_size(block);
        block=prev_block;
        make_block(block, new_size, 1);
    }
    
    if (block_free(next_block)) {
        delete_node(next_block);
        size_t new_size=block_size(next_block)
                        +block_size(block);
        make_block(block, new_size, 1);
    }
    
    if(block_size(block)<=8)
        return;
    
    //insert into the front
    void* old_first_b=next_free_block(base_address);
    void* next=next_free_block(old_first_b);
    uint32_t old_first_off=
    extract_next_offset(base_address);
    set_prev_offset(block, 0);
    set_next_offset(block, old_first_off);
    set_next_offset(base_address, get_offset(block));
    if (old_first_off) {
        set_prev_offset(old_first_b, get_offset(block));
    }
    
    next=next_free_block(old_first_b);

}
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
dbg_printf("init\n");
/*padding*/
    char* start_add=mem_heap_lo();
    char* padding_add=align(start_add, 8);
    mem_sbrk((int)(padding_add-start_add));
    base_address=padding_add-8;
    mem_sbrk(4);
    /*end padding*/
    
    /* allocate prologue and epilogue */
    uint32_t* prologue=(uint32_t*)mem_sbrk(8);
    *prologue=8;
    block_mark(prologue, 0);
    
    uint32_t* epilogue=(uint32_t*)mem_sbrk(4);
    *epilogue=4|0;
    /* end allocate prologue and epilogue */
    
    /* assign bottom and top */
    heap_bottom=heap_top=(void*)epilogue;
    /* end assign */
    
    /* init free list */
    set_next_offset(base_address, 0);
    /* end init */
    return 0;
}

/*
 * malloc
 */
void *malloc (size_t size) {
dbg_printf("malloc size%u\n",(uint32_t)size);
   
mm_checkheap(1);
    size_t aligned_size=resize(size+8);
    void* free_block=find_fit(aligned_size);
    
    if (free_block!=NULL) {
        split(free_block, aligned_size);
    }
    else{
        free_block=expand(aligned_size);
        if (free_block==(void*)-1) {
            return NULL;
        }
    }
    
    make_block(free_block, aligned_size, 0);
    
    return block_mem(free_block);
}

/*
 * free
 */
void free (void *ptr) {
dbg_printf("free%p\n",ptr);
if (ptr==NULL) {
        return;
    }
    
    //make ptr point to header
    ptr=(char*)ptr-4;
    
    if (block_free(ptr)) {
        return;
    }
    
    //free block
    block_mark(ptr, 1);
    
    coalesce(ptr);

}


/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *ptr, size_t size) {
	size_t oldsize;
    void *newptr;
ptr=(char *ptr)-4;
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
    oldsize = block_size(ptr);
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

//char *bp = heap_bottom;

    if (verbose){
dbg_printf("Heap (%p):\n", heap_bottom);
}
 /*  if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
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
//dbg_printf("list:(%p)\n",free_list);*/
return 0;
}
/*

static void printblock(void *bp) 
{
    size_t hsize, halloc, fsize, falloc;

   // checkheap(0);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));  
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));  

    if (hsize == 0) {
	dbg_printf("%p: EOL\n", bp);
	return;
    }

     dbg_printf("%p: header: [%u:%c] footer: [%u:%c]\n", bp, 
(uint32_t)	hsize, (halloc ? 'a' : 'f'), 
	(uint32_t)fsize, (falloc ? 'a' : 'f')); 
}
static void checkblock(void *bp) 
{
    if ((size_t)bp % 8)
	printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
	printf("Error: header does not match footer\n");
}
*/
