#ifndef LOCK_SYMTAB
#  define LOCK_SYMTAB

#  include "utils.h"

// elements per hash cell
#  ifndef SYMPOOL
#    define SYMPOOL (5)
#  endif

// hash prime
#  ifndef SYMTAB_PRIM
#    define SYMTAB_PRIM  (61)
#  endif

// hash cells per table
#  ifndef SYMTAB_CELL
#    define SYMTAB_CELL  (60)
#  endif

/** NOTE: this is a sacrifice: the hashing algorithm is pretty simple, so we
          need to fix the length of the identifiers. 94 characters should be
          enough, for example:

          aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
 */
// maximum number of characters per symbol
#  define SYMTAB_MAX_SYM (94)

#  define ASCII_FAC (94)

#  define ASCII_NORM(x) \
  ((x) -= 32)

#  define FOR_EACH_TABENT(i,x) \
  for (uint (i) = 0; (i) < (x); ++(i))

#  define HASH_IDX(x) \
  ((x).sum % SYMTAB_CELL)

struct lisp_hash {
  uint   sum;  /** @sum:  the weighted numeric sum of the symbol
                     - the hash index is gotten by modulating the
                       field with the `SYMTAB_CELL' macro */

  uint   psum; /** @psum: the numeric sum of the symbol, without weights */
  ushort len;  /** @len:  the length of the symbol */

  ushort com_part; /** @com_part: a fail-safe for the hash algorithm */
};

enum lisp_sym_typ {
  __LISP_NULL = 0,

  __LISP_SYM,
  __LISP_FUN,
  __LISP_CLISP_SYM,
  __LISP_CLISP_FUN,
};

#  define INFINITY (-1u)

/** NOTE: the implementations of this LISP are *purely symbolic*,
          that means no integer, string, float, or whatever other
          literal type you're used to are implemented in this language.
          for those in a LISP, see my other project GPLD:

                     https://github.com/matthmr/gpld

          it uses VSL as the stage 0 of its bootstrap and is
          forwards-compatible with (some) VSL code
*/
struct lisp_sym {
  struct lisp_hash  hash; /** @hash: the hash of the symbol */

  void*             dat;  /** @dat:  data for the symbol    */
  enum lisp_sym_typ typ;  /** @typ:  type of the symbol     */

  uint litr[2];
  uint size[2];
};

////////////////////////////////////////////////////////////////////////////////

struct clisp_sym {
  const char*     str; /** @str: the C-string representation of the symbol   */
  struct lisp_sym sym; /** @sym: the symbol template for the symtab          */
};

////////////////////////////////////////////////////////////////////////////////

struct lisp_hash_ret {
  struct lisp_hash master;
  int slave;
};

struct lisp_sym_ret {
  struct lisp_sym* master; // TODO: see `stack.c's TODO
  int slave;
};

struct lisp_hash_ret inc_hash(struct lisp_hash hash, char c);
struct lisp_hash_ret str_hash(const char* str);
void inc_hash_done(struct lisp_hash* hash);
void hash_done(struct lisp_hash* hash);

// TODO: right now, these functions take the `symtab` variable globally.
//       make them take it as an argument
int lisp_symtab_set(struct lisp_sym sym);
struct lisp_sym_ret lisp_symtab_get(struct lisp_hash hash);

int symtab_init(void);
#endif

#ifndef LOCK_SYMTAB_INTERNALS
#  define LOCK_SYMTAB_INTERNALS

#  define POOL_ENTRY_T struct lisp_sym
#  define POOL_AM      SYMPOOL

#  define LOCK_POOL_THREAD
#  include "pool.h"

#  ifndef PROVIDE_SYMTAB_TABDEF
extern
#  endif
POOL_T symtab[SYMTAB_CELL]

#  ifdef PROVIDE_SYMTAB_TABDEF
= {0};
#  else
;
#  endif

extern POOL_T* symtab_pp[SYMTAB_CELL];

#endif
