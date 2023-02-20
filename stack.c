#include "debug.h"
#include "lex.h"   // also includes `stack.h'

// TODO: stub on the SEXP stack

////////////////////////////////////////////////////////////////////////////////

static int lisp_stack_sexp_frame_var(struct lisp_frame* frame) {
  return 0;
}

static int lisp_stack_sexp_frame(struct lisp_stack* stack) {
  return 0;
}


void lisp_stack_sexp_push(struct lisp_stack* stack, POOL_T* mpp,
                          struct lisp_sexp* head) {
  return;
}

void lisp_stack_sexp_push_var(struct lisp_stack* stack, POOL_T* mpp,
                              struct lisp_sexp* head, enum lisp_stack_ev ev) {
  return;
}

void lisp_stack_sexp_pop(struct lisp_stack* stack, POOL_T* mpp,
                         struct lisp_sexp* head) {
  return;
}

////////////////////////////////////////////////////////////////////////////////

static uint lisp_stack_lex_frame_var(struct lisp_fun_arg* reg,
                                     struct lisp_sym* sym, uint idx) {

  // TODO: stub
  switch (sym->typ) {
  case __LISP_VAR_SYM:
    break;
  case __LISP_VAR_SEXP:
    break;
  }

  return ++idx;
}

static uint lisp_stack_lex_frame_lit(struct lisp_fun_arg* reg,
                                     struct lisp_lex_stack* stack,
                                     uint idx) {
  if (stack->expr) {
    reg->mem.sym = stack->mem.sym;
    reg->typ     = __LISP_VAR_SYMP;
  }
  else {
    reg->mem.hash = stack->mem.hash;
    reg->typ      = __LISP_VAR_HASH;
  }

  return ++idx;
}


struct lisp_fun_ret lisp_stack_lex_frame(struct lisp_stack* stackp) {
  int ret = 0;

  enum lisp_stack_ev ev    = {0};
  struct lisp_fun_arg* reg = NULL;

  DB_MSG("[ == ] stack(lex): stack push frame");

  struct lisp_frame frame = {0};

  frame.sym.p     = lisp_symtab_get(stackp->typ.lex.mem.hash);

  frame.stack     = *stackp;
  frame.reg.i     = 0;

  // give the parent error precedence over `EISNOTFUNC'
  assert(frame.sym.p.slave == 0, OR_ERR());
  assert(frame.sym.p.master->typ == __LISP_VAR_FUN &&
         frame.sym.p.master->dat != NULL,
         err(EISNOTFUNC));

  frame.sym.m = *frame.sym.p.master;

  const uint f_argv = (1 + frame.sym.m.size[0] +
                       (int) (frame.sym.m.size[0] != frame.sym.m.size[1]));

  // TODO: this could be heap-allocated
  {
    struct lisp_fun_arg argp[f_argv];
    frame.reg._ = argp;
  }

  frame.reg._[0].typ     = __LISP_VAR_SYM;
  frame.reg._[0].mem.sym = frame.sym.p.master;

  reg = frame.reg._;

  DB_FMT("[ == ] stack(lex): stack on argp[%d]", IDX_MH(frame.reg.i));

  // called with pop event still set: it must've been `()'
  if (STACK_POPPED(stackp->ev)) {
    DB_MSG("[ == ] stack(lex): immediate pop");
    goto pop;
  }

yield:
  if ((frame.sym.m.litr[0] != 0 && frame.reg.i >= frame.sym.m.litr[0]) &&
      (frame.sym.m.litr[1] == INFINITY || frame.reg.i <= frame.sym.m.litr[1])) {
    frame.stack.ev |= __STACK_LIT;

yield_wait:
    ret = lisp_lex_bytstream(&frame.stack);
    ev  = frame.stack.ev;

    // the paren level of the literal is bigger than ours: the lexer has the
    // responsibility of calling the SEXP functions
    if (ret == __LEX_SEXP_HANDLED) {
      DB_MSG("[ == ] stack(lex.handle): __LEX_SEXP_HANDLED");
      goto yield_wait;
    }

    assert(ret == __LEX_OK || ret == __LEX_DEFER, OR_ERR());

    // TODO: implement this
    if (frame.sym.m.litr[0] != 0 && frame.reg.i >= frame.sym.m.litr[0]) {
      DB_MSG("[ == ] stack(lex): stack push literal [masked]");
      exit(0);
    }
    else {
      DB_MSG("[ == ] stack(lex): stack push literal");

      if (frame.sym.m.litr[1] != INFINITY &&
          (frame.reg.i + 1) > frame.sym.m.size[1]) {
        defer_as(err(EARGTOOBIG));
      }

      ++reg;
      frame.reg.i = lisp_stack_lex_frame_lit(frame.reg._, &frame.stack.typ.lex,
                                             frame.reg.i);
    }

    if (STACK_POPPED(ev)) {
      assert(frame.reg.i >= frame.sym.m.litr[0], err(EARGTOOSMALL));
      frame.stack.ev &= ~__STACK_LIT;
      goto pop;
    }

    goto yield;
  }

