#ifndef LOCK_STACK
#  define LOCK_STACK

// we only want the *definition* of POOL_T from `sexp.h', not the implementation
#  define LOCK_POOL_DEF
#  define LOCK_POOL_THREAD

#  include "sexp.h" // also includes `symtab.h' (lock internals)
#  include "pool.h" // also includes `err.h', `utils.h', `stdlib.h'
#  include "prim.h" // also includes `symtab.h'

struct lisp_stack;
struct lisp_frame;

enum lisp_stack_ev {
  __STACK_POP        = BIT(0), /** for both */

  __STACK_PUSH_LEFT  = BIT(1), /** for SEXP */
  __STACK_PUSH_RIGHT = BIT(2), /** for SEXP */
  __STACK_PUSH_VAR   = BIT(3), /** for both */
  __STACK_PUSH_FUNC  = BIT(4), /** for both */

  __STACK_LIT        = BIT(5), /** for both */
  __STACK_SYM        = BIT(6), /** for LEX */
};

#  define __STACK_QUOT (__STACK_LIT)
#  define STACK_QUOT(x) \
  ((x) & __STACK_QUOT)

#  define __STACK_POPPED (__STACK_POP)
#  define STACK_POPPED(x) \
  ((x) & __STACK_POPPED)

#  define __STACK_PUSHED_FUNC (__STACK_PUSH_FUNC)
#  define STACK_PUSHED_FUNC(x) \
  ((x) & __STACK_PUSHED_FUNC)

#  define __STACK_PUSHED_VAR (__STACK_PUSH_VAR | __STACK_PUSH_LEFT | __STACK_PUSH_RIGHT)
#  define STACK_PUSHED_VAR(x) \
  ((x) & __STACK_PUSHED_VAR)

#  define __STACK_PUSHED (__STACK_PUSHED_VAR | __STACK_PUSHED_FUNC)
#  define STACK_PUSHED(x) \
  (STACK_PUSHED_VAR(x) | (STACK_PUSHED_FUNC(x)))

////////////////////////////////////////////////////////////////////////////////

struct lisp_lex_stack {
  struct lisp_hash hash;  /** @hash:  the current hash      */
  uint             paren; /** @paren: the paren level       */
};

struct lisp_sexp_stack {
  struct lisp_sexp* head; /** @head: the current sexp head   */
  POOL_T*           mpp;  /** @mpp:  the current pool thread */
};

union lisp_stack_typ {
  struct lisp_sexp_stack sexp; /** @sexp: SEXP stack; used for functions  */
  struct lisp_lex_stack  lex;  /** @lex:  LEX stack; used for source code */
};

struct lisp_stack {
  union lisp_stack_typ typ; /** @typ:  the stack type (LEX|SEXP) */
  enum lisp_stack_ev   ev;  /** @ev:   the stack event           */
};

struct lisp_frame_reg {
  struct lisp_fun_arg* _; /** @_: the argument register     */
  uint i;                 /** @i: the current element index */
};

struct lisp_frame {
  struct lisp_stack     stack; /** @stack: the current stack state       */
  struct lisp_symc_ret  sym;   /** @sym:   the current function          */
  struct lisp_frame_reg reg;   /** @reg:   the argument register         */
  struct lisp_fun_ret   pop;   /** @pop:   the value from a function pop */
};

////////////////////////////////////////////////////////////////////////////////

/**
  NOTE
  ----

  the functions below have two variants:

  1. a SEXP one
  2. a LEX one

  as for 1., it acts on functions: it reads the expressions
  already LEX'd by the VSL lexer and parsed into the AST and
  structures the memory accordingly. its callbacks are:

                       lisp_sexp_trans

  as for 2., it acts on LEX callbacks (i.e. when the VSL lexer
  encounters `(' or `)'): it immediately executes the expression
  as soon as the exit token is issued by the lexer. its callbacks
  are:

                      lisp_lex_bytstream
                     parse_bytstream_base
 */
void lisp_stack_sexp_push(struct lisp_stack* stack,
                          POOL_T* mpp, struct lisp_sexp* head);

void lisp_stack_sexp_push_var(struct lisp_stack* stack, POOL_T* mpp,
                              struct lisp_sexp* head, enum lisp_stack_ev ev);

void lisp_stack_sexp_pop(struct lisp_stack* stack,
                         POOL_T* mpp, struct lisp_sexp* head);

////////////////////////////////////////////////////////////////////////////////

struct lisp_fun_ret lisp_stack_lex_frame(struct lisp_stack* stack);

#endif
