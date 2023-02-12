#include "stack.h"
#include "debug.h"
#include "err.h"
#include "lex.h"

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

////////////////////////////////////////////////////////////////////////////////

static int
lisp_stack_lex_frame_var(struct lisp_frame* frame, struct lisp_sym sym) {
  int ret = 0;

  DB_MSG("[ == ] stack(lex): stack push variable");

  // TODO: the lisp primitive functions have a iterative callback that can allow
  // the existence of of bound(less) range of variables; that's not implemented

  frame->stack.ev &= ~__STACK_PUSHED_VAR;

  // this function should not have been called
  if (sym.size[0] == 0 ||
      (sym.size[1] != INFINITY && IDX_MH(frame->reg.i) > sym.size[1])) {
    defer_as(err(EARGTOOBIG));
  }
  if (sym.size[0] == 0) {
    defer_as(err(EARGTOOBIG));
  }
  else if (sym.size[1] != INFINITY) {
    assert(frame->reg.i <= sym.size[1], err(EARGTOOBIG));
  }

  struct lisp_sym_ret stret = lisp_symtab_get(frame->stack.typ.lex.hash);

  assert(stret.slave == 0, OR_ERR());

  // TODO: this is probably wrong; we *can* take things as pointers with
  //       (ref x y)
  // frame->reg.dat[frame->reg.i].mem.sym = *stret.master;

  ++frame->reg.i;
  done_for(ret);
}

static inline int
lisp_stack_lex_frame_lit(struct lisp_frame* frame) {
  int ret = 0;

  DB_MSG("[ == ] stack(lex): stack push literal");

  // TODO: stub: this will *only* work if the literal is a symbol; this has to
  // be cached to the SEXP tree, and then clean after the function pops. that
  // way we can also take full SEXP trees as literals
  frame->reg._.argp[frame->reg.i].mem.hash = frame->stack.typ.lex.hash;

  done_for(ret);
}

static inline int
lisp_stack_lex_frame_pop_to(struct lisp_frame* frame,
                            struct lisp_fun_mem* arg) {
  // NOTE: this function is clear of *size assertion* errors

  int ret = 0;

  // TODO: stub
  frame->reg._.argp[frame->reg.i].mem.gen = arg->mem.gen;

  ++frame->reg.i;
  done_for(ret);
}


struct lisp_fun_ret lisp_stack_lex_frame(struct lisp_stack* stackp) {
  int ret                 = 0;

  struct lisp_fun_ret pop = {0};
  struct lisp_frame frame = {0};

  DB_MSG("[ == ] stack(lex): stack push frame");

  frame.stack     = *stackp;
  frame.reg.i     = 0;
  frame.stack.ev &= ~__STACK_PUSHED_FUNC;

  struct lisp_sym_ret stret = lisp_symtab_get(stackp->typ.lex.hash);
  assert(stret.slave == 0, OR_ERR());

  struct lisp_sym sym = *stret.master;
  assert((sym.typ == __LISP_FUN && sym.dat != NULL), err(EISNOTFUNC));

  // allocate the memory; needs to be scoped because this branch does not always
  // run in this function
  {
    // FIXME: this could be heap-allocated
    struct lisp_fun_mem argp[sym.size[0]];
    frame.reg._ = (struct lisp_fun_arg) {
      .argp = argp,
      .argv = sym.size[0],
    };
  }

yield_litr:
  /** see if the root expr asked for literals. if so,
      then ask for the lexer to send its tokens to
      the SEXP tree
  */
  if ((sym.litr[0] != 0 && IDX_MH(frame.reg.i) >= sym.litr[0]) &&
      (sym.litr[1] == INFINITY || IDX_MH(frame.reg.i) <= sym.litr[1])) {
    DB_FMT("[ == ] stack(lex): index %d is literal", frame.reg.i);
    frame.stack.ev |= __STACK_LIT;

yield_litr_lexer:
    ret = LEXER(frame) (&frame.stack);

    // the function popped while reading literals
    switch (ret) {
    case __LEX_POP_LITR:
      // assert that what we have already is enough
      assert((sym.litr[0] != 0 && frame.reg.i >= sym.litr[0]) &&
             (sym.litr[1] == INFINITY || frame.reg.i <= sym.litr[1]),
             err(EARGTOOSMALL));

      frame.stack.ev &= ~__STACK_LIT;

      if (frame.reg.i > 0) {
        assert(lisp_stack_lex_frame_lit(&frame) == 0, OR_ERR());
        ++frame.reg.i;
      }
      goto pop;

    case __LEX_INPUT:
      goto yield_litr_lexer;
    }

    assert(ret == -1, OR_ERR());
    //assert(lisp_stack_lex_frame_lit(&frame) == 0, OR_ERR());
    ++frame.reg.i;

    goto yield_litr;
  }
  else {
    frame.stack.ev &= ~__STACK_LIT;
  }

yield_exp:
  ret = LEXER(frame) (&frame.stack);

  assert((ret == 0 || ret == -1), OR_ERR());

  enum lisp_stack_ev ev = frame.stack.ev;

  // TODO: 'pop' the function with a mask, that way the function can keep
  // calling itself and receiving the extra arguments

  if (STACK_PUSHED_VAR(ev)) {
    assert(lisp_stack_lex_frame_var(&frame, sym) == 0, OR_ERR());
    goto yield_exp;
  }

  else if (STACK_PUSHED_FUNC(ev)) {
    if (sym.size[0] == 0 ||
        (sym.size[1] != INFINITY && IDX_MH(frame.reg.i) > sym.size[1])) {
      defer_as(err(EARGTOOBIG));
    }

    pop = lisp_stack_lex_frame(&frame.stack);
    assert(pop.slave == 0, OR_ERR());

    assert(lisp_stack_lex_frame_pop_to(&frame, pop.master.argp) == 0, OR_ERR());
    goto yield_exp;
  }

  else if (STACK_POPPED(ev)) {
pop:
    DB_MSG("[ == ] stack(lex): stack popped");
    if (sym.size[0] != 0 && frame.reg.i < sym.size[0]) {
      defer_as(err(EARGTOOSMALL));
    }

    frame.stack.ev &= ~__STACK_POPPED;

    return ((lisp_fun) sym.dat) (frame.reg._, &sym);
  }

  done_for_with(pop, pop.slave = ret);
}
