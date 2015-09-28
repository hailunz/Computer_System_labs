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
#define DSIZE       8       /* Doubleword size (bytes) */  //line:vm:mm:endconst 
//#define CHUNKSIZE  (1<<12)  /* initial heap size (bytes) */  

//#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))  
  
/* single word (4) or double word (8) alignment */  
#define ALIGNMENT 8  
  
/* rounds up to the nearest multiple of 32 bytes(h+f+pred+succ) */  
static inline size_t alignsize(size_t size){
	return (((size) + (8-1)) & ~(8-1));
}


/* Pack a size and allocated bit into a word */
static inline uint32_t PACK(uint32_t size, uint32_t alloc){
    return ((size) | (alloc));
}


/* Read and write a Dword at address p */
static inline uint32_t GET(void *p){
    return (*(uint32_t *)(p))   ;
}        

static inline void PUT(void *p, uint32_t val){
  *(uint32_t *)(p) = (uint32_t) val;
}
/* Read the size and allocated fields from address p */
static inline uint32_t GET_SIZE(void *p){
  return (GET(p) & ~0x7);
}                  
static inline uint32_t GET_ALLOC(void *p){
    return (GET(p) & 0x1);
}                    

/* Given block ptr bp, compute address of its header and footer,
bp point to payload*/
static inline void * HDRP(void *bp){
	return (void *)((char *)(bp) - WSIZE);                 
}    
static inline void * FTRP(void *bp){
       return (void *)((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE);
}
//bp point to header
static inline void * FTRP1(void *bp){
       return (void *)((char *)(bp) + GET_SIZE(bp)-WSIZE);
}



/* Given block ptr bp, compute address of next and previous blocks */
//bp point to header


static inline void *next_blkp(void *bp){
     return (void *)((char *)(bp) + GET_SIZE(bp));
}
static inline void *prev_blkp(void *bp){
     return (void *)((char *)(bp) - GET_SIZE(((char *)(bp) - WSIZE)));
}

static void *base;

/* explicit freelist*/
/* bp point to header*/
// get succ pred address

static inline uint32_t get_succ_offset(void *bp){
	return *((uint32_t*)bp+2);
}
static inline uint32_t get_pred_offset(void *bp){
	return *((uint32_t*)bp+1);
}

// set pred succ addr in the block
static inline void set_pred(void *bp,uint32_t predoff){
	*((uint32_t *)bp+1)=predoff;
}
static inline void set_succ(void *bp,uint32_t succoff){
	*((uint32_t *)bp+2)=succoff;
}
static inline uint32_t get_offset(void *bp){
	return (uint32_t)((char*)bp-(char *)base);
}
static inline void *get_addr(uint32_t offset){
	return (void *)((char *)base+offset);
}
// get pred succ pointer, point to the header
static inline void* pred_blkp(void *bp){
	uint32_t offset=get_pred_offset(bp);
	return  get_addr(offset);   
}
static inline void* succ_blkp(void *bp){
	uint32_t offset=get_succ_offset(bp);
	return  get_addr(offset);   
	
}



/* $end mallocmacros */

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  
static void *free_list=NULL;// pointer to the free_list root


static void *heap_bottom;
static void *heap_top;

static int num=1;//print list

#ifdef NEXT_FIT
static char *rover;           /* Next fit rover */
#endif

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t asize);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
//static void printblock(void *bp); 
//static void checkblock(void *bp);
static void *set_free_block(void *bp,size_t size,uint32_t alloc);
static void *add_free_block(void *bp);
static void delete_free_block(void *bp);
static void alloc_block(void *bp,size_t size);

static int check_epi_pro(int verbose);
static int check_align(int verbose);
static int check_hf(int verbose);
static int check_coalesce(int verbose);
static int check_heap(int verbose);
static void print_heap(void *bp);

