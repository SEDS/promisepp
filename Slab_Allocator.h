//Author: Nyalia Lui
//Professor: Dr. Fengguang Song
//Course: CSCI503000
//Due: Thursday December 7 @ 11:59pm
// allocator library.

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstdlib>

//a single chunk of memory
struct chunk
{
  int is_used;
  void* data;
};

//a 1MB slab of memory
struct slab
{
  size_t chunk_size;
  size_t num_chunks;
  int has_space;
  struct chunk* chunks;
  struct slab* next;
  struct slab* prev;
};

//slab allocator
struct slab_allocator
{
  int avail_slabs;
  int possible_slabs;
  void* pool;
  size_t pool_size;
  size_t first_free_byte;
  struct slab* slab_head;
  struct slab* slab_tail;
};

//get_number_chunks - procedure that returns number
//of chunks given chunk size.
size_t get_number_chunks (size_t);

//get_chunk_size - procedure that returns the chunk size
//given the last slab class.
size_t get_chunk_size (size_t);

//number_slab_classes - procedure that returns
//total number of SlabClasses given memory pool
//size.
int number_slab_classes (size_t);

//is_multiple8 - procedure that returns 1 when
//given address is a multiple of 8.
int is_multiple8(void*);

//malloc_bytes - procedure that allocates a block of
//size bytes from given memory pool and returns the
//address of first block.
void* malloc_bytes(void*, size_t*, size_t);

//remainder_size - procedure that returns the number of bytes
//left over since no chunk size evenly divides into 1MB.
size_t remainder_size(struct slab*);

//create_chunks - procedure that creates a slab's
//memory chunks;
void create_chunks (struct slab_allocator*, struct slab*);

//create_slab - procedure that creates a single slab
//for a class with divided chunks.
void create_slab (struct slab_allocator*, size_t, int);

//create_slabs - procedure that creates all
//the slabs for each class with chunks not divided yet.
void create_slabs (struct slab_allocator*, int);

//init_slab_allocator - procedure that creates a slab
//allocator with designated size of memory.
void init_slab_allocator (struct slab_allocator*, size_t);

//closest_chunk_size - procedure that returns the chunks size
//which given number of bytes is closest to but upper end.
//returns 573760 if number of bytes >= 573760.
size_t closest_chunk_size(int);

//pick_slab - procedure that returns an address to slab of memory
//to pick a chunk from. Retruns NULL if not enough slabs.
struct slab* pick_slab (struct slab_allocator*, int);

//pick_chunk - procedure that returns an address to a chunk of memory
//that has been removed from given slab.
struct chunk* pick_chunk (struct slab*);

//alloc_mem - procedure that returns a chunk of memory
//with room for specified number of bytes. Retun NULL if error.
void* alloc_mem (struct slab_allocator*, int);

//remove_if_belong_here - procedure that returns 0 if the chunk
//of memory belongs to the given slab and is removed. Otherwise -1.
int remove_if_belong_here (struct slab*, void*);

//free_mem - procedure that frees a chunk of memory
//back to the slab allocator
int free_mem(struct slab_allocator*, void*);

//free_pool - actually free the memory
//in the slab allocator.
void free_pool(struct slab_allocator*);

#endif // !ALLOCATOR_H
