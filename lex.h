#ifndef LOCK_LEX
#  define LOCK_LEX

#  define LOCK_SYMTAB_INTERNALS

#  include "symtab.h"

#  ifndef IOBLOCK
#    define IOBLOCK (4096)
#  endif

#  define __LISP_WHITESPACE \
       0x00:                \
  case 0x20:                \
  case 0x09:                \
  case 0x0a:                \
  case 0x0b:                \
  case 0x0c:                \
  case 0x0d

#  define __LISP_ALLOWED_IN_NAME(x) \
  (((x) > 0x20) && (x) != 0x7f)

enum lisp_c {
  __LISP_PAREN_OPEN  = '(',
  __LISP_PAREN_CLOSE = ')',
};

enum lisp_lex_ev {
  __LISP_PAREN_OUT      = BIT(0),
  __LISP_SYMBOL_OUT     = BIT(1),

  __LISP_PAREN_IN       = BIT(2),
  __LISP_SYMBOL_IN      = BIT(3),
};

struct lisp_lex_m {
  enum lisp_lex_ev   ev;    /** @ev:    the lex event   */
  uint               paren; /** @paren: the paren level */
  struct lisp_symtab chash; /** @chash: the symbol hash */
};

struct lisp_lex {
  struct lisp_lex_m master;
  int slave;
};

int parse_bytstream(int fd);

#endif

#ifndef LOCK_LEX_FUNC
#  define LOCK_LEX_FUNC

#  define RIGHT (__SEXP_RIGHT_SYM | __SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)
#  define SWAP_RL(x) (((x) & RIGHT) >> 1)

#endif