static void print_list(int num);
static int check_list_block(int verbose);
static int check_free_list(int verbose);

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
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
	return -1;
	base=heap_listp;
    PUT(heap_listp, 0);                          /* Alignment padding */

    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2*WSIZE);                     //line:vm:mm:endinit  

	heap_bottom=heap_listp+3*WSIZE;

	free_list=base;

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
//    checkheap(1);  // Let's make sure the heap is ok!
    size_t newsize;  /* Adjusted block size */
    char *bp;      

    /* Ignore spurious requests */
    if (size == 0)
	return NULL;

    newsize=alignsize(size+8);  

    /* Search the free list for a fit */
    if ((bp = find_fit(newsize)) != NULL) {  
		place(bp, newsize);
	    return (void *)((uint32_t *)bp+1);
    }

    /* No fit found. Get more memory and place the block */             
    if ((bp = extend_heap(newsize)) == NULL)  
		return NULL; 
	alloc_block(bp,newsize);
    return (void *)((uint32_t *)bp+1);
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
	
	if (heap_listp == 0){
	mm_init();
    }

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

	coalesce(header);

}


/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
/* $begin coalesce */
static void *coalesce(void *bp) //bp point to header
{
	void *prev_block=prev_blkp(bp);
	void *next_block=next_blkp(bp);

    uint32_t prev_alloc = GET_ALLOC(prev_block);
    uint32_t next_alloc = GET_ALLOC(next_block);
    uint32_t size = GET_SIZE(bp);

    if (prev_alloc && next_alloc) {            /* Case 1 */
		return bp;
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

    void *bp = (void *)(heap_listp+1);
	int flag=0;
    if (verbose)
	printf("Heap (%p):\n", heap_listp);
    flag=check_heap(1)|check_free_list(1);
	print_heap(bp);
	print_list(num);
	if(flag)
		return 1;
	return 0;
}



/* 
 * The remaining routines are internal helper routines 
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
//bp point to the payload,return pointer to header
static void *extend_heap(size_t asize) 
{
    void *bp;
	bp = mem_sbrk((int)asize);
    if (bp ==(void *)-1)  
		return NULL;                                        
	uint32_t size=(uint32_t)asize;
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   
    PUT(next_blkp(HDRP(bp)), PACK(0, 1)); /* New epilogue header */ 
	
	heap_top=next_blkp(HDRP(bp));

	return coalesce(HDRP(bp));
} 

// set header and footer
static void *set_free_block(void *bp,size_t size,uint32_t alloc){
	
	REQUIRES(in_heap(bp));

	PUT((uint32_t *)bp,PACK((uint32_t)size,alloc));
	PUT((uint32_t *)bp+1,0);//pred addr
	PUT((uint32_t *)bp+2,0);//succ addr
	PUT(FTRP1(bp), PACK((uint32_t)size, alloc)); 
	return bp;
}

/* addfreeblock*/
static void *add_free_block(void *bp){

	REQUIRES(in_heap(bp));

	set_succ(bp,get_offset(free_list));
	set_pred(bp,0);

	if (get_offset(free_list)!=0)
	set_pred(free_list,get_offset(bp));
	 
	//set new list header
	free_list=bp;
	return bp;
}

static void delete_free_block(void *bp){
	
	void *pred_block=pred_blkp(bp);
	void *succ_block=succ_blkp(bp);

	if (get_pred_offset(bp)==0){//header
		set_pred(succ_block,0);
		free_list=succ_block;
	}
	else if (get_succ_offset(bp)==0){
		set_succ(pred_block,0);
	}
	else {
		set_succ(pred_block,get_succ_offset(bp));
		set_pred(succ_block,get_pred_offset(bp));
	}

}

