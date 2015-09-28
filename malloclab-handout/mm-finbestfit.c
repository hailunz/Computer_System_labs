/*
 * mm.c
 * Hailun Zhu-ID:hailunz
 *
 * I use a segregated list with class_num size classes. 
 * Based on (explicit list+ FILO first fit);
 * The minimum block size is 16 bytes except prologue and epilogue.
 * The blocks include a header, footer, pred part and succ part. 
 * The pred and succ part store the offset of the predcessor and
 * successor blocks in the freelist.
 * Size classes: there is class_num size classes, the nth class stores 
 * the blocks of size 2^(n+4)~2^(n+5)-1. I ignore the sizes smaller than 
 * 16, so the size classes start with 2^4 bytes.
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
    return p <=mem_heap_hi() && p >= mem_heap_lo();
}


/*
 *  Block Functions
 *  ---------------
 *  The functions below act similar to the macros in the book, but calculate
 *  size in multiples of 4 bytes.
 *	My block funtions include: GET,PUT,GET_SIZE,GET_ALLOC,HDRP,FTRP,NEXT_BLKP
 *  PREV_BLKP,get_succ_offset,get_pred_offset,set_pred,set_succ,pred_blkp,
 *  succ_blkp.
 *  The details about each function is described individually.
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

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Doubleword size (bytes) */
#define CHUNKSIZE  (1<<8)  /* Extend heap by this amount (bytes) */ 

static inline size_t MAX(size_t x,size_t y){
	return((x) > (y)? (x) : (y));
}

/* Pack a size and allocated bit into a word */
static inline size_t PACK(size_t size,size_t alloc){
 return ((size) | (alloc)); 
}
/* Read and write a word at address p */
static inline unsigned int GET(void *p){
    REQUIRES(p != NULL);
    REQUIRES(in_heap(p));	
return  (*(unsigned int *)(p));
}           
static inline void PUT(void *p, size_t val) {
    REQUIRES(p != NULL);
    REQUIRES(in_heap(p));
 (*(unsigned int *)(p) = (val));
}    

/* Read the size and allocated fields from address p */
static inline size_t GET_SIZE(void *p){
	REQUIRES(p != NULL);
    REQUIRES(in_heap(p));
return  (GET(p) & ~0x7) ;
}                
static inline size_t GET_ALLOC(void *p){
    REQUIRES(p != NULL);
    REQUIRES(in_heap(p));
	return (GET(p) & 0x1);
}                 

/* Given block ptr bp, compute address of its header and footer */
static inline void *HDRP(void *bp){
	REQUIRES(bp != NULL);
    return (void *)  ((char *)(bp) - WSIZE) ;
}                  
static inline void *FTRP(void *bp){
    REQUIRES(bp != NULL);
	REQUIRES(in_heap(bp));
	return  (void *)((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE);
} 

/* Given block ptr bp, compute address of next and previous blocks */
static inline void *NEXT_BLKP(void *bp){
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));
return (void *) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)));
} 
static inline void *PREV_BLKP(void *bp){
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));
return (void *) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)));
} 
/* $end mallocmacros */

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  

static void *free_list=NULL;/* free list head*/

static void *heap_bottom;
static void *heap_top;
static void *base;/* base address to compute offset*/

static int class_num=20;
static int free_block_num=0;
static void *seg_list=NULL;/* segregated list head */

// get the successor and predecessor blocks' offset
static inline uint32_t get_succ_offset(void *bp){
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));
	return *((uint32_t*)bp+1);
}
static inline uint32_t get_pred_offset(void *bp){
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));
	return *((uint32_t*)bp);
}