  ret = lisp_lex_bytstream(&frame.stack);
  assert(ret == __LEX_OK || ret == __LEX_DEFER, OR_ERR());

  ev = frame.stack.ev;

  // TODO: 'pop' the function with a mask, that way the function can keep
  // calling itself and receiving the extra arguments

  if (STACK_PUSHED_VAR(ev)) {
    DB_FMT("[ == ] stack(lex): stack push variable argp[%d]", frame.reg.i);

    frame.stack.ev &= ~__STACK_PUSHED_VAR;
    ++reg;

    if (frame.sym.m.size[1] != INFINITY &&
        (frame.reg.i + 1) > frame.sym.m.size[1]) {
      defer_as(err(EARGTOOBIG));
    }

    if ((frame.reg.i + 1) > frame.sym.m.size[0]) {
      DB_MSG("[ == ] stack(lex): stack push variable [masked]");
    }
    else {
      frame.sym.pv = lisp_symtab_get(frame.stack.typ.lex.mem.hash);
      assert(frame.sym.pv.slave == 0, OR_ERR());

      frame.reg.i = lisp_stack_lex_frame_var(frame.reg._, frame.sym.pv.master,
                                             frame.reg.i);
    }

    if (STACK_POPPED(ev)) {
      goto pop;
    }

    goto yield;
  }

  else if (STACK_PUSHED_FUNC(ev)) {
    DB_FMT("[ == ] stack(lex): stack push function argp[%d]", frame.reg.i);

    frame.stack.ev &= ~__STACK_PUSHED_FUNC;
    ++reg;

    if (frame.sym.m.size[1] != INFINITY &&
        (frame.reg.i + 1) > frame.sym.m.size[1]) {
      defer_as(err(EARGTOOBIG));
    }

    if ((frame.reg.i + 1) > frame.sym.m.size[0]) {
      DB_MSG("[ == ] stack(lex): stack push function [masked]");
    }
    else {
      frame.pop = lisp_stack_lex_frame(&frame.stack);
      assert(frame.pop.slave == __LISP_FUN_OK, OR_ERR());

      // TODO: stub
      switch (frame.pop.master.typ) {
      case __LISP_VAR_GEN:
        reg->mem.gen  = frame.pop.master.mem.gen;
        break;
      case __LISP_VAR_SYM:
        reg->mem.sym  = frame.pop.master.mem.sym;
        break;
      case __LISP_VAR_HASH:
        reg->mem.hash = frame.pop.master.mem.hash;
        break;
      case __LISP_VAR_SEXP:
        reg->mem.sexp = frame.pop.master.mem.sexp;
        break;
      }

      ++frame.reg.i;

      // NOTE: this is utterly fucking retarded
      // frame.reg.i = lisp_stack_lex_frame_pop(frame.reg._, frame.pop.master,
      //                                        frame.reg.i);
    }

    // undo the mask for `()'
    if (STACK_POPPED(ev)) {
      frame.stack.ev &= ~__STACK_POPPED;
    }

    goto yield;
  }

  else if (STACK_POPPED(ev)) {
pop:
    DB_MSG("[ == ] stack(lex): stack pop frame");

    frame.stack.ev &= ~__STACK_POPPED;

    if (frame.reg.i < frame.sym.m.size[0]) {
      defer_as(err(EARGTOOSMALL));
    }

    frame.pop = ((lisp_fun) frame.sym.m.dat) (frame.reg._, frame.reg.i);
    ret       = frame.pop.slave;

    DB_MSG("[ == ] stack(lex): next");
  }

  done_for_with(frame.pop, frame.pop.slave = ret);
}
