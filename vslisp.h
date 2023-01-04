#ifndef LOCK_VSLISP
#  define LOCK_VSLISP

#  ifdef LOCK_POOL_DEF
#    undef LOCK_POOL_DEF
#  endif

#  include "sexp.h"

#  ifndef IOBLOCK
#    define IOBLOCK (4096)
#  endif

#  define __LISP_WHITESPACE \
         0x00: \
    case 0x20: \
    case 0x09: \
    case 0x0a: \
    case 0x0b: \
    case 0x0c: \
    case 0x0d

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

#  define CHILD       (-0)
#  define ROOT        (-1)
#  define RIGHT_CHILD (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)
#  define LEFT_CHILD  (__SEXP_LEFT_SEXP  | __SEXP_LEFT_LEXP)

#endif

#ifndef LOCK_VSLISP_FRONTEND

typedef int (*Frontend)(void);
extern Frontend frontend;

#endif

#ifndef LOCK_VSLISP_FUNC
#  define LOCK_VSLISP_FUNC

#  define RIGHT (__SEXP_RIGHT_SYM | __SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)
#  define SWAP_RL(x) (((x) & RIGHT) >> 1)

#endif
