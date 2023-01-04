#ifndef LOCK_SYMTAB
#  define LOCK_SYMTAB

#  include "utils.h"

#  ifndef SYMPOOL
#    define SYMPOOL (5)
#  endif

#  ifndef SYMTAB_PRIM
#    define SYMTAB_PRIM  (97)
#  endif

#  define SYMTAB_MAX_SYM (SYMTAB_PRIM*SYMTAB_PRIM)

#  define ASCII_FAC (32)

union lisp_symtab_sym {
  void* func;
  void* sym;
};

enum lisp_symtab_typ {
  __LISP_FUN = BIT(0),
  __LISP_SYM = BIT(1),
};

struct lisp_symtab {
  /**
     @sum:  the weighted numeric sum of the symbol
              - the hash index is gotten by modulating the
                field with the `SYMTAB_PRIM' macro
     @psum: the numeric sum of the symbol, without weights
     @len:  the length of the symbol
   */
  uint   sum, psum;
  ushort len;

  /**
     @com_part: the sequential partitional factor of the
                term after a modular zero. See example.
     @com_pos:  the position for the field(s) above

     EXAMPLE
     -------

     For prime 5:
       - 2*1 + 5*2 + 3*3 -> @com_part = 3, @com_pos = 3
       - 2*1 + 5*2       -> @com_part = 5, @com_pos = 2
       - 4*1 + 2*2 + 4*3 -> @com_part = 4, @com_pos = 3
       - 5*1 + 5*2       -> @com_part = 0, @com_pos = 0
   */
  uchar com_part, com_pos;

  // TODO: create a datatype for LISP functions
  //       that doesn't need to be casted
  /**
     @typ: type of the symbol
     @dat: data for the symbol, needs to be casted
   */
  enum lisp_symtab_typ  typ;
  union lisp_symtab_sym dat;
};

struct clisp_symtab {
  const char* str;

  /** @hash: will be filled at runtime via the
             `frontend' exported function
  */
  struct lisp_symtab hash;
};

int  do_chash(struct lisp_symtab* chash, char c);
int  lisp_symtab_set(struct lisp_symtab* chash);
int  lisp_symtab_get(struct lisp_symtab* chash);
void done_chash(void);
#endif

#ifndef LOCK_SYMTAB_INTERNALS
#  define LOCK_SYMTAB_INTERNALS

#  define POOL_ENTRY_T  struct lisp_symtab
#  define POOL_AM       SYMPOOL

#  include "pool.h"

#endif
