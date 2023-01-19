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

struct lisp_hash {
  /**
     @sum:  the weighted numeric sum of the symbol
              - the hash index is gotten by modulating the
                field with the `SYMTAB_PRIM' macro
   */
  uint   sum;

  uint   psum; /** @psum: the numeric sum of the symbol, without weights */
  ushort len;  /** @len:  the length of the symbol */

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
};

enum lisp_sym_typ {
  __LISP_SYM,
  __LISP_FUN,
};

/** NOTE: the implementations of this LISP are *purely symbolic*,
          that means no integer, string, float, or whatever other
          literal type you're used to are implemented in this language.
          for those in a LISP, see my other project GPLD:

                     https://github.com/matthmr/gpld

          it uses VSL as the stage 0 of its bootstrap and is
          forwards-compatible with (some) VSL code
*/
struct lisp_sym {
  struct lisp_hash  hash; /** @hash:  the hash of the symbol */

  void*             dat;  /** @dat:  data for the symbol     */
  enum lisp_sym_typ typ;  /** @typ:  type of the symbol      */

  uint litr[2];
  uint size[2];
};

struct lisp_hash_ret {
  struct lisp_hash master;
  int slave;
};

struct lisp_sym_ret {
  struct lisp_sym master;
  int slave;
};

struct clisp_sym {
  const char* str; /** @str: the C-string representation of the symbol   */
  const char* fun; /** @fun: the C-string representation of the function */

  /** @tab: will be filled at runtime via the `frontend' function        */
  struct lisp_sym sym;
};

struct lisp_hash_ret inc_hash(struct lisp_hash hash, char c);
struct lisp_hash_ret str_hash(const char* str);
void inc_hash_done(void);
void hash_done(struct lisp_hash* hash);

struct lisp_sym_ret lisp_symtab_set(struct lisp_hash hash);
struct lisp_sym_ret lisp_symtab_get(struct lisp_hash hash);
#endif

#ifndef LOCK_SYMTAB_INTERNALS
#  define LOCK_SYMTAB_INTERNALS

#  define POOL_ENTRY_T  struct lisp_sym
#  define POOL_AM       SYMPOOL

#  include "pool.h"

#endif
