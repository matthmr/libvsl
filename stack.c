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

static void lisp_stack_lex_frame_var(struct lisp_fun_arg* const restrict reg,
                                     const struct lisp_sym* const sym) {

  // TODO: stub
  switch (sym->typ) {
  case __LISP_VAR_SYM:
    break;
  case __LISP_VAR_SEXP:
    break;
  }
}

// TODO: is this *actually* the right thing to do?
static inline void
lisp_stack_lex_frame_lit(struct lisp_fun_arg* const restrict reg,
                         const struct lisp_lex_stack stack_lex) {

  if (stack_lex.expr) {
    reg->mem.sexp = stack_lex.mem.sexp;
    reg->typ      = __LISP_VAR_SEXP;
  }
  else {
    reg->mem.hash = stack_lex.mem.hash;
    reg->typ      = __LISP_VAR_HASH;
  }
}

static inline void
lisp_stack_lex_frame_pop(struct lisp_fun_arg* const restrict reg,
                         const struct lisp_fun_arg pop) {

  switch (pop.typ) {
  case __LISP_VAR_GEN:
    reg->mem.gen  = pop.mem.gen;
    break;
  case __LISP_VAR_SYM:
    reg->mem.sym  = pop.mem.sym;
    break;
  case __LISP_VAR_HASH:
    reg->mem.hash = pop.mem.hash;
    break;
  case __LISP_VAR_SEXP:
    reg->mem.sexp = pop.mem.sexp;
    break;
  }
}


struct lisp_fun_ret lisp_stack_lex_frame(struct lisp_stack* stackp) {
  int ret = 0;

  enum lisp_stack_ev ev    = {0};
  struct lisp_fun_arg* reg = NULL;

  DB_MSG_SAFE("[ == ] stack(lex): stack push frame");

  struct lisp_frame frame = {0};

  frame.sym.p = lisp_symtab_get(stackp->typ.lex.mem.hash);

  frame.stack = *stackp;
  frame.reg.i = 0;

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
    reg = frame.reg._ = argp;
  }

  reg->typ     = __LISP_VAR_SYM;
  reg->mem.sym = frame.sym.p.master;
  ++reg;

  DB_FMT("[ == ] stack(lex): stack on argp[%d]", IDX_MH(frame.reg.i));

  // called with pop event still set: it must've been `()'
  if (STACK_POPPED(stackp->ev)) {
    DB_MSG_SAFE("[ == ] stack(lex): immediate pop");
    goto pop;
  }

yield:
  if ((frame.reg.i + 1) >= frame.sym.m.litr[0] &&
      (frame.sym.m.litr[1] == INFINITY ||
       (frame.reg.i + 1) <= frame.sym.m.litr[1])) {
    frame.stack.ev |= __STACK_LIT;

    DB_MSG_SAFE("[ == ] stack(lex): on literal");

    ret = lisp_lex_bytstream(&frame.stack);
    ev  = frame.stack.ev;

    assert(ret & (__LEX_OK | __LEX_DEFER), OR_ERR());

    // TODO: implement this
    if (frame.sym.m.litr[0] != 0 && frame.reg.i >= frame.sym.m.litr[0]) {
      DB_MSG_SAFE("[ == ] stack(lex): stack push literal [masked]");
      frame.stack.typ.lex.over = true;
      exit(0);
    }
    else {
      DB_MSG_SAFE("[ == ] stack(lex): stack push literal");

      if (frame.sym.m.litr[1] != INFINITY &&
          (frame.reg.i + 1) > frame.sym.m.size[1]) {
        defer_as(err(EARGTOOBIG));
      }

      lisp_stack_lex_frame_lit(reg, frame.stack.typ.lex);
      ++reg, ++frame.reg.i;
      frame.stack.typ.lex.expr = false;
    }

    if (STACK_POPPED(ev)) {
      frame.stack.ev &= ~__STACK_LIT;
      goto pop;
    }

    // better to check in the literal branch itself, instead of in every
    // yield call
    if (frame.sym.m.litr[1] != INFINITY &&
        (frame.reg.i + 1) > frame.sym.m.litr[1]) {
      frame.stack.ev &= ~__STACK_LIT;
    }

    goto yield;
  }

  ret = lisp_lex_bytstream(&frame.stack);
  assert(ret & (__LEX_OK | __LEX_DEFER), OR_ERR());

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
      DB_MSG_SAFE("[ == ] stack(lex): stack push variable [masked]");
      if (frame.stack.typ.lex.over) {
      }
      else {
        frame.stack.typ.lex.over = true;
        exit(0);
      }
    }
    else {
      frame.sym.pv = lisp_symtab_get(frame.stack.typ.lex.mem.hash);
      assert(frame.sym.pv.slave == 0, OR_ERR());

      lisp_stack_lex_frame_var(reg, frame.sym.pv.master);
      ++reg, ++frame.reg.i;
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
      DB_MSG_SAFE("[ == ] stack(lex): stack push function [masked]");
      frame.stack.typ.lex.over = true;
      exit(0);
    }
    else {
      frame.pop = lisp_stack_lex_frame(&frame.stack);
      assert(frame.pop.slave == __LISP_FUN_OK, OR_ERR());

      lisp_stack_lex_frame_pop(reg, frame.pop.master);
      ++reg, ++frame.reg.i;
    }

    // undo the mask for `()'
    if (STACK_POPPED(ev)) {
      frame.stack.ev &= ~__STACK_POPPED;
    }

    goto yield;
  }

  else if (STACK_POPPED(ev)) {
pop:
    DB_MSG_SAFE("[ == ] stack(lex): stack pop frame");

    frame.stack.ev &= ~__STACK_POPPED;

    if (frame.reg.i < frame.sym.m.size[0]) {
      defer_as(err(EARGTOOSMALL));
    }

    frame.pop = ((lisp_fun) frame.sym.m.dat) (frame.reg._, frame.reg.i);
    ret       = frame.pop.slave;

    DB_MSG_SAFE("[ == ] stack(lex): next");
  }

  done_for_with(frame.pop, frame.pop.slave = ret);
}
