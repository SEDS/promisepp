#include "Slab_Allocator.h"
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstring>
#define MEGA_BYTE 1048576
#define EXA_BYTE 1152921504606846976

//get_number_chunks - procedure that returns number
//of chunks given chunk size.
size_t get_number_chunks (size_t chunk_size)
{
  return (size_t) floor(MEGA_BYTE / (chunk_size * 1.0));
}

//get_chunk_size - procedure that returns the chunk size
//given the last slab class.
size_t get_chunk_size (size_t last_class_size)
{
  size_t new_size = (1.25 * last_class_size);
  size_t rem = new_size % 8;

  if (rem > 4)
    new_size = new_size + (8 - rem);
  else
    new_size = new_size - rem;

  return new_size;
}

//number_slab_classes - procedure that returns
//total number of SlabClasses given memory pool
//size.
int number_slab_classes (size_t mem_pool_size)
{
  if (mem_pool_size < (41*MEGA_BYTE))
    return 0;

  return (size_t) ceil(mem_pool_size / (MEGA_BYTE * 1.0));
}

//is_multiple8 - procedure that returns 1 when
//given address is a multiple of 8.
int is_multiple8(void* address)
{
  return ((unsigned long)address % 8 == 0);
}

//malloc_bytes - procedure that allocates a block of
//size bytes from given memory pool and returns the
//address of first block.
void* malloc_bytes(void* pool, size_t* first_free_byte, size_t size_request)
{
	size_t free_byte = (*first_free_byte);
	*first_free_byte = free_byte + size_request;

	return (void*)((char*)pool + free_byte);
}

//remainder_size - procedure that returns the number of bytes
//left over since no chunk size evenly divides into 1MB.
size_t remainder_size(struct slab* my_slab)
{
  return (MEGA_BYTE-(my_slab->num_chunks *  my_slab->chunk_size));
}

//create_chunks - procedure that creates a slab's
//memory chunks;
void create_chunks(struct slab_allocator* alloc, struct slab* my_slab)
{
  size_t i=0;
  for(; i<my_slab->num_chunks; i++)
  {
    my_slab->chunks[i].is_used = 0;
    my_slab->chunks[i].data = malloc_bytes(alloc->pool, &alloc->first_free_byte, my_slab->chunk_size);
  }

  //move the first free byte marker over by remainder
  //number of bytes. this way, the last chunk takes up
  //the left over bytes.
  alloc->first_free_byte += remainder_size(my_slab);
}

//create_slab - procedure that creates a single slab
//for a class with divided chunks. position 0 means slab is head.
void create_slab (struct slab_allocator* alloc, size_t csize, int position)
{
  struct slab* temp_slab = NULL;

  //create a new slab.
  temp_slab = (struct slab*) malloc_bytes(alloc->pool, &alloc->first_free_byte, sizeof(struct slab));
  // printf("%d | ", csize);
  temp_slab->chunk_size = csize;
  temp_slab->num_chunks = get_number_chunks(csize);
  // printf("%d\n", temp_slab->num_chunks);
  temp_slab->chunks = (struct chunk*) malloc_bytes(alloc->pool, &alloc->first_free_byte, temp_slab->num_chunks * sizeof(struct chunk));
  temp_slab->has_space = 1;

  //divide head slab into its proper chunks
  create_chunks (alloc, temp_slab);

  //add slab to the double linked list.
  //head slab which has chunk size 80
  //must be added properly though.
  temp_slab->next = NULL;

  if (position == 0)
  {
    temp_slab->prev = NULL;
    alloc->slab_head = temp_slab;
  }
  else
  {
    temp_slab->prev = alloc->slab_tail;
    alloc->slab_tail->next = temp_slab;
  }

  alloc->slab_tail = temp_slab;
  alloc->avail_slabs = alloc->avail_slabs + 1;
}

