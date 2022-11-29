#ifndef LOCK_VSLISP
#  define LOCK_VSLISP

#  ifndef false
#    define false 0x0
#  endif

#  ifndef true
#    define true  0x1
#  endif

#  ifndef neither
#    define neither 0x2
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
  uint  __i;
  uint  len;
  ulong hash;
};

enum sexp_t {
  __SEXP_EMPTY = 0,
  __SEXP_SEXP,
  __SEXP_LEXP,
  __SEXP_SYM,
  __SEXP_ROOT,
};

struct off_t {
  uint am;
  enum sexp_t t; /**
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
  int pi; /**
               >0: look to `pi' pools ahead
               <0: look to `pi' pools below
               =0: look in the same pool
             */
};

struct node_t {
  struct lisp_hash sym;
  struct off_t     off;
};

struct lisp_sexp {
  struct node_t root;
  struct node_t left;
  struct node_t right;
};

#endif
