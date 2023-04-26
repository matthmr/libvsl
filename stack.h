#ifndef LOCK_STACK
#  define LOCK_STACK

#  include "sexp.h" // also includes `symtab.h'
#  include "lisp.h"

#  define DISJOINT(x) (int) ((x)[0] != (x)[1])
#  define LAZY(x)     (int) ((x).lazy == 0)
#  define LISP_FUN(x) ((lisp_fun) (x))

enum lisp_stack_ev {
  __STACK_POP      = BIT(0),

  __STACK_PUSH_VAR = BIT(1),
  __STACK_PUSH_FUN = BIT(2),

  __STACK_QUOT     = BIT(3),
};

#  define STACK_QUOT(x)       ((x) & __STACK_QUOT)
#  define STACK_POPPED(x)     ((x) & __STACK_POP)
#  define STACK_PUSHED_FUN(x) ((x) & __STACK_PUSH_FUN)
#  define STACK_PUSHED_VAR(x) ((x) & __STACK_PUSH_VAR)
#  define STACK_PUSHED(x)     (STACK_PUSHED_VAR(x) | (STACK_PUSHED_FUNC(x)))

////////////////////////////////////////////////////////////////////////////////

struct lisp_stack {
  struct lisp_sexp* lazy; /** @lazy:      the SEXP pointer for arguments
                              over the lower limit; lazily evaluated     */
  struct lisp_sexp* expr; /** @expr:      the SEXP pointer for general
                              purpose SEXPs; disjoint from @lazy         */
  struct lisp_hash  hash; /** @hash:      the lexer hash                 */
  uint             paren; /** @paren:     the paren level                */
  uint         lit_paren; /** @lit_paren: the parent paren level of a symbolic
                              quote                                      */
  enum lisp_stack_ev  ev; /** @ev:        the stack event                */
};

struct lisp_frame {
  struct lisp_stack stack; /** @stack: the current stack state       */
  struct lisp_sym     sym; /** @sym:   the current function          */
  struct lisp_ret     pop; /** @pop:   the value from a function pop */
  struct lisp_arg*   argp; /** @argp:  the argument register         */
  uint               argv; /** @argv:  the current argument amount; the
                               *absolute* amount is stored in the frame
                               function' stack instead               */
};

enum lisp_stack_stat {
  __STACK_NEW  = -2, // as in: push a new frame
  __STACK_DONE = -1, // as in: popped to the frame
  __STACK_OK   = 0,
};

////////////////////////////////////////////////////////////////////////////////

/**
  NOTE
  ----

  the functions below have two variants:

  1. a SEXP one
  2. a LEX one

  as for (1.), it acts on functions: it reads the expressions already LEX'd by
  the VSL lexer and parsed into the AST and structures the memory accordingly

  as for (2.), it acts on LEX callbacks (i.e. when the VSL lexer encounters `('
  or `)'): it immediately executes the expression as soon as the exit token is
  issued by the lexer
 */
struct lisp_ret lisp_stack_sexp_frame(struct lisp_stack* stack);
struct lisp_ret lisp_eval_fun(struct lisp_arg* argp, uint argv);

////////////////////////////////////////////////////////////////////////////////

struct lisp_ret lisp_stack_lex_frame(struct lisp_stack* stack);

#endif
