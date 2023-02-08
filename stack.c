#include "stack.h"
#include "debug.h"
#include "err.h"

// TODO: some functions may change variables through a pointer

////////////////////////////////////////////////////////////

// TODO: stub
/** SEXP stack: BEGIN */

void lisp_stack_sexp_push(struct lisp_stack* stack, POOL_T* mpp,
                          struct lisp_sexp* head) {
  stack->ev  |= __STACK_PUSH_FUNC;
  // stack->fun  = (lisp_fun) head->left.sym.dat;

  stack->typ.sexp.head = head;
  stack->typ.sexp.mpp  = mpp;
}

void lisp_stack_sexp_push_var(struct lisp_stack* stack, POOL_T* mpp,
                              struct lisp_sexp* head, enum lisp_stack_ev ev) {
  stack->ev  |= ev;
  // stack->fun  = (lisp_fun) ((ev & __STACK_PUSH_LEFT)?
  //                            head->left.sym: head->right.sym).dat;

  stack->typ.sexp.head = head;
  stack->typ.sexp.mpp  = mpp;
}

void lisp_stack_sexp_pop(struct lisp_stack* stack, POOL_T* mpp,
                         struct lisp_sexp* head) {
  stack->ev |= __STACK_POP;
  stack->typ.sexp.head = head;
  stack->typ.sexp.mpp  = mpp;
}

static int lisp_stack_sexp_frame_var(struct lisp_frame* frame) {
  int ret = 0;
  done_for(ret);
}

static int lisp_stack_sexp_frame(struct lisp_stack* stack) {
  int ret  = 0;
  done_for(ret);
}
/** SEXP stack: END */

////////////////////////////////////////////////////////////

/** LEX stack: BEGIN */
static int lisp_stack_lex_frame_var(struct lisp_frame* frame,
                                    struct lisp_sym* sym) {
  int ret = 0;

  DB_MSG("[ == ] stack: lex_frame_var()");

  uint* size = sym->size;

  if (size[1]) {
    assert((frame->tab.i < size[1]), err(EARGTOOBIG));
  }
  else if (size[0] == 0) {
    defer_as(err(EARGTOOBIG));
  }

  struct lisp_sym_ret stret = lisp_symtab_get(frame->stack.typ.lex.hash);

  assert(stret.slave == 0, OR_ERR());

  // TODO: this is probably wrong
  frame->tab.reg[frame->tab.i] = *stret.master;

  done_for(ret);
}

// TODO: make the frame function check the grammar
int lisp_stack_lex_frame(struct lisp_stack* stack) {
  int ret = 0;

  struct lisp_frame frame;

  DB_MSG("[ == ] stack: lex_frame()");

  frame.stack  = *stack;
  frame.tab.i  = 1;

  // it's guaranteed that if we're calling this function,
  // the first argument is expected to be a function
  struct lisp_sym_ret stret = lisp_symtab_get(stack->typ.lex.hash);
  assert(stret.slave == 0, OR_ERR());

  struct lisp_sym* sym      = stret.master;

  {
    struct lisp_sym reg[sym->size[0]];
    frame.tab.reg = reg;
  }

  // appease the compiler by letting the memory be allocated beforehand,
  // even though we know there's nothing here
  assert(stret.slave == 0, OR_ERR());
  assert(sym->typ == __LISP_FUN && sym->dat != NULL,
         err(EISNOTFUNC));

yield:
  /** see if the root expr asked for literals. if so,
      then ask for the lexer to send its tokens to
      the SEXP tree
  */
  if (sym->litr[0] &&
      (frame.tab.i >= sym->litr[0] &&
       (sym->litr[1] == -1 || frame.tab.i <= sym->litr[1]))) {
    frame.stack.ev |= __STACK_LIT;
    assert(FRAME_LEXER(frame) (&frame.stack) == 0, OR_ERR());
    assert(lisp_stack_lex_frame_var(&frame, sym) == 0, OR_ERR());
    ++frame.tab.i;
    goto yield;
  }
  else {
    assert(FRAME_LEXER(frame) (&frame.stack) == 0, OR_ERR());
    ++frame.tab.i;
  }

  enum lisp_stack_ev ev = frame.stack.ev;

  if (STACK_PUSHED_VAR(ev)) {
    frame.stack.ev &= ~__STACK_PUSHED_VAR;
    // assert(lisp_stack_lex_frame_var(&frame, sym) == 0, 1);

    if (STACK_POPPED(ev)) {
      goto close_pop;
    }
    else {
      goto yield;
    }
  }

  else if (STACK_PUSHED_FUNC(ev)) {
    assert(lisp_stack_lex_frame(&frame.stack) == 0, OR_ERR());
    frame.stack.ev &= ~__STACK_PUSHED_FUNC;

    // TODO: i think this is wrong. the `frame_var' function
    // should be dealing with objects in the symbol table already
    assert(lisp_stack_lex_frame_var(&frame, sym) == 0, OR_ERR());
    goto yield;
  }

  else if (STACK_POPPED(ev)) {
close_pop:
    frame.stack.ev &= ~__STACK_POPPED;
    assert(
      ((lisp_fun) sym->dat) (&frame) == 0, OR_ERR());
  }

  done_for(ret);
}
/** LEX stack: END */

////////////////////////////////////////////////////////////