// set pred succ address in the block
static inline void set_pred(void *bp,uint32_t predoff){
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));
	*((uint32_t *)bp)=predoff;
}
static inline void set_succ(void *bp,uint32_t succoff){
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));
	*((uint32_t *)bp+1)=succoff;
}
// get offset and absolute address
static inline uint32_t get_offset(void *bp){
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));
	return (uint32_t)((char *)bp-(char *)base);
}
static inline void *get_addr(uint32_t offset){
	return (void *)((char *)base+offset);
}
// get pred succ pointer
static inline void* pred_blkp(void *bp){
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));
	uint32_t offset=get_pred_offset(bp);
	return  get_addr(offset);   
}
static inline void* succ_blkp(void *bp){
    REQUIRES(bp != NULL);
    REQUIRES(in_heap(bp));
	uint32_t offset=get_succ_offset(bp);
	return  get_addr(offset);   
}


/* Function prototypes for internal helper routines */
static void *extend_heap(size_t size);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp); 
static int checkblock(void *bp);
static void *add_free_block(void *bp,size_t size);
static void delete_free_block(void *bp);
static void alloc_block(void *bp,size_t size);
static int check_my_heap(int verbose);
static int checklist(int verbose);
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
//	dbg_printf("init\n");
	if ((heap_listp = mem_sbrk((class_num+4)*WSIZE)) == (void *)-1) //line:vm:mm:begininit
		return -1;
	base=heap_listp;
	char *bp;
	int i=0;
	for (bp=heap_listp;i<(class_num>>1);i++){
		    *((size_t *) bp)=0;/* Alignment padding */
			bp=bp+DSIZE;
		}
	PUT(bp,0);
	heap_listp =bp;          
	dbg_printf("list%p",heap_listp); 
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2*WSIZE);                 
	heap_bottom=heap_listp+WSIZE;/* point to the first block */
	heap_top=heap_bottom;

	dbg_printf("heap list%p\n",heap_listp);
	seg_list=base;

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE) == NULL) 
		return -1;

    return 0;
}

/*
 * malloc
 */
void *malloc (size_t size) {
	dbg_printf("malloc size%u\n",(uint32_t)size);
	size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    void *bp;      
	checkheap(1);

    /* Ignore spurious requests */
    if (size == 0)
	return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
	asize=(size+15)&(~0x7);
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {  
		place(bp, asize);                  
		return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);                
    if ((bp = extend_heap(extendsize)) == NULL)  
		return NULL;                                 
    place(bp, asize);      
	checkheap(1);
    return bp;
}

/*
 * free
 */
void free (void *ptr) {
	//dbg_printf("free%p\n",ptr);
    if(ptr == 0) 
		return;

    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

	coalesce(ptr);

}


/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */

static void* coalesce(void *bp) //bp point to header
{
	//dbg_printf("coalesce %p\n",bp);
	void *prev_blk=PREV_BLKP(bp);
	void *next_blk=NEXT_BLKP(bp);
	size_t prev_alloc = GET_ALLOC(FTRP(prev_blk));
    size_t next_alloc = GET_ALLOC(HDRP(next_blk));
    size_t size = GET_SIZE(HDRP(bp));
	
    if (prev_alloc && next_alloc) {            /* Case 1 */	
		//dbg_printf("no coal%p\n",bp);
		add_free_block(bp,size);
		return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
		size += GET_SIZE(HDRP(next_blk));
		delete_free_block(next_blk);
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
		size += GET_SIZE(HDRP(prev_blk));
		delete_free_block(prev_blk);
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(prev_blk), PACK(size, 0));
		bp = prev_blk;
    }

    else {                                     /* Case 4 */
		size += GET_SIZE(HDRP(prev_blk)) + 
	    GET_SIZE(FTRP(next_blk));
		delete_free_block(prev_blk);
		delete_free_block(next_blk);
		PUT(HDRP(prev_blk), PACK(size, 0));
		PUT(FTRP(next_blk), PACK(size, 0));
		bp = prev_blk;
   }

	add_free_block(bp,size);
    return bp;
	
}

