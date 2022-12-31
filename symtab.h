#ifndef LOCK_SYMTAB
#  define LOCK_SYMTAB

#  include "pool.h"

#  ifndef SYMPOOL
#    define SYMPOOL (10)
#  endif

#  ifndef SYMTAB
#    define SYMTAB  (53)
#  endif

union lisp_symtab_sym {
  void* func;
  void* sym;
};

enum  lisp_symtab_typ {
  __LISP_FUN,
  __LISP_SYM,
};

struct lisp_symtab_raw {
  const char* str;
  ulong       hash;

  enum  lisp_symtab_typ typ;
  union lisp_symtab_sym sym;
};

struct lisp_symtab {
  char c;
};

extern const struct lisp_symtab_raw*
  symtab_raw;
extern struct MEMPOOL_TMPL(symtab)*
  symtab;

ulong do_chash(ulong chash, char c);
ulong done_chash(ulong chash);

#endif
