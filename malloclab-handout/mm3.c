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

    return (block[0] & 0x3FFFFFFF);
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
#define DSIZE       8       /* Doubleword size (bytes) */  //line:vm:mm:endconst 
#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */  

//#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))  
  
/* single word (4) or double word (8) alignment */  
#define ALIGNMENT 8  
  
/* rounds up to the nearest multiple of 32 bytes(h+f+pred+succ) */  
static inline size_t alignsize(size_t size){
	return (((size) + (32-1)) & ~(32-1));
}


/* Pack a size and allocated bit into a Dword */
static inline size_t PACK(size_t size, size_t alloc){
    return ((size) | (alloc));
}

/* Read and write a Dword at address p */
static inline size_t GET(void *p){
    return (*(size_t *)(p))   ;
}        

static inline void PUT(void *p, size_t val){
  *(size_t *)(p) = (size_t) val;
}
/* Read the size and allocated fields from address p */
static inline size_t GET_SIZE(void *p){
  return (GET(p) & ~0x7);
}                  
static inline size_t GET_ALLOC(void *p){
    return (GET(p) & 0x1);
}                    

/* Given block ptr bp, compute address of its header and footer,
bp point to payload*/
static inline void * HDRP(void *bp){
	return (void *)((char *)(bp) - DSIZE);                 
}    
static inline void * FTRP(void *bp){
       return (void *)((char *)(bp) + GET_SIZE(HDRP(bp)) - 2*DSIZE);
}
//bp point to header
static inline void * FTRP1(void *bp){
       return (void *)((char *)(bp) + GET_SIZE(bp)-DSIZE);
}



/* Given block ptr bp, compute address of next and previous blocks */
//bp point to header
static inline void *next_blkp(void *bp){
     return (void *)((char *)(bp) + GET_SIZE(bp));
}
static inline void *prev_blkp(void *bp){
     return (void *)((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)));
}



/* explicit freelist*/
/* bp point to header*/
// get succ pred address
static inline size_t pred_blka(void *bp){
	return  (size_t) *((size_t*)(bp)+1);
}
static inline size_t succ_blka(void *bp){
	return  (size_t) *((size_t*)(bp)+2);
}

// set pred succ addr in the block
static inline void set_pred(void *bp,size_t predaddr){
	*((size_t *)bp+1)=predaddr;
}
static inline void set_succ(void *bp,size_t succaddr){
	*((size_t *)bp+2)=succaddr;
}

// get pred succ pointer, point to the header
static inline void* pred_blkp(void *bp){
	return (void *) pred_blka(bp);   
}
static inline void* succ_blkp(void *bp){
	return (void *) succ_blka(bp);   
}



/* $end mallocmacros */

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  
static void *free_list=NULL;// pointer to the free_list root

static void *heap_bottom;
static void *heap_top;

#ifdef NEXT_FIT
static char *rover;           /* Next fit rover */
#endif

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void coalesce(void *bp);
//static void printblock(void *bp); 
//static void checkblock(void *bp);
static void *set_free_block(void *bp,size_t size,int alloc);
static void *add_free_block(void *bp);
static void delete_free_block(void *bp);
static void alloc_block(void *bp,size_t size);
static int check_epi_pro(int verbose);
static int check_align(int verbose);
static int check_hf(int verbose);
static int check_coalesce(int verbose);
static int check_heap(int verbose);


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
    if ((heap_listp = mem_sbrk(4*DSIZE)) == (void *)-1)
	return -1;

    PUT(heap_listp, 0);                          /* Alignment padding */

    PUT(heap_listp + (1*DSIZE), PACK(2*DSIZE, 1)); /* Prologue header */ 
    PUT(heap_listp + (2*DSIZE), PACK(2*DSIZE, 1)); /* Prologue footer */ 
    PUT(heap_listp + (3*DSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2*DSIZE);                     //line:vm:mm:endinit  

	heap_bottom=heap_listp+DSIZE;
	heap_top=mem_heap_hi();

#ifdef NEXT_FIT
    rover = heap_listp;
#endif
/* $begin mminit */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
  //  if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
	//return -1;
    return 0;   
}

/*
 * malloc
 */
