#include "symtab.h"

struct lisp_hash_body do_chash(struct lisp_hash_body body,
                               int i, char c) {
  ulong hash_byt = ((c + i) % 0x100);

  i         %= 8;
  hash_byt <<= (i*8);

  body.hash     |= hash_byt;
  body.cmask[i]  = c;

  return body;
}