//create_slabs - procedure that creates all
//the slabs for each class with chunks not divided yet.
void create_slabs (struct slab_allocator* alloc, int TN)
{
  int i=0;

  size_t csize = 80;
  create_slab(alloc, csize, 0);

  for(i=1; i<TN; i++)
  {
    csize = get_chunk_size(csize);
    // printf("%d | ", i);
    create_slab(alloc, csize, 3);
  }
}

//init_slab_allocator - procedure that creates a slab
//allocator with designated size of memory.
void init_slab_allocator (struct slab_allocator* alloc, size_t mem_pool_size)
{

  if (alloc == NULL)
  {
    printf("init_slab_allocator failed. given allocator is NULL.\n");
    return;
    //exit (EXIT_FAILURE);
  }

  if (mem_pool_size < MEGA_BYTE)
  {
    printf("init_slab_allocator failed. mem_pool_size is less than 1MB.\n");
    return;
    //exit (EXIT_FAILURE);
  }

  //determine number of classes needed and total memory needed in 1 malloc.
  int TN = number_slab_classes (mem_pool_size);
  if(TN == 0)
  {
    printf("init_slab_allocator failed. memory pool must be 41MB or greater.\n");
    return;
    // exit (EXIT_FAILURE);
  }
  else if (TN > 41)
  {
    alloc->possible_slabs = TN;
    TN = 41;
  }
  else
    alloc->possible_slabs = TN;

  alloc->avail_slabs = 0;

  //since the initial allocator MUST have 41 classes with 1 slab each, then we
  //know there will be 67377 available chunks to start out with.
  //this number is the sum of all chunk sizes of each class.
  size_t init_chunk_count = 67377;

  //maximum chunks needed for the potential extra slabs would be
  //from the class 1 needing to take ALL the memory. This is because
  //class 1 has the highest number of chunks (13107) in a single slab.
  size_t max_extra_chunks = (alloc->possible_slabs - 41) * 13107;

  size_t total_slab_size = (alloc->possible_slabs * sizeof(struct slab));
  size_t total_chunk_size = ((init_chunk_count + max_extra_chunks) * sizeof(struct chunk));
  // printf("-- %d -- %d\n", (init_chunk_count + max_extra_chunks), max_extra_chunks);
  size_t total_data_size = (alloc->possible_slabs * MEGA_BYTE);
  size_t all_size = total_slab_size + total_chunk_size + total_data_size;

  if(all_size > EXA_BYTE)
  {
    printf("init_slab_allocator failed. Memory request exceeded 1Exabyte.\n");
    exit (EXIT_FAILURE);
  }

  //allocate all the memory needed.
  void* all = malloc (all_size);
  if (all == NULL)
  {
    printf("init_slab_allocator failed. Malloc failed.\n");
    exit (EXIT_FAILURE);
  }

  alloc->pool = all;
  alloc->pool_size = all_size;
  alloc->first_free_byte = 0;
  alloc->slab_head = NULL;
  alloc->slab_tail = NULL;

  //create slabs with divided chunks.
  create_slabs (alloc, TN);
}

//closest_chunk_size - procedure that returns the chunks size
//which given number of bytes is closest to but upper end.
//returns 573760 if number of bytes >= 573760.
size_t closest_chunk_size(int num_bytes)
{
  if(num_bytes >= 573760)
    return 573760;

  size_t csize=80;
  while (num_bytes > csize)
    csize = get_chunk_size(csize);

  return csize;
}

//pick_slab - procedure that returns an address to slab of memory
//to pick a chunk from. Retruns NULL if not enough slabs.
struct slab* pick_slab (struct slab_allocator* alloc, int num_bytes)
{
  struct slab* picked = alloc->slab_head;

  size_t up_bound = closest_chunk_size(num_bytes);

  while(picked != NULL && !(picked->chunk_size == up_bound && picked->has_space == 1))
  {
    picked = picked->next;
  }

  return picked;
}

//pick_chunk - procedure that returns an address to a chunk of memory
//that has been removed from given slab.
struct chunk* pick_chunk (struct slab* my_slab)
{
  struct chunk* picked = NULL;
  size_t i=0;