void *malloc (size_t size) {
    checkheap(1);  // Let's make sure the heap is ok!
    size_t newsize;  /* Adjusted block size */
    char *bp;      

    /* Ignore spurious requests */
    if (size == 0)
	return NULL;

    newsize=alignsize(size+16);  

    /* Search the free list for a fit */
    if ((bp = find_fit(newsize)) != NULL) {  
		place(bp, newsize);
	    return (void *)((size_t *)bp+1);
    }

    /* No fit found. Get more memory and place the block */             
    if ((bp = extend_heap(newsize)) == NULL)  
		return NULL; 
	alloc_block(bp,newsize);
    return (void *)((size_t *)bp+1);
}

/*
 * free
 */
void free (void *ptr) {
    if (ptr == NULL) {
        return;
    }
    
	void *header=HDRP(ptr);
	size_t size = GET_SIZE(header);
	
	if(!GET_ALLOC(header))
		return;

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

	coalesce(header);

}


/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
/* $begin coalesce */
static void coalesce(void *bp) //bp point to header
{
	void *prev_block=prev_blkp(bp);
	void *next_block=next_blkp(bp);

    size_t prev_alloc = GET_ALLOC(prev_block);
    size_t next_alloc = GET_ALLOC(next_block);
    size_t size = GET_SIZE(bp);

    if (prev_alloc && next_alloc) {            /* Case 1 */
		return ;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
	size += GET_SIZE(next_block);
	delete_free_block(next_block);
	PUT(bp, PACK(size, 0));
	PUT(FTRP1(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
	size += GET_SIZE(prev_block);
	delete_free_block(prev_block);
	PUT(FTRP1(bp), PACK(size, 0));
	PUT(prev_block, PACK(size, 0));
	bp = prev_block;
    }

    else {                                     /* Case 4 */
	size += GET_SIZE(prev_block) + 
	    GET_SIZE(next_block);
	delete_free_block(prev_block);
	delete_free_block(next_block);

	PUT(prev_block, PACK(size, 0));
	PUT(FTRP1(next_block), PACK(size, 0));
	bp = prev_block;
    }

	add_free_block(bp);
	
}
/* $end coalecse */



/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
	size_t oldsize;
	void *newptr;
	size_t newsize;
	newsize=alignsize(size+16);

  /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
		free(oldptr);
    return 0;
  }

  /* If oldptr is NULL, then this is just malloc. */
	if(oldptr == NULL) {
		return malloc(size);
  }

  newptr = malloc(size);

  /* If realloc() fails the original block is left untouched  */
  if(!newptr) {
    return 0;
  }

  /* Copy the old data. */
  oldsize = GET_SIZE(oldptr);
  if(newsize < oldsize) oldsize = newsize;
  memcpy(HDRP(newptr), HDRP(oldptr), oldsize);

  /* Free the old block. */
  free(oldptr);

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

   // char *bp = heap_listp;
	int flag=0;
    if (verbose)
	printf("Heap (%p):\n", heap_listp);

    flag=check_heap(1);

	return flag;
}



/* 
 * The remaining routines are internal helper routines 
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
//bp point to the header
static void *extend_heap(size_t asize) 
{
    void *bp;
	bp = mem_sbrk((int)asize);
    if (bp ==(void *)-1)  
		return NULL;                                        

    /* Initialize free block header/footer and the epilogue header */
    PUT(bp, PACK(asize, 0));         /* Free block header */   
    PUT(FTRP1(bp), PACK(asize, 0));         /* Free block footer */   
    PUT(next_blkp(bp), PACK(0, 1)); /* New epilogue header */ 
	heap_top=next_blkp(bp);

	return bp;
} 

// set header and footer
static void *set_free_block(void *bp,size_t size,int alloc){
	
	REQUIRES(in_heap(bp));

	PUT((size_t *)bp,PACK(size,alloc));
	PUT((size_t *)bp+1,0);//pred addr
	PUT((size_t *)bp+2,0);//succ addr
	PUT(FTRP1(bp), PACK(size, alloc)); 
	return bp;
}

