#ifndef LOCK_VSLISP
#  define LOCK_VSLISP

#  ifndef false
#    define false 0x0
#  endif

#  ifndef true
#    define true  0x1
#  endif

#  define IOBLOCK (4096)
#  define SEXPPOOL (4)   // DEBUG

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
  (((x) > 0x20) || (x) != 0x7f)

typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char bool;

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

struct lisp_hash_body {
  ulong hash;
  char  cmask[sizeof(ulong)];
};

struct lisp_hash {
  uint len;
  struct lisp_hash_body body;
};

enum sexp_t {
  __SEXP_CHILD       = -2,
  __SEXP_ROOT        = -1,

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

#  define RIGHT_CHILD_T (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)
#  define LEFT_CHILD_T  (__SEXP_LEFT_SEXP  | __SEXP_LEFT_LEXP)

struct pos_t {
  uint am;
  int  pidx; /** index of the pool section;
                 mirroed by `pool.h:MEMPOOL::idx' */
};

union node_t {
  struct lisp_hash sym;
  struct pos_t     pos;
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
                                     a   =
                                        / \
                                       b   c
                   */
};

#endif
