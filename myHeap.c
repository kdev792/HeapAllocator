#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "myHeap.h"
 
/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {           

    int size_status;
    /*
     * Size of the block is always a multiple of 8.
     * Size is stored in all block headers and in free block footers.
     *
     * Status is stored only in headers using the two least significant bits.
     *   Bit0 => least significant bit, last bit
     *   Bit0 == 0 => free block
     *   Bit0 == 1 => allocated block
     *
     *   Bit1 => second last bit 
     *   Bit1 == 0 => previous block is free
     *   Bit1 == 1 => previous block is allocated
     * 
     * End Mark: 
     *  The end of the available memory is indicated using a size_status of 1.
     * 
     * Examples:
     * 
     * 1. Allocated block of size 24 bytes:
     *    Allocated Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 25
     *      If the previous block is allocated p-bit=1 size_status would be 27
     * 
     * 2. Free block of size 24 bytes:
     *    Free Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 24
     *      If the previous block is allocated p-bit=1 size_status would be 26
     *    Free Block Footer:
     *      size_status should be 24
     */
} blockHeader;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */
blockHeader *heapStart = NULL;     

/* Size of heap allocation padded to round to nearest page size.
 */
int allocsize;

/*
 * Additional global variables may be added as needed below
 */

 
/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block (payload) on success.
 * Returns NULL on failure.
 *
 * This function must:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8 
 *   and possibly adding padding as a result.
 *
 * - Use BEST-FIT PLACEMENT POLICY to chose a free block
 *
 * - If the BEST-FIT block that is found is exact size match
 *   - 1. Update all heap blocks as needed for any affected blocks
 *   - 2. Return the address of the allocated block payload
 *
 * - If the BEST-FIT block that is found is large enough to split 
 *   - 1. SPLIT the free block into two valid heap blocks:
 *         1. an allocated block
 *         2. a free block
 *         NOTE: both blocks must meet heap block requirements 
 *       - Update all heap block header(s) and footer(s) 
 *              as needed for any affected blocks.
 *   - 2. Return the address of the allocated block payload
 *
 * - If a BEST-FIT block found is NOT found, return NULL
 *   Return NULL unable to find and allocate block for desired size
 *
 * Note: payload address that is returned is NOT the address of the
 *       block header.  It is the address of the start of the 
 *       available memory for the requesterr.
 *
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* myAlloc(int size) {     

    	//TODO: Your code goes in here.
    if(size <= 0 || size > allocsize){
	    return NULL;
    }

    //add 4 to size
    size += 4;

    //checks if multiple of 8
    if(size % 8 != 0){
	    size = size + 8 - (size % 8);
    }

    //blockHeader types for the current pointer and a pointer for the best spot in memory
    blockHeader *best = NULL;
    blockHeader *current = heapStart;

    // signifies that you found a place for the best fit block
    int flag = 0;

    //size of the current and best pointer
    int current_size = 0; 
    int best_size = 0;

    //loops through the current pointer to memory as long as it does not reach the end bit
    while(current -> size_status != 1){
	

        //updates size_status	    
	current_size = current -> size_status - current -> size_status % 8;
	// checks if the a-bit is free meaning this particular 
	// block of code is free. % 2 takes into consideration the last bit
	// odd is 1, even 0
	if(current -> size_status % 2 != 0 || current_size < size){
		//casts to void* which is a generic pointer, scale factor is 1
		current = (void*)current + current_size;
		//current_size = current -> size_status - current -> size_status % 8;
		continue;

	}

	//happens the first time to set things in place, initialize flag and set pointers
	if(flag==0){
		flag = 1;
		best = current;
		best_size = current_size;
		
	}

	//finds the smallest appropriate block of memory in heap for us to use
	if(current_size < best_size){
		best_size = current_size;
		best = current;
	}

	//jumps to next block unconditionally, not enough memory in this one
	current = (void*) current + current_size;
    }

    //if the flag is 0, meaning we have found no eligible blocks, we return NULL 
    if(flag == 0) return NULL;

    //the case for when the size is perfect for the data
    if(best_size == size){
	// set a block to 1
	best -> size_status += 1;

	//jumps to next block space
	blockHeader *new = (void*) best +  best_size;
	
	//as long as its not on the end bit 
	if(new -> size_status != 1){
		// set p block to 1
		new -> size_status += 2;
	}

	//returns the best ptr if the memory block is the exact size and then adds on the extra space for the header
	return (void*) best + sizeof(blockHeader); //why do we not just return best? wouldn't this take us to the beginning of the previous block?	
    }

    //if the size is too big and can be split up into an allocated block and a free block
	    blockHeader *new = (blockHeader*) ((void*) best + size); 

	    //increments size_status to increment a bit, and to update the size in the footer 
	    best -> size_status += size - best_size + 1;
	    
	    //size_status should be the p-bit + a-bit + size-of-block
	    //so this one, is unallocated, the previous is allocated, would 
	    //have a value of p + a + size = 2 + 0 + (best_size - size)
	    new -> size_status = 2 + 0 + (best_size - size);

	    //footer pointer, moves over to start of footer
	    blockHeader *new_footer = (void*) new + best_size - size - sizeof(blockHeader);

	    //footer holds only size of the memory space
	    new_footer -> size_status = best_size - size;

	   
	    //returns address of the payload
	    return (void*) best + sizeof(blockHeader);
} 
 