static void alloc_block(void *bp,size_t size){

	PUT(bp,PACK((uint32_t)size,1));
	PUT(FTRP1(bp),PACK((uint32_t)size,1));
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

	if (remain_size>=16){
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

	if (free_list==base)
		return NULL;
	
	for (bp=free_list;get_succ_offset(bp)!=0;bp=succ_blkp(bp)){
		if (GET_SIZE(bp)>=asize){
in_heap(bp);
			return bp;
}	}
	return NULL;
}

//used in checkheap
static void print_heap(void *bp){
	//int count=0;
	uint32_t hsize,halloc,fsize,falloc;

	printf("Heap_bottom (%p):\n",(char *)bp);
	for(;bp!=heap_top;bp=next_blkp(bp)){
		hsize=GET_SIZE(bp);
		halloc=GET_ALLOC(bp);
		fsize=GET_SIZE(FTRP1(bp));
		falloc=GET_ALLOC(FTRP1(bp));

		printf("%p: header[%u:%u] footer[%u:%u]\n",
			(char *)bp,hsize,halloc,fsize,falloc);
	
	}
	printf("%p: EOL\n",(char *)bp);
}

static int check_epi_pro(int verbose){

    if (verbose){
dbg_printf("check_epi\n");}
	void *prologue=heap_listp;
	void *epilogue=heap_top;
	size_t pro_sizeh=GET_SIZE(HDRP(prologue));
	size_t pro_sizef=GET_SIZE(prologue);
	size_t epi_size=GET_SIZE(epilogue);
	size_t pro_alloch=GET_ALLOC(HDRP(prologue));
	size_t pro_allocf=GET_ALLOC(prologue);
	size_t epi_alloc=GET_ALLOC(epilogue);

	if (pro_sizeh!=8){
		dbg_printf("Error:bad prologue header size\n");
		return 1;
	}
	if (pro_sizef!=8){
		dbg_printf("Error:bad prologue footer size\n");
		return 1;
	}
	if (epi_size!=0){
		dbg_printf("Error:bad epilogue header size\n");
		return 1;
	}
	if (pro_alloch!=1){
		dbg_printf("Error:bad prologue header alloc\n");
		return 1;
	}
	if (pro_allocf!=1){
		dbg_printf("Error:bad prologue footer alloc\n");
		return 1;
	}
	if (epi_alloc!=1){
		dbg_printf("Error:bad epilogue header alloc\n");
		return 1;
	}
	return 0;

}

static int check_align(int verbose){
if(verbose)
{
dbg_printf("check_align\n");}
	void *bp=heap_bottom;
	int count=0;
	for(;bp!=heap_top;count++,bp=next_blkp(bp)){
		if(!aligned((void *)((uint32_t *)bp+1))){
			dbg_printf("Error: NO. %d block isn't aligned.\n",count);
		return 1;
		}
	}
	return 0;
}

//static int check_bound(int verbose){

//}

static int check_hf(int verbose){
if(verbose){
dbg_printf("check_hf\n");}
	void *bp=heap_bottom;
	int count=0;
	uint32_t header;
	uint32_t footer;

	for(;bp!=heap_top;bp=next_blkp(bp),count++){
			header=*((uint32_t *)bp);
			footer=*((uint32_t *)FTRP1(bp));
			if (header!=footer){
				dbg_printf("Error: NO. %d block isn't aligned.\n",count);
				return 1;
			}
	}
	return 0;
}

static int check_coalesce(int verbose){
if(verbose){
dbg_printf("check_coal\n");}
	void *bp=heap_bottom;
	void *next_blk=next_blkp(bp);
	int count=0;

	uint32_t bp_free;
	uint32_t next_free;

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
if (verbose){
dbg_printf("check_heap\n");}
	int flag=check_epi_pro(1)|check_align(1);
	flag|=check_hf(1)|check_coalesce(1);
	return flag;
}

//check list
static void print_list(int num){
	void *bp=free_list;
	printf("List %d:" ,num);
	
	if (free_list==base){
		printf("(null)\n");
	num++;
	return;
	}
	for(;get_succ_offset(bp)!=0;bp=succ_blkp(bp)){
		printf("(%p):",(char *)bp);
	}
	num++;
}

static int check_list_consis(int verbose){
	if(verbose){
		void *bp=free_list;
		void *succ_blk;
		int count=0;

		if (bp==base)
			return 0;
		for(;get_succ_offset(bp)!=0;bp=succ_blkp(bp),count++){
			succ_blk=succ_blkp(bp);
			if (bp!=pred_blkp(succ_blk)){
				printf("list not consis: block %d\n",count);
				return 1;
			}
		}
		
	}
	return 0;
}
static int check_list_block(int verbose){
	if (verbose){
		void *bp=free_list;
		uint32_t alloc=0;
		int count=0;
		
		if (bp==base)
			return 0;
		for(;get_succ_offset(bp)!=0;bp=succ_blkp(bp),count++){
			alloc=GET_ALLOC(bp);
			if (alloc){
			printf("list has alloc block %d\n",count);
			return 1;
			}
		}
	}
		return 0;
	
}

static int check_free_list(int verbose){
	int flag=0;
	if (verbose){
		flag=check_list_consis(1)|check_list_block(1);
	}
	return flag;
}





