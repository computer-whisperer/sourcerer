#include <stdlib.h>

#include "membox.h"

struct Membox_T * build_membox(size_t len) {
  struct Membox_T * membox = malloc(sizeof(struct Membox_T));
  membox->data_start = malloc(len);
  membox->data_end = membox->data_start + len;
  membox->seed = 4819;
  
  // Deterministically fuzz the memory region
  char * ptr = membox->data_start;
  while ((void *)ptr < membox->data_end) {
    *ptr = 101;
    ptr++;
  }

  membox->first_freeblock = malloc(sizeof(struct MemboxFreeblock_T));
  membox->last_freeblock = membox->first_freeblock;
  membox->first_freeblock->data_start = membox->data_start;
  membox->first_freeblock->data_end = membox->data_end;
  membox->first_freeblock->next = NULL;
  membox->first_freeblock->prev = NULL;
  return membox;
}

struct Membox_T * reset_membox(struct Membox_T * membox) {
  // Free all freeblocks
  struct MemboxFreeblock_T * freeblock = membox->first_freeblock;
  while (freeblock) {
    struct MemboxFreeblock_T * tofree = freeblock;
    freeblock = freeblock->next;
    free(tofree);
  }
  
  membox->seed = 4819;
  
  // Add original freeblock
  membox->first_freeblock = malloc(sizeof(struct MemboxFreeblock_T));
  membox->last_freeblock = membox->first_freeblock;
  membox->first_freeblock->data_start = membox->data_start;
  membox->first_freeblock->data_end = membox->data_end;
  membox->first_freeblock->next = NULL;
  membox->first_freeblock->prev = NULL;
  return membox;
}

void * membox_malloc(struct Membox_T * membox, size_t len) {
  struct MemboxFreeblock_T * freeblock = membox->first_freeblock;
  while (freeblock) {
    size_t freeblock_size = freeblock->data_end - freeblock->data_start;
    if (freeblock_size >= len + sizeof(struct MemboxAllocatedheader_T)) {
      
      // Use this block
      struct MemboxAllocatedheader_T * header = (struct MemboxAllocatedheader_T *) freeblock->data_start;
      header->header_start = freeblock->data_start;
      header->data_start = freeblock->data_start + sizeof(struct MemboxAllocatedheader_T);
      header->data_end = header->data_start + len;
      header->magic_start = 0x4819;
      header->magic_end = 0x9184;
      
      // Fix free block
      freeblock->data_start = header->data_end;
      if (freeblock->data_start >= freeblock->data_end) {
        // This free block is empty, and should be removed.
        if (freeblock->prev)
          freeblock->prev->next = freeblock->next;
        else
          membox->first_freeblock = freeblock->next;
        
        if (freeblock->next)
          freeblock->next->prev = freeblock->prev;
        else
          membox->last_freeblock = freeblock->prev;
        
        free(freeblock);
      }
      
      // Deterministically fuzz the memory region
      
      char * ptr = header->data_start;
      while ((void *)ptr < header->data_end) {
        //membox->seed = (214013*membox->seed+2531011);
        //*ptr = (membox->seed>>16)&0x7FFF;
        *ptr = 101;
        ptr++;
      }
      
      return header->data_start;
    }
    freeblock = freeblock->next;
  }
  return NULL;
}

int membox_free(struct Membox_T * membox, void * data_start) {
  struct MemboxAllocatedheader_T * header = data_start - sizeof(struct MemboxAllocatedheader_T);
  if (header->magic_start != 0x4819 || header->magic_end != 0x9184)
    return 1; // Memory corrupted, failure
  struct MemboxFreeblock_T * freeblock = malloc(sizeof(struct MemboxFreeblock_T));
  freeblock->data_start = header->header_start;
  freeblock->data_end = header->data_end;
  
  // Find freeblock->next
  freeblock->next = membox->first_freeblock;
  while (freeblock->next && freeblock->next->data_start < freeblock->data_start)
    freeblock->next = freeblock->next->next;
  
  // Find freeblock->prev
  if (freeblock->next)
    freeblock->prev = freeblock->next->prev;
  else
    freeblock->prev = membox->last_freeblock;
  
  // Set other links appropriately
  if (freeblock->next)
    freeblock->next->prev = freeblock;
  else
    membox->last_freeblock = freeblock;
    
  if (freeblock->prev)
    freeblock->prev->next = freeblock;
  else
    membox->first_freeblock = freeblock;
  
  // Check for merging with previous freeblock
  if (freeblock->prev && freeblock->prev->data_end == freeblock->data_start) {
    struct MemboxFreeblock_T * freeblock_tofree = freeblock->prev;
    freeblock->data_start = freeblock->prev->data_start;
    // Patch linked list
    freeblock->prev = freeblock->prev->prev;
    if (freeblock->prev)
      freeblock->prev->next = freeblock;
    else
      membox->first_freeblock = freeblock;
    // Free block
    free(freeblock_tofree);
  }
  
  // Check for merging with next freeblock
  if (freeblock->next && freeblock->next->data_start == freeblock->data_end) {
    struct MemboxFreeblock_T * freeblock_tofree = freeblock->next;
    freeblock->data_end = freeblock->next->data_end;
    // Patch linked list
    freeblock->next = freeblock->next->next;
    if (freeblock->next)
      freeblock->next->prev = freeblock;
    else
      membox->last_freeblock = freeblock;
    // Free block
    free(freeblock_tofree);
  }
  return 0; // Success
}

void demolish_membox(struct Membox_T * membox) {
  // free data region
  free(membox->data_start);
  // Free all freeblocks
  struct MemboxFreeblock_T * freeblock = membox->first_freeblock;
  while (freeblock) {
    struct MemboxFreeblock_T * tofree = freeblock;
    freeblock = freeblock->next;
    free(tofree);
  }
  // Free membox struct
  free(membox);
}
