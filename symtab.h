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
#    define SYMTAB_CELL  (64)
#  endif

/** NOTE: this is a sacrifice: the hashing algorithm is pretty simple, so we
          need to fix the length of the identifiers. 94 characters should be
          enough, for example:

          the-quick-brown-fox-jumps-over-the-lazy-dog-can-fit-in-the-name-and-i-still-got-some-size-left
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

enum lisp_hash_mask {
  __LISP_HASH_sum      = BIT(0),
  __LISP_HASH_psum     = BIT(1),
  __LISP_HASH_len      = BIT(2),
  __LISP_HASH_com_part = BIT(3),
};

struct lisp_hash {
  uint   sum;  /** @sum:  the weighted numeric sum of the symbol
                     - the hash index is gotten by modulating the
                       field with the `SYMTAB_CELL' macro */

  uint   psum; /** @psum: the numeric sum of the symbol, without weights */
  ushort len;  /** @len:  the length of the symbol */

  ushort com_part; /** @com_part: a fail-safe for the hash algorithm */
  enum lisp_hash_mask rep; /** @rep: repetition mask for `get' functions */
};

enum lisp_sym_typ {
  __LISP_VAR_NO_TYPE = 0,

  __LISP_CLISP_SYM,
  __LISP_CLISP_FUN,

  __LISP_VAR_GEN,

  __LISP_VAR_SYM,
  __LISP_VAR_SEXP,
  __LISP_VAR_LEXP,

  __LISP_VAR_SYMP,
  __LISP_VAR_HASH,

  __LISP_VAR_FUN,
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

  uint          size[2];  /** @size: argument range; masked */
  uint          litr[2];  /** @litr: literal range: < size  */
};

////////////////////////////////////////////////////////////////////////////////

struct sort_t;

typedef bool (*in_between_t) (struct lisp_sym* ppm, struct lisp_hash hash,
                              uint lower, uint upper);
typedef bool (*ex_between_t) (struct lisp_sym* ppm, struct lisp_hash hash,
                              uint lower, uint upper);
typedef bool (*repeats_t)    (const struct sort_t* sort);
typedef uint (*yield_t)      (struct lisp_sym* ppm, uint i);
typedef bool (*eq_t)         (uint n, struct lisp_hash hash);
typedef bool (*lt_t)         (uint n, struct lisp_hash hash);

struct sort_t {
  in_between_t  in_between;
  ex_between_t  ex_between;
  repeats_t     repeats;
  yield_t       yield;
  eq_t          eq;
  lt_t          lt;

  enum lisp_hash_mask mask;
  struct sort_t* next;
};

////////////////////////////////////////////////////////////////////////////////

struct clisp_sym {
  const char*     str; /** @str: the C-string representation of the symbol */
  struct lisp_sym sym; /** @sym: the symbol template for the symtab        */
};

////////////////////////////////////////////////////////////////////////////////

struct lisp_hash_ret {
  struct lisp_hash master;
  int slave;
};

struct lisp_sym_ret {
  struct lisp_sym* master;
  int slave;
};

/** used for stack calls:
    - the ret types @p and @pv are needed to change the value of the
      symbol directly
    - the value @m is used for speed ups (no need to dereference)
*/
struct lisp_symc_ret {
  struct lisp_sym_ret p, pv;
  struct lisp_sym     m;
};

enum lisp_sort_stat {
  __SORT_RETURN = -1,
  __SORT_OK     =  0,
  __SORT_NEXT   =  1,
};

struct lisp_sort_ret {
  uint master;
  enum lisp_sort_stat slave;
};

struct lisp_hash_ret inc_hash(struct lisp_hash hash, char c);
struct lisp_hash_ret str_hash(const char* str);
void inc_hash_done(struct lisp_hash* hash);
void hash_done(struct lisp_hash* hash);

// TODO: right now, these functions take the `symtab` variable globally.
//       make them take it as an argument
int lisp_symtab_set(struct lisp_sym sym);
struct lisp_sym_ret lisp_symtab_get(struct lisp_hash hash);

int symtab_init(bool force);
#endif

#ifndef LOCK_SYMTAB_INTERNALS
#  define LOCK_SYMTAB_INTERNALS

#  define POOL_ENTRY_T struct lisp_sym
#  define POOL_AM      SYMPOOL

#  include "pool.h"

extern POOL_T* symtab;

struct lisp_symtab_pp {
  POOL_T* mem;
  POOL_T* base;
};

extern struct lisp_symtab_pp symtab_pp[SYMTAB_CELL];

#endif