  for(;i<my_slab->num_chunks; i++)
  {
    if (my_slab->chunks[i].is_used == 0)
    {
      my_slab->chunks[i].is_used = 1;
      //set has_space to 0 as long as we didn't use the last chunk
      if (i == my_slab->num_chunks - 1)
        my_slab->has_space = 0;

      picked = &my_slab->chunks[i];
      //set termination condition
      i = my_slab->num_chunks;
    }
  }

  return picked;
}

//alloc_mem - procedure that returns a chunk of memory
//with room for specified number of bytes. Retun NULL if error.
void* alloc_mem (struct slab_allocator* alloc, int num_bytes)
{
  if(alloc == NULL)
  {
    printf("alloc_mem failed. Given slab allocator is NULL.\n");
    return NULL;
  }

  if(num_bytes <= 0)
  {
    printf("alloc_mem failed. Request for 1 or more bytes only.\n");
    return NULL;
  }

  //pick a slab to allocate memory from
  struct slab* picked_slab = pick_slab (alloc, num_bytes);

  //create a new slab if the current class doesn't have any free slabs
  //and there's still enough memory in the pool.
  if ((picked_slab == NULL) && (alloc->avail_slabs < alloc->possible_slabs))
  {
    size_t csize = closest_chunk_size(num_bytes);
    create_slab(alloc, csize, 3);
    picked_slab = pick_slab (alloc, num_bytes);
    if (picked_slab == NULL)
    {
      printf("alloc_mem failed. Couldn't create a new slab.\n");
      return NULL;
    }
  }
  //what to do when there isn't any memory left.
  else if (picked_slab == NULL)
  {
    printf("alloc_mem failed. Not enough memory to satisfy request.\n");
    return NULL;
  }

  //otherwise a slab was found or created.

  //pick a chunk of memory from that slab
  struct chunk* picked_chunk = pick_chunk (picked_slab);
  if (picked_chunk == NULL)
  {
    printf("alloc_mem failed. No more chunks!\n");
    return NULL;
  }

  void* memory = picked_chunk->data;

   return memory;
}

//remove_if_belong_here - procedure that returns 0 if the chunk
//of memory belongs to the given slab and is removed. Otherwise -1.
int remove_if_belong_here (struct slab* my_slab, void* ptr)
{
  int found = -1;
  size_t i=0;
  for(;i<my_slab->num_chunks; i++)
  {
    if((my_slab->chunks[i].data == ptr) && (my_slab->chunks[i].is_used == 1))
    {
      my_slab->chunks[i].is_used = 0;
      found = 0;
      i=my_slab->num_chunks;
    }
  }

  return found;
}

//free_mem - procedure that frees a chunk of memory
//back to the slab allocator
int free_mem(struct slab_allocator* alloc, void* ptr)
{
  int found = -1;
  if (alloc == NULL)
  {
    printf("free_mem failed. Allocator is NULL.\n");
    return -1;
  }

  if (ptr == NULL)
  {
    printf("free_mem failed. 2nd argument is NULL.\n");
    return -1;
  }

  struct slab* temp_slab = alloc->slab_head;

  //determine if this chunk belongs to this slab.
  while((temp_slab != NULL) && (found < 0))
  {
    found = remove_if_belong_here(temp_slab, ptr);

    if(found < 0)
      temp_slab = temp_slab->next;
  }


  //after we'er done freeing, make sure their pointer is
  //NULL. this way they don't try to do anything.
  if (found < 0)
    printf("free_mem failed.\n");
  else
  {
    memset (ptr, 0, temp_slab->chunk_size);
    ptr = NULL;
  }

  return found;
}

//free_pool - actually free the memory
//in the slab allocator.
void free_pool(struct slab_allocator* alloc)
{
  free(alloc->pool);
  alloc->slab_head = NULL;
  alloc->slab_tail = NULL;
}