/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - Update header(s) and footer as needed.
 */                   
int myFree(void *ptr) {    
    //TODO: Your code goes in here.
    //
     //return -1 if ptr is NULL
    if(ptr == NULL){
            return -1;
    }

    //return -1 if ptr is not a multiple of 8
    if((int)ptr % 8 != 0){
            return -1;
    }

    
    //return -1 if ptr is outside of the heap space
    if(ptr < (void*)heapStart || ptr > (void*)heapStart + allocsize){
	    return -1;
    }

    //header variable
    blockHeader *header = ptr - sizeof(blockHeader);

    //checks if there is an A-Bit or not, if it is already freed, return -1
    if(header -> size_status % 2 == 0){
		return -1;
    }

    //takes out a bit 
    header -> size_status -= 1;

    //this jumps to next block so we can change the p-bit
    int block_size = header -> size_status - header -> size_status % 8;

    //new pointer that goes to the header of the next one
    blockHeader *new = (void*)header + block_size;

    //as long as its not at the end bit 
    if(new -> size_status != 1){

	    //checks p-bit status by &ing with 2 and decrements size status by it
	    int p = new -> size_status & 2;
	    new -> size_status -= p;
    }

    //this is the footer pointer
    blockHeader *footer = (void*)header + block_size - sizeof(blockHeader); 
    
    // changes the size of the footer 
    footer -> size_status = block_size;

    //returns 0 because successful
    return 0;
} 

/*
 * Function for traversing heap block list and coalescing all adjacent 
 * free blocks.
 *
 * This function is used for delayed coalescing.
 * Updated header size_status and footer size_status as needed.
 */
int coalesce() {
    //TODO: Your code goes in here.

	//creates a new pointer to the beginning of the heap
	blockHeader *ptr = heapStart;
	int ptr_size = 0;

	//while the end bit is not hit
	while(ptr -> size_status != 1){

		//update size_status for the ptr
		ptr_size = ptr -> size_status - ptr -> size_status % 8;
		//if there is an a bit we increment the size accordingly and keep looping
		if(ptr -> size_status & 1){
			ptr = (void*) ptr + ptr_size;
			continue;
		}

		//pointer to the next block
		blockHeader *next = (void*) ptr + ptr_size;

		//checks the a bit and if allocated, ptr is set to the next block
		if(next -> size_status & 1){
			ptr = next;
			continue;
                }

		//sets the next size
		int next_size = next -> size_status - next -> size_status % 8;

		//sets pointer to the next blocks footer 
		blockHeader *nextFooter = (void*) next + next_size - sizeof(blockHeader*);

		//updates current pointers size_status for the footer because now the size post-coalesce is larger
		ptr -> size_status += next_size;
		nextFooter -> size_status += ptr_size;
		ptr_size += next_size;

	}

	return 1;
}

 
/* 
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int myInit(int sizeOfRegion) {    
 
    static int allocated_once = 0; //prevent multiple myInit calls
 
    int pagesize;   // page size
    int padsize;    // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* endMark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }

    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    allocsize = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    allocsize -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heapStart = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    endMark = (blockHeader*)((void*)heapStart + allocsize);
    endMark->size_status = 1;

    // Set size in header
    heapStart->size_status = allocsize;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heapStart->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heapStart + allocsize - 4);
    footer->size_status = allocsize;
  
    return 0;
} 
                  
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void dispMem() {     
 
    int counter;
    char status[6];
    char p_status[6];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heapStart;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout, 
	"*********************************** Block List **********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "alloc");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "FREE ");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "alloc");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "FREE ");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
    fprintf(stdout, 
	"*********************************************************************************\n");
    fprintf(stdout, "Total used size = %4d\n", used_size);
    fprintf(stdout, "Total free size = %4d\n", free_size);
    fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
    fprintf(stdout, 
	"*********************************************************************************\n");
    fflush(stdout);

    return;  
} 


// end of myHeap.c (sp 2021)                                         

