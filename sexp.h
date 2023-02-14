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

  __SEXP_SELF_EMPTY  = BIT(9),
  __SEXP_LEFT_EMPTY  = BIT(10),
  __SEXP_RIGHT_EMPTY = BIT(11),
};

#  define CHILD       (-0)
#  define ROOT        (-1)

#  define RIGHT_CHILD (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)
#  define LEFT_CHILD  (__SEXP_LEFT_SEXP  | __SEXP_LEFT_LEXP)

struct pos_t {
  uint am;
  int  pidx; /** index of the pool section;
                 mirroed by `pool.h:MEMPOOL::idx' */
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

int sexp_init(void);

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