/* addfreeblock*/
static void *add_free_block(void *bp){

	REQUIRES(in_heap(bp));

	set_succ(bp,(size_t)free_list);
	set_pred(bp,0);

	set_pred(free_list,(size_t)bp);
	
	//set new list header
	free_list=bp;
	return bp;
}

static void delete_free_block(void *bp){
	
	void *pred_block=pred_blkp(bp);
	void *succ_block=succ_blkp(bp);

	if (pred_blka(bp)==0){//header
		set_pred(succ_block,0);
		free_list=succ_block;
	}
	else if (succ_blka(bp)==0){
		set_succ(pred_block,0);
	}
	else {
		set_succ(pred_block,succ_blka(bp));
		set_pred(succ_block,pred_blka(bp));
	}

}

static void alloc_block(void *bp,size_t size){

	PUT(bp,PACK(size,1));
	PUT(FTRP(bp),PACK(size,1));
}
/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */

static void place(void *bp, size_t asize){
	
	size_t osize=GET_SIZE(bp);
	size_t remain_size=osize-asize;

//	size_t old_predaddr=pred_blka(bp);
//	size_t old_succaddr=succ_blka(bp);

	void *remain_block=(char *)bp+asize;

	if (remain_size>=32){
		delete_free_block(bp);
		set_free_block(remain_block,remain_size,0);
		add_free_block(remain_block);
	}
	else{
		delete_free_block(bp);
	}
	alloc_block(bp,asize);
}


/* 
 * find_fit - Find a fit for a block with asize bytes ,find first fit
 */


static void *find_fit(size_t asize){
	
	void *bp;

	if (free_list==NULL)
		return NULL;
	
	for (bp=free_list;succ_blka(bp)!=0;bp=succ_blkp(bp)){
		if (GET_SIZE(bp)>=asize){
in_heap(bp);
			return bp;
}	}
	return NULL;
}

//used in checkheap

static int check_epi_pro(int verbose){
if(verbose){;}
	void *prologue=heap_listp;
	void *epilogue=mem_heap_hi();
	
	if (GET_SIZE(HDRP(prologue))!=16){
		dbg_printf("Error:bad prologue header\n");
		return 1;
	}
	
	if (GET_SIZE(prologue)!=16){
		dbg_printf("Error:bad prologue footer\n");
		return 1;
	}
	if (GET_SIZE(epilogue)!=0){
		dbg_printf("Error:bad epilogue header\n");
		return 1;
	}

	return 0;

}

static int check_align(int verbose){
if(verbose){;}
	void *bp=heap_bottom;
	int count=0;
	for(;bp!=heap_top;count++,bp=next_blkp(bp)){
		if(!aligned((void *)((size_t *)bp+1))){
			dbg_printf("Error: NO. %d block isn't aligned.\n",count);
		return 1;
		}
	}
	return 0;
}

//static int check_bound(int verbose){

//}

static int check_hf(int verbose){
if(verbose){;}
	void *bp=heap_bottom;
	int count=0;
	size_t header;
	size_t footer;

	for(;bp!=heap_top;bp=next_blkp(bp),count++){
			header=*((size_t *)bp);
			footer=*((size_t *)FTRP1(bp));
			if (header!=footer){
				dbg_printf("Error: NO. %d block isn't aligned.\n",count);
				return 1;
			}
	}
	return 0;
}

static int check_coalesce(int verbose){
if(verbose){;}
	void *bp=heap_bottom;
	void *next_blk=next_blkp(bp);
	int count=0;

	size_t bp_free;
	size_t next_free;

	for(;bp!=heap_top;bp=next_blkp(bp),count++){
		next_blk=next_blkp(bp);
		if (next_blk!=heap_top){
			bp_free=!(GET_ALLOC(bp));
			next_free=!(GET_ALLOC(next_blk));
			if (bp_free&&next_free){
				dbg_printf("Error:two free block %d %d.\n"
					,count,count+1);
				return 1;
			}
		}
	}
	return 0;
}

static int check_heap(int verbose){
if (verbose){;}
	int flag=check_epi_pro(1)|check_align(1);
	flag|=check_hf(1)|check_coalesce(1);
	return flag;
}
//static int check_free_list(int verbose){

//}



