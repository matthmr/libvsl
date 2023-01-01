#ifndef LOCK_VSLISP
#  define LOCK_VSLISP

// don't use `symtab.h's definitions for their pool
#  define LOCK_POOL
#  define LOCK_SYMTAB_INTERNALS

#  include "symtab.h"

#  ifndef IOBLOCK
#    define IOBLOCK (4096)
#  endif

#  ifndef SEXPPOOL
#    define SEXPPOOL (64)
#  endif

#  define __LISP_WHITESPACE \
         0x00: \
    case 0x20: \
    case 0x09: \
    case 0x0a: \
    case 0x0b: \
    case 0x0c: \
    case 0x0d

#  define BIT(x) (1 << (x))
#  define MSG(x) \
  x, ((sizeof(x)/sizeof(char)) - 1)

#  define __LISP_ALLOWED_IN_NAME(x) \
  (((x) > 0x20) && (x) != 0x7f)

enum lisp_c {
  __LISP_PAREN_OPEN = '(',
  __LISP_PAREN_CLOSE = ')',
};

enum lisp_pev {
  __LISP_PAREN_OUT  = BIT(0),
  __LISP_SYMBOL_OUT = BIT(1),
};

enum lisp_pstat {
  __LISP_PAREN_IN  = BIT(0),
  __LISP_SYMBOL_IN = BIT(1),
};

struct lisp_cps {
  enum lisp_pstat master;
  int             slave;
};

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

  __SEXP_LEFT_EMPTY  = BIT(9),
  __SEXP_RIGHT_EMPTY = BIT(10),
  __SEXP_SELF_EMPTY  = BIT(11),
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

union node_t {
  ulong        sym;
  struct pos_t pos;
};

struct lisp_sexp {
  struct pos_t root;
  union node_t left;
  union node_t right;
  enum sexp_t  t; /**
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
};

#  define POOL_ENTRY_T struct lisp_sexp
#  define POOL_AM      SEXPPOOL

#  ifdef LOCK_POOL
#    undef LOCK_POOL
#  endif

#  include "pool.h"

void (*frontend)(void);

#endif

#ifndef LOCK_VSLISP_FUNC
#  define LOCK_VSLISP_FUNC

#  define RIGHT (__SEXP_RIGHT_SYM | __SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)
#  define SWAP_RL(x) (((x) & RIGHT) >> 1)

#endif
