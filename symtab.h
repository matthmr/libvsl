#ifndef LOCK_SYMTAB
#  define LOCK_SYMTAB

#  include "utils.h"

// hash cells per table
#  ifndef SYMTAB_CELLS
#    define SYMTAB_CELLS (94)
#  endif

// entries per hash cell
#  ifndef SYMTAB_ENTRIES
#    define SYMTAB_ENTRIES (5)
#  endif

// hash prime
#  ifndef SYMTAB_PRIM
#    define SYMTAB_PRIM  (103)
#  endif

/**
   maximum number of characters per symbol

   NOTE
   ----
   this is a sacrifice: the hashing algorithm is pretty simple, so we need to
   fix the length of the identifiers. 94 characters should be enough,
   for example:

   the-quick-brown-fox-jumps-over-the-lazy-dog-can-fit-in-the-name-and-i-still-got-some-size-left

   is 94 characters. Can you remember a name that long? I don't think so.
 */
#  define SYMTAB_MAX_SYM_LEN (94)

#  define ASCII_FAC          (32)

#  define SUM_OVER(x)    (x)*((x)+1)/2
#  define ASCII_NORM(x)  ((x) -= 32)
#  define HASH_IDX(x)    ((x).sum % SYMTAB_CELLS)
#  define FOR_EACH_TABENT(i,x) for (uint (i) = 0; (i) < (x); ++(i))

#  define LITERAL(i,litr) \
  (((i) >= (litr)[0]) && ((litr)[1] == INFINITY || ((i) <= (litr)[1])))

////////////////////////////////////////////////////////////////////////////////

// what lack of classes does to a mfr...
#define SORT_FUNC_FOR(x) \
  static inline bool in_between__##x(struct lisp_sym* sym,             \
                                     struct lisp_hash hash,            \
                                     uint lower, uint upper) {         \
    return hash.x >= sym[lower].hash.x && hash.x <= sym[upper].hash.x; \
  } \
  static inline bool ex_between__##x(struct lisp_sym* sym,             \
                                     struct lisp_hash hash,            \
                                     uint lower, uint upper) {         \
    return hash.x > sym[lower].hash.x && hash.x < sym[upper].hash.x;   \
  } \
  static inline bool repeats__##x(struct lisp_sym* sym) {              \
    return sym->rep & __LISP_HASH_##x;                                 \
  } \
  static inline uint sym_yield__##x(struct lisp_sym* sym) {            \
    return (uint) sym->hash.x;                                         \
  } \
  static inline uint hsh_yield__##x(struct lisp_hash hash) {           \
    return (uint) hash.x;                                              \
  } \
  static struct sort_t sort_##x = {  \
    .in_between  = in_between__##x,  \
    .ex_between  = ex_between__##x,  \
    .repeats     = repeats__##x,     \
    .sym_yield   = sym_yield__##x,   \
    .hsh_yield   = hsh_yield__##x,   \
    .field       = __LISP_HASH_##x,  \
                                     \
    .next        = NULL,             \
  }


////////////////////////////////////////////////////////////////////////////////

struct sort_t;
struct lisp_sym;
struct lisp_hash;

typedef bool (*in_between_t) (struct lisp_sym* sym, struct lisp_hash hash,
                              uint lower, uint upper);
typedef bool (*ex_between_t) (struct lisp_sym* sym, struct lisp_hash hash,
                              uint lower, uint upper);
typedef bool (*repeats_t)    (struct lisp_sym* sym);
typedef uint (*sym_yield_t)  (struct lisp_sym* sym);
typedef uint (*hsh_yield_t)  (struct lisp_hash hash);

enum sort_mask {
  __LISP_HASH_sum  = BIT(0),
  __LISP_HASH_wsum = BIT(1),
  __LISP_HASH_len  = BIT(2),
  __LISP_HASH_rlov = BIT(3),
};

struct sort_t {
  in_between_t in_between;
  ex_between_t ex_between;
  sym_yield_t   sym_yield;
  hsh_yield_t   hsh_yield;
  repeats_t       repeats;
  enum sort_mask    field; // only one of the fields is on per variable

  struct sort_t*     next;
};

////////////////////////////////////////////////////////////////////////////////

struct lisp_hash {
  ushort  len; /** @len:  the length of the symbol               */
  ushort    w; /** @w:    the transient weight                   */
  ushort rlov; /** @rlov: (roll-over) a fail-safe for the hash
                   algorithm; will change if the old sum is even or
                   if the normalized character, or normalized length
                   are even, along with the transient weight     */
  uint    sum; /** @sum:  the numeric sum of the symbol, weighted by
                   prime factor; the hash index is gotten by
                   modulating the field with the `SYMTAB_CELL'
                   macro                                         */
  uint   wsum; /** @wsum: the numeric sum of the symbol, weighted by
                   the transient weight and normalized character */
};

enum lisp_sym_typ {
  __LISP_TYP_NO_TYPE = 0,

  __LISP_CLISP_SYM,
  __LISP_CLISP_FUN,

  __LISP_TYP_HASH,

  __LISP_TYP_SYM,
  __LISP_TYP_SYMP,

  __LISP_TYP_SEXP,
  __LISP_TYP_LEXP,

  __LISP_TYP_FUN,
  __LISP_TYP_FUNP,

  __LISP_TYP_GEN,
};

#  define INFINITY (-1u)

// TODO: we could make this smaller by making `dat' by a union? type for SEXPs
struct lisp_sym {
  struct lisp_hash   hash; /** @hash: the hash of the symbol */

  void*               dat; /** @dat:  data for the symbol    */
  enum lisp_sym_typ   typ; /** @typ:  type of the symbol     */

  uint            size[2]; /** @size: argument range; masked */
  uint            litr[2]; /** @litr: literal range: < size  */
  enum sort_mask      rep; /** @rep:  the repetition mask for `get' functions,
                               one of its fields being set means this hash is
                               the same in that field         */
};

struct lisp_symarr {
  struct lisp_sym sym[SYMTAB_ENTRIES];
  uint           size;
};

struct lisp_sym_cell {
  struct lisp_symarr   entry;
  struct lisp_sym_cell* next;
};

/**
   Used for C LISP definitions
 */
struct clisp_sym {
  const char*     str; /** @str: the C-string representation of the symbol */
  struct lisp_sym sym; /** @sym: the symbol template for the symtab        */
};

////////////////////////////////////////////////////////////////////////////////

struct lisp_hash_ret {
  struct lisp_hash master;
  int               slave;
};

/** UNUSED */
enum lisp_sort_stat {
  __SORT_RETURN = -1,
  __SORT_OK     =  0,
  __SORT_NEXT   =  1,
};

/** UNUSED */
struct lisp_sort_ret {
  uint               master;
  enum lisp_sort_stat slave;
};

////////////////////////////////////////////////////////////////////////////////

struct lisp_hash_ret hash_c(struct lisp_hash hash, char c);
struct lisp_hash_ret hash_str(const char* str);
void                 hash_done(struct lisp_hash* hash);

// TODO: right now, these functions take the `symtab` variable globally
struct lisp_sym* lisp_symtab_set(struct lisp_sym sym);
struct lisp_sym* lisp_symtab_get(struct lisp_hash hash);
int              lisp_symtab_init(void);

#endif
