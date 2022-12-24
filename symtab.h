#ifndef LOCK_SYMTAB
#  define LOCK_SYMTAB

typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char bool;

struct lisp_hash_body {
  ulong hash;
  char  cmask[sizeof(ulong)];
};

struct lisp_hash {
  uint len;
  struct lisp_hash_body body;
};

struct lisp_hash_body do_chash(struct lisp_hash_body body,
                               int i, char c);

#endif
