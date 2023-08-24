#ifndef LOCK_STACK
#  define LOCK_STACK

#  include "lisp.h"

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
  struct lisp_sexp* expr; /** @expr:      the SEXP pointer for general
                              purpose SEXPs; disjoint from @lazy */
  struct lisp_hash  hash; /** @hash:      the lexer hash         */
  uint             paren; /** @paren:     the paren level        */
  uint         lit_paren; /** @lit_paren: the parent paren level of a symbolic
                              quote                              */
  enum lisp_stack_ev  ev; /** @ev:        the stack event        */
};

// TODO: do we need `::pop' to be part of this struct?
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
  __STACK_ELEM = -3, // as in: pushed an element
  __STACK_NEW  = -2, // as in: push a new frame
  __STACK_DONE = -1, // as in: popped to the frame
  __STACK_OK   =  0,
};

////////////////////////////////////////////////////////////////////////////////

struct lisp_ret lisp_stack_sexp_frame(struct lisp_stack* stack);
struct lisp_ret lisp_eval_fun(struct lisp_arg* argp, uint argv);

//// SEXP

struct lisp_ret lisp_stack_lex_frame(struct lisp_stack* stack);

#endif
