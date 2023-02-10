#include "stack.h"
#include "debug.h"
#include "prim.h"
#include "err.h"

// TODO: some functions may change variables through a pointer
// TODO: stub on the SEXP stack

static int lisp_stack_sexp_frame_var(struct lisp_frame* frame) {
  int ret = 0;
  done_for(ret);
}

static int lisp_stack_sexp_frame(struct lisp_stack* stack) {
  int ret  = 0;
  done_for(ret);
}


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

////////////////////////////////////////////////////////////

static int lisp_stack_lex_frame_var(struct lisp_frame* frame, uint* size) {
  int ret = 0;

  DB_MSG("[ == ] stack(lex): frame_var()");

  // TODO: the lisp primitive functions have a iterative callback that can allow
  // the existence of of bound(less) range of variables; that's not implemented

  // this function should not have been called
  if (size[0] == 0) {
    defer_as(err(EARGTOOBIG));
  }
  else if (size[1] != INFINITY) {
    assert(frame->reg.i <= size[1], err(EARGTOOBIG));
  }

  struct lisp_sym_ret stret = lisp_symtab_get(frame->stack.typ.lex.hash);

  assert(stret.slave == 0, OR_ERR());

  // TODO: this is probably wrong; we *can* take things as pointers with
  //       (ref x y)
  frame->reg.dat[IDX_HM(frame->reg.i)] = *stret.master;

  done_for(ret);
}


// TODO: the LISP functions' structure are out-of-date
int lisp_stack_lex_frame(struct lisp_stack* stack) {
  int ret = 0;

  struct lisp_frame frame  = {0};
  struct lisp_fun_ret fret = {0};

  DB_MSG("[ == ] stack(lex): frame()");

  frame.stack = *stack;
  frame.reg.i = 0;

  // it's guaranteed that if we're calling this function,
  // the first argument is expected to be a function
  struct lisp_sym_ret stret = lisp_symtab_get(stack->typ.lex.hash);
  assert(stret.slave == 0, OR_ERR());

  struct lisp_sym sym = *stret.master;
  assert(sym.typ == __LISP_FUN && sym.dat != NULL,
         err(EISNOTFUNC));

  // allocate the memory; needs to be scoped because this branch does not always
  // run in this function
  {
    // FIXME: this could be heap-allocated
    struct lisp_sym reg[sym.size[0]];
    frame.reg.dat = reg;
  }

yield:
  /** see if the root expr asked for literals. if so,
      then ask for the lexer to send its tokens to
      the SEXP tree
  */
  if ((sym.litr[0] != 0 && frame.reg.i >= sym.litr[0]) &&
      (sym.litr[1] == INFINITY || frame.reg.i <= sym.litr[1])) {
    DB_FMT("[ == ] stack(lex): index %d is literal", frame.reg.i);
    frame.stack.ev |= __STACK_LIT;
    assert(FRAME_LEXER(frame) (&frame.stack) == 0, OR_ERR());
    assert(lisp_stack_lex_frame_var(&frame, sym.size) == 0, OR_ERR());
    ++frame.reg.i;
    goto yield;
  }

  assert(FRAME_LEXER(frame) (&frame.stack) == 0, OR_ERR());

  enum lisp_stack_ev ev = frame.stack.ev;

  if (STACK_PUSHED_VAR(ev)) {
    if (++frame.reg.i,
        (sym.size[0] == 0 || frame.reg.i > sym.size[0])) {
      defer_as(err(EARGTOOBIG));
    }

    frame.stack.ev &= ~__STACK_PUSHED_VAR;
    assert(lisp_stack_lex_frame_var(&frame, sym.size) == 0, OR_ERR());
    goto yield;
  }

  else if (STACK_PUSHED_FUNC(ev)) {
    if (++frame.reg.i,
        (sym.size[0] == 0 || frame.reg.i > sym.size[0])) {
      defer_as(err(EARGTOOBIG));
    }

    assert(lisp_stack_lex_frame(&frame.stack) == 0, OR_ERR());
    frame.stack.ev &= ~__STACK_PUSHED_FUNC;

    // TODO: i think this is wrong. the `frame_var' function
    // should be dealing with objects in the symbol table already
    assert(lisp_stack_lex_frame_var(&frame, sym.size) == 0, OR_ERR());
    goto yield;
  }

  else if (STACK_POPPED(ev)) {
pop:
    if (sym.size[0] != 0 && frame.reg.i < sym.size[0]) {
      defer_as(err(EARGTOOSMALL));
    }

    frame.stack.ev &= ~__STACK_POPPED;

    fret = ((lisp_fun) sym.dat) ((struct lisp_fun_arg) {
        .size = {sym.size[0], sym.size[1]},
        .litr = {sym.litr[0], sym.litr[1]},
      });
    defer_as(fret.slave);
  }

  done_for(ret);
}
