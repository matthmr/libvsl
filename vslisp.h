#ifndef LOCK_VSLISP
#  define LOCK_VSLISP

#  ifndef false
#    define false 0x0
#  endif

#  ifndef true
#    define true  0x1
#  endif

#  define IOBLOCK (4096)
#  define SEXPPOOL (128)

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

#  define MEMPOOL_TMPL(t) __mempool_##t

#  define MEMPOOL(t,am)            \
  MEMPOOL_TMPL(t) {                \
    struct t mem[am];              \
    struct MEMPOOL_TMPL(t) * next; \
    struct MEMPOOL_TMPL(t) * prev; \
    uint used;                     \
    uint total;                    \
  }

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

struct lisp_hash {
  uint  len;
  ulong hash;
};

enum sexp_t {
  __SEXP_EMPTY      = 0,
  __SEXP_SELF_ROOT  = BIT(0),
  __SEXP_SELF_SEXP  = BIT(1),
  __SEXP_LEFT_SEXP  = BIT(2),
  __SEXP_RIGHT_SEXP = BIT(3),
  __SEXP_SELF_LEXP  = BIT(4),
  __SEXP_LEFT_LEXP  = BIT(5),
  __SEXP_RIGHT_LEXP = BIT(6),
  __SEXP_SELF_SYM   = BIT(7),
  __SEXP_LEFT_SYM   = BIT(8),
  __SEXP_RIGHT_SYM  = BIT(9),
};

struct off_t {
  int am;
  int pi; /**
               >0: look to `pi' pools ahead
               <0: look to `pi' pools below
               =0: look in the same pool
             */
};

union node_t {
  struct lisp_hash sym;
  struct off_t     off;
};

struct lisp_sexp {
  struct off_t root;
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
