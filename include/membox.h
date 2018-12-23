#include <stdlib.h>

#ifndef MEMBOX_H
#define MEMBOX_H

struct Membox_T {
  void * data_start;
  void * data_end;
  
  unsigned int seed;
  
  struct MemboxFreeblock_T * first_freeblock;
  struct MemboxFreeblock_T * last_freeblock;
};

struct MemboxAllocatedheader_T {
  int magic_start;
  void * header_start;
  void * data_start;
  void * data_end;
  int magic_end;
};

struct MemboxFreeblock_T {
  void * data_start;
  void * data_end;
  struct MemboxFreeblock_T * next;
  struct MemboxFreeblock_T * prev;
};

struct Membox_T * build_membox(size_t len);
struct Membox_T * reset_membox(struct Membox_T * membox);
void * membox_malloc(struct Membox_T * membox, size_t len);
int membox_free(struct Membox_T * membox, void * data_start);

#endif /* MEMBOX_H */
