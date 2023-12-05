/** LISP lexer */

#ifndef LOCK_LEX
#  define LOCK_LEX

#  include "symtab.h"

#  ifndef IOBLOCK
#    define IOBLOCK 4096
#  endif

#  ifndef IOFD
#    define IOFD 0 // STDIN
#  endif

#  define __LEX_C_WHITESPACE \
       0x00: \
  case 0x20: \
  case 0x09: \
  case 0x0a: \
  case 0x0b: \
  case 0x0c: \
  case 0x0d

#  define __LISP_ALLOWED_IN_NAME(x) \
  (((x) > 0x20) && (x) != 0x7f)

/** Base lexer triggers */
enum lisp_lex_c {
  __LEX_C_PAREN_OPEN  = '(',
  __LEX_C_PAREN_CLOSE = ')',
};

/** Base lexer events */
enum lisp_lex_ev {
  __LEX_EV_PAREN_OUT  = BIT(0),
  __LEX_EV_SYMBOL_OUT = BIT(1),

  __LEX_EV_PAREN_IN   = BIT(2),
  __LEX_EV_SYMBOL_IN  = BIT(3),
};

#  define __LEX_TANGLE (__LEX_EV_PAREN_IN | __LEX_EV_PAREN_OUT)
#  define LEX_TANGLE(ev)     ((ev) & __LEX_TANGLE)
#  define LEX_PAREN_IN(ev)   ((ev) & __LEX_EV_PAREN_IN)
#  define LEX_PAREN_OUT(ev)  ((ev) & __LEX_EV_PAREN_OUT)
#  define LEX_SYMBOL_OUT(ev) ((ev) & __LEX_EV_SYMBOL_OUT)
#  define LEX_SYMBOL_IN(ev)  ((ev) & __LEX_EV_SYMBOL_IN)

/** Base lexer stream stat */
enum lisp_lex_stat {
  __LEX_NO_INPUT = -1,
  __LEX_OK       =  0,
};

/** Main lexer interface. NOTE: this is a *very simple* lexer, it doesn't use a
    context-free grammar, nor does it use regular expressions. This is just for
    this LISP */
struct lisp_lex {
  /* yielded events */
  enum lisp_lex_ev ev;

  /* symbol buffer. The `idx' field doubles as the size */
  string_ip symbuf;

  /* I/O buffer. The `idx' field is the 'callback index' */
  string_ip iobuf;

  /* current paren level*/
  uint paren;

  /* significant size of iobuf */
  uint iobuf_size;
};

////////////////////////////////////////////////////////////////////////////////

// the main lexer. use this variable instead of passing it as an argument
extern struct lisp_lex lex;

/** Feed the lexer with @IOBLOCK bytes, from fd @IOFD to some buffer */
int lisp_lex_feed(void);

/** Yield expressions from the lexer */
int lisp_lex_yield(void);

#endif
