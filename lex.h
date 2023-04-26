#ifndef LOCK_LEX
#  define LOCK_LEX

#  include "symtab.h" // also includes `utils.h'
#  include "stack.h"  // also includes `sexp.h'

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
};

#  define __LEX_TANGLE (__LISP_EV_PAREN_IN | __LISP_EV_PAREN_OUT)
#  define LEX_TANGLE(ev)     ((ev) & __LEX_TANGLE)
#  define LEX_PAREN_IN(ev)   ((ev) & __LISP_EV_PAREN_IN)
#  define LEX_PAREN_OUT(ev)  ((ev) & __LISP_EV_PAREN_OUT)
#  define LEX_SYMBOL_OUT(ev) ((ev) & __LISP_EV_SYMBOL_OUT)
#  define LEX_SYMBOL_IN(ev)  ((ev) & __LISP_EV_SYMBOL_IN)

enum lisp_lex_stat {
  __LEX_NO_INPUT = -1,
  __LEX_OK       =  0,
};

struct lisp_lex {
  enum lisp_lex_ev   ev; /** @ev:     the lex event                    */
  uint            paren; /** @paren:  the paren level                  */
  uint           cb_idx; /** @cb_idx: the bytstream character index    */
  uint             size; /** @size:   the bytstream (significant) size */
  struct lisp_hash hash; /** @chash:  the symbol hash                  */
};

/** UNUSED */
struct lisp_lex_ret {
  struct lisp_lex   master;
  enum lisp_lex_stat slave;
};

////////////////////////////////////////////////////////////////////////////////

int lisp_parser(int fd);
int lisp_lex_yield(struct lisp_stack* stack);

#endif
