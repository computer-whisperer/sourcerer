#include "membox.h"

int main() {
  struct Membox_T * membox = build_membox(500);
  char * buff_a = membox_malloc(membox, 100);
  char * buff_b = membox_malloc(membox, 100);
  char * buff_c = membox_malloc(membox, 50);
  membox_free(membox, buff_b);
  buff_b = membox_malloc(membox, 25);
  char * buff_d = membox_malloc(membox, 25);
  membox_free(membox, buff_c);
  membox_free(membox, buff_a);
  membox_free(membox, buff_b);
  membox_free(membox, buff_c);
}