/*
 * realloc 
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
	oldsize=GET_SIZE(HDRP(ptr));
	size_t asize=(size+15)&(~0x7);   
	if (asize==oldsize){
		return ptr;
	}
	/*
	else if (asize<oldsize){
		size_t free_size=oldsize-asize;
		if(free_size<16){
			newptr=malloc(size);
			if(!newptr){
				return 0;
			}
		memcpy(newptr,ptr,size);
		free(ptr);
		return newptr;
		}
		else{
			void *free_block=(void*)((char *)ptr+asize);
			alloc_block(ptr,asize);
			PUT(HDRP(free_block),PACK(free_size,0));
			PUT(FTRP(free_block),PACK(free_size,0));
			add_free_block(free_block);
			return ptr;
		}
	}*/
	else{   
		newptr = malloc(size);

    /* If realloc() fails the original block is left untouched  */
		if(!newptr) {
		return 0;
		}

    /* Copy the old data. */
	  if(asize < oldsize) oldsize = size;
		memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
		free(ptr);
		return newptr;
	}
    return newptr;
}

/*
 * calloc 
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
	if(verbose==0){		
		return 0;
	}
	int error=0;

	error=((check_my_heap(verbose))|(checklist(verbose)));	
	
	return error;
}

/* 
 * The remaining routines are internal helper routines 
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */

static void *extend_heap(size_t size) {
    char *bp;
	dbg_printf("extend heap\n");
  
    if ((long)(bp = mem_sbrk(size)) == -1)  
	return NULL;                                        

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ 
	
	/* point to the epilogue header*/
	heap_top=HDRP(NEXT_BLKP(bp));
	
    /* Coalesce if the previous block was free */
	//dbg_printf("extend%p\n",bp);
    return coalesce(bp);                          
	}

/* add freeb lock*/
static void *add_free_block(void *bp,size_t size){
	//dbg_printf("addfree%p\n",bp);
size_t osize=size;
	dbg_printf("add free%p %lu\n",bp,size);
	int list=0;
	size>>=5;
	while((list<(class_num-2))&&(size>1)){
		size=size>>1;
		list++;
	}
	//get free_list head
	free_list=(void *)((uint32_t *)seg_list+list);
void *block=succ_blkp(free_list);
	dbg_printf("add free list%d %p\n",list,free_list);
if(block==base){	
set_succ(bp,get_succ_offset(free_list));
	set_pred(bp,get_offset(free_list));

//	if (get_succ_offset(free_list)!=0)
//	set_pred(succ_blkp(free_list),get_offset(bp));
	 
	//set new list header
	set_succ(free_list,get_offset(bp));
/*
	bp=free_list;
	while(bp!=base){
	dbg_printf("list: %p->:\n",bp);
	bp=succ_blkp(bp);
}*/
	return bp;
}
void *pred;
while(block!=base){
pred=block;
	if(osize<GET_SIZE(HDRP(block))){
set_succ(bp,get_offset(block));
set_pred(bp,get_pred_offset(block));
set_succ(pred_blkp(block),get_offset(bp));
set_pred(block,get_offset(bp));
return bp;
		
	}
if(osize==GET_SIZE(HDRP(block))){
	set_succ(bp,get_succ_offset(block));
	set_pred(bp,get_offset(block));
if(get_succ_offset(block))	
set_pred(succ_blkp(block),get_offset(bp));
set_succ(block,get_offset(bp));
return bp;
}
block=succ_blkp(block);
}
set_succ(bp,0);
set_pred(bp,get_offset(pred));
set_succ(pred,get_offset(bp));
return bp;
}

static void delete_free_block(void *bp){

	dbg_printf("delete%p\n",bp);
	void *pred_block=pred_blkp(bp);
	void *succ_block=succ_blkp(bp);
	
	set_succ(pred_block,get_succ_offset(bp));

	if (get_succ_offset(bp))
	set_pred(succ_block,get_pred_offset(bp));
}

static void alloc_block(void *bp,size_t size){
	//dbg_printf("alloc %p %u\n",bp,(uint32_t)size);
	PUT(HDRP(bp),PACK(size,1));
	PUT(FTRP(bp),PACK(size,1));
}

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */

