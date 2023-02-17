#ifndef LOCK_SEXP
#  define LOCK_SEXP

#  ifndef SEXPPOOL
#    define SEXPPOOL (32)
#  endif

#  define LOCK_SYMTAB_INTERNALS

#  include "symtab.h"

enum sexp_t {
  __SEXP_SELF_ROOT   = BIT(0),

  __SEXP_SELF_SEXP   = BIT(1),
  __SEXP_LEFT_SEXP   = BIT(2),
  __SEXP_RIGHT_SEXP  = BIT(3),

  __SEXP_SELF_LEXP   = BIT(4),
  __SEXP_LEFT_LEXP   = BIT(5),
  __SEXP_RIGHT_LEXP  = BIT(6),

  __SEXP_LEFT_SYM    = BIT(7),
  __SEXP_RIGHT_SYM   = BIT(8),
};

// used for `get_pos'
#  define __RIGHT_CHILD \
  (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)

#  define __LEFT_CHILD  \
  (__SEXP_LEFT_SEXP  | __SEXP_LEFT_LEXP)

#  define __ROOT        \
  (__SEXP_SELF_ROOT)

#  define __CHILD \
  (__RIGHT_CHILD | __LEFT_CHILD)

#  define RIGHT_CHILD(x) ((x) & (__SEXP_RIGHT_SYM | __RIGHT_CHILD))
#  define LEFT_CHILD(x)  ((x) & (__SEXP_LEFT_SYM  | __LEFT_CHILD))
#  define IS_ROOT(x)     ((x) & __ROOT)
#  define IS_LEXP(x)     ((x) & __SEXP_SELF_LEXP)

#  define RIGHT_CHILD_EXPR(x) \
  ((x) & (__RIGHT_CHILD))

#  define LEFT_CHILD_EXPR(x) \
  ((x) & (__LEFT_CHILD))

struct pos_t {
  uint pidx; /** @pidx: offset for the appropriate type; relative to the base
                        of a pool section                   */
  int  cidx; /** @cidx: index of the pool section;
                        mirrored by `pool.h:MEMPOOL::c_idx' */
};

// TODO: add `struct lisp_sym` as a type
union node_t {
  struct lisp_hash sym;
  struct pos_t     pos;
};

struct lisp_sexp {
  struct pos_t root;
  union node_t left;
  union node_t right;

  /**
        SEXP
        ----
        (a (b c)) ->     .
                        / \
                       a   .
                          / \
                         b   c
        LEXP
        ----
        (a b c) ->       .
                        / \
                       a   ,
                          / \
                         b   c
  */
  enum sexp_t  t;
};

void sexp_init(void);

#endif

#ifndef LOCK_SEXP_FUNC
#  define LOCK_SEXP_FUNC

#  define RIGHT (__SEXP_RIGHT_SYM | __SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)
#  define SWAP_RL(x) (((x) & RIGHT) >> 1)

#endif

#ifndef LOCK_SEXP_INTERNALS
#  define LOCK_SEXP_INTERNALS

#  define POOL_ENTRY_T struct lisp_sexp
#  define POOL_AM      SEXPPOOL

#  ifdef LOCK_POOL
#    undef LOCK_POOL
#  endif

#  include "pool.h"

void lisp_sexp_sym(POOL_T** mpp, struct lisp_hash hash);
void lisp_sexp_end(POOL_T* mpp);
void lisp_sexp_node_add(POOL_T** mpp);
int  lisp_sexp_eval(POOL_T* mpp);

#endif
