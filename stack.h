/** Functional stack interface */

#ifndef LOCK_STACK
#  define LOCK_STACK

#  include "symtab.h"
#  include "sexp.h"

/** Main stack interface */
struct lisp_stack {
  /* main argument register, represented as the SEXP tree that would've been
     formed had this function been made through the lexer, with the symbols and
     sub-expressions resoluted down to their evaluations */
  struct lisp_sexp* argp;

  /* argument amount, counting the function name */
  uint argv;

  /* current lexical scope */
  struct lisp_symtab* envp;

  /* did we just inherited the parent? */
  bool inherit;

  /* is this function literal? i.e. does it defer expression resolution? */
  bool lit;
};

#if 0
/** Base stack stat */
enum lisp_stack_stat {
  __STACK_ERR  = -1,

  __STACK_OK   =  0,
  __STACK_ELEM, // as in: pushed an element
  __STACK_NEW,  // as in: push a new frame
  __STACK_DONE, // as in: popped the current frame
};
#endif

////////////////////////////////////////////////////////////////////////////////

/** Create a new stack frame. Will yield from the SEXP @exp */
struct lisp_ret
lisp_stack_frame_sexp(struct lisp_sexp* exp, struct lisp_symtab* envp);

/** Create a new stack frame. Will yield from the lexer until a `pop' event is
    sent */
struct lisp_ret lisp_stack_frame_lex(struct lisp_symtab* envp);

#endif
