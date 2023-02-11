#ifndef LOCK_STACK
#  define LOCK_STACK

// we only want the *definition* of POOL_T from `sexp.h', not the implementation
#  define LOCK_POOL_DEF
#  define LOCK_POOL_THREAD

#  include "sexp.h"
#  include "pool.h"

struct lisp_stack;
struct lisp_frame;

/** for LEX */
typedef int  (*lexer) (struct lisp_stack* stack);

/** for SEXP */
typedef void (*sexp_cb) (struct lisp_stack* stack);

/** for both */
// typedef int  (*lisp_fun) (struct lisp_frame* frame);

enum lisp_stack_ev {
  __STACK_POP        = BIT(0), /** for both */

  __STACK_PUSH_LEFT  = BIT(1), /** for SEXP */
  __STACK_PUSH_RIGHT = BIT(2), /** for SEXP */
  __STACK_PUSH_VAR   = BIT(3), /** for both */
  __STACK_PUSH_FUNC  = BIT(4), /** for both */

  __STACK_LIT        = BIT(5), /** for both */
  __STACK_SYM        = BIT(6), /** for LEX */
  __STACK_EMPTY      = BIT(7), /** for LEX  */
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
  lexer            cb;    /** @cb:    the callback function */
};

struct lisp_sexp_stack {
  struct lisp_sexp* head; /** @head: the current sexp head   */
  POOL_T*           mpp;  /** @mpp:  the current pool thread */
  sexp_cb           cb;   /** @cb:   the callback function   */
};

union lisp_stack_typ {
  struct lisp_sexp_stack sexp; /** @sexp: SEXP stack; used for functions  */
  struct lisp_lex_stack  lex;  /** @lex:  LEX stack; used for source code */
};

struct lisp_stack {
  union lisp_stack_typ typ; /** @typ:  the stack type (LEX|SEXP) */
  enum lisp_stack_ev   ev;  /** @ev:   the stack event           */
};

enum lisp_frame_reg_t {
  __FRAME_VAR_GEN = 0,
  __FRAME_VAR_SYMP,
  __FRAME_VAR_SYM,
  __FRAME_VAR_HASH,
  __FRAME_VAR_SEXP,
};

union lisp_frame_reg_m {
  struct lisp_sym*  sym;  /** @sym:  a symbol pointer; for (ref) and alike */
  struct lisp_hash  hash; /** @hash: a hash; for most set/get quotes       */
  struct lisp_sexp* sexp; /** @sexp: a sexp; for most general quotes       */
  void*             gen;  /** @gen:  generic memory; casted by the caller  */
};

struct lisp_frame_reg_dat {
  enum lisp_frame_reg_t  typ; /** @typ: the type of the memory */
  union lisp_frame_reg_m mem; /** @mem: the memory             */
};

struct lisp_frame_reg {
  struct lisp_frame_reg_dat* dat; /** @dat:  the argument value register */
  uint size;                      /** @size: the minimum functional size */
  uint i;                         /** @i:    the current element index   */
};

struct lisp_frame {
  struct lisp_stack     stack;
  struct lisp_frame_reg reg;
};

#  define LEXER(frame) \
  frame.stack.typ.lex.cb

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
void
lisp_stack_sexp_push(struct lisp_stack* stack,
                     POOL_T* mpp, struct lisp_sexp* head);

void
lisp_stack_sexp_push_var(struct lisp_stack* stack, POOL_T* mpp,
                         struct lisp_sexp* head, enum lisp_stack_ev ev);

void
lisp_stack_sexp_pop(struct lisp_stack* stack,
                    POOL_T* mpp, struct lisp_sexp* head);

////////////////////////////////////////////////////////////////////////////////

int
lisp_stack_lex_frame(struct lisp_stack* stack);

#endif