static void place(void *bp, size_t asize)   {
	//dbg_printf("place%p\n",bp);
    size_t csize = GET_SIZE(HDRP(bp));   
	size_t remain_size=csize-asize;
    if ((remain_size) >= 16) {
		void *remain_blk; 
		delete_free_block(bp);	
		alloc_block(bp,asize);
		remain_blk= NEXT_BLKP(bp);
		PUT(HDRP(remain_blk), PACK(remain_size, 0));
		PUT(FTRP(remain_blk), PACK(remain_size, 0));
		add_free_block(remain_blk,remain_size);
    }
    else { 
		delete_free_block(bp);
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
    }
}

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */

static void *find_fit(size_t asize)
{
	dbg_printf("find fit size%d\n",(uint32_t)asize);
    /* First fit search */
	int list=0;
	size_t size=asize>>5;
	while((list<(class_num-2))&&(size>1)){
		size=size>>1;
		list++;
	}
	//get free_list head
	free_list=(void *)((uint32_t *)seg_list+list);
    while(list<class_num){
		void *bp=succ_blkp(free_list);
		while(bp!=base){
			if(GET_SIZE(HDRP(bp))>=asize){
				return bp;
			}
			bp=succ_blkp(bp);	
		}
		list++;
		//get free_list head
		free_list=(void *)((uint32_t *)seg_list+list);
	}
    return NULL; /* No fit */
}

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

static int checkblock(void *bp) 
{	
	size_t size=GET_SIZE(HDRP(bp));
    if ((size_t)bp % 8){
		printf("Error: %p is not doubleword aligned\n", bp);
		return 1;
	}
	if (GET(HDRP(bp)) != GET(FTRP(bp))){
		printf("Error: header does not match footer\n");
		return 1;
	}
	if ((size<8)||(size % 8)){
		printf("Error:[%p] wrong block size %lu\n",bp,size);
		return 1;
	}
	if (!in_heap(bp)){
		printf("Error:block %p is not in the heap\n",bp);
		return 1;
	}
	return 0;
}
//check heap
static int check_my_heap(int verbose){
	if (verbose==0){
		return 0;
	}
	else {
		dbg_printf("Heap (%p):\n", heap_listp);
		void *bp;
		if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp))){
			printf("Bad prologue header\n");
			return 1;
		}
		if (checkblock(heap_listp)){
			return 1;
		}
		free_block_num=0;
		for (bp = heap_listp+8; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
			printblock(bp);
			if (checkblock(bp)){
				return 1;
			}
			if (GET_ALLOC(HDRP(bp))==0)
				free_block_num++;
		}

		if (verbose){
			dbg_printf("%p: EOL\n", heap_top);
			dbg_printf("heap end %p\n",mem_heap_hi());
		}
		if ((GET_SIZE(heap_top) != 0) || !(GET_ALLOC(heap_top))){
			printf("Bad epilogue header\n");
			return 1;
		}
	}
	return 0;
}
//check free_list
static int checklist(int verbose){
	if (!verbose){
		return 0;
	}
	else {
		int list=0;
		free_list=seg_list;
		void *bp;
		int num=0;
		while(list<class_num){
			dbg_printf("freelist [%d] %p:->",list,free_list);
			bp=succ_blkp(free_list);
			while(bp!=base){
			dbg_printf("list:%p->",bp);
				//in_heap
				if(!in_heap(bp)){
				printf("Error:free list pointer is not in the heap.\n");
					return 1;
				}
				//check consistency
				if ((succ_blkp(pred_blkp(bp)))!=bp){
					printf("Error:free list is not consistent.\n");
					return 1;
				}
				//check block allocated or not
				if (GET_ALLOC(HDRP(bp))){
					printf("Error:free list has allocated block.\n");
					return 1;				
				}

				//all blocks in each list bucket 
				//fall within bucket size range
				if (((GET_SIZE(HDRP(bp)))>>(list+6))!=0){
					printf("Error:wrong free block size%d.\n",list);
					return 1;			
				}
				num++;
				bp=succ_blkp(bp);	
			}
			dbg_printf("end.\n");
			list++;
			//get free_list head
			free_list=(void *)((uint32_t *)seg_list+list);
		}
		if (num!=free_block_num){
			printf("Error:wrong free block number.\n");
			return 1;		
		}
	}
	return 0;
}
