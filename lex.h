#ifndef LOCK_LEX
#  define LOCK_LEX

#  define LOCK_SYMTAB_INTERNALS
#  include "symtab.h"

#  ifndef IOBLOCK
#    define IOBLOCK (4096)
#  endif

#  define __LISP_C_WHITESPACE \
       0x00:                  \
  case 0x20:                  \
  case 0x09:                  \
  case 0x0a:                  \
  case 0x0b:                  \
  case 0x0c:                  \
  case 0x0d

#  define __LISP_ALLOWED_IN_NAME(x) \
  (((x) > 0x20) && (x) != 0x7f)

enum lisp_c {
  __LISP_C_PAREN_OPEN  = '(',
  __LISP_C_PAREN_CLOSE = ')',
};

enum lisp_lex_ev {
  __LISP_EV_PAREN_OUT  = BIT(0),
  __LISP_EV_SYMBOL_OUT = BIT(1),

  __LISP_EV_PAREN_IN   = BIT(2),
  __LISP_EV_SYMBOL_IN  = BIT(3),

  __LISP_EV_FEED       = BIT(4),
};

struct lisp_lex_m {
  enum lisp_lex_ev ev;     /** @ev:     the lex event                    */
  uint             paren;  /** @paren:  the paren level                  */
  uint             cb_idx; /** @cb_idx: the bytstream character index    */
  uint             size;   /** @size:   the bytstream (significant) size */
  struct lisp_hash hash;   /** @chash:  the symbol hash                  */
};

struct lisp_lex {
  struct lisp_lex_m master;
  int slave;
};

int parse_bytstream(int fd);

#endif
