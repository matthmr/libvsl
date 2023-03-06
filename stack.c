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

// TODO: stub
static void lisp_stack_lex_frame_var(struct lisp_fun_arg* const restrict reg,
                                     const struct lisp_sym* const sym) {

  switch (sym->typ) {
  case __LISP_VAR_SYM:
    break;
  case __LISP_VAR_SEXP:
    break;
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


// TODO: refactor the expression yielding to its own function
struct lisp_fun_ret lisp_stack_lex_frame(struct lisp_stack* f_stack) {
  int ret = 0;

  enum lisp_stack_ev ev    = {0};
  struct lisp_fun_arg* reg = NULL;
  struct lisp_stack* stack = NULL;
  struct lisp_frame frame  = {0};

  DB_MSG_SAFE("[ == ] stack(lex): push frame");

  frame.sym.p = lisp_symtab_get(f_stack->typ.lex.mem.hash);
  assert(frame.sym.p.slave == 0, OR_ERR());

  frame.stack = *f_stack;
  stack       = &frame.stack;
  frame.reg.i = 0;

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
  if (STACK_POPPED(f_stack->ev)) {
    DB_MSG_SAFE("[ == ] stack(lex): immediate pop");
    goto pop;
  }

yield:

  // yield literal:
  if ((frame.reg.i + 1) >= frame.sym.m.litr[0] &&
      (frame.sym.m.litr[1] == INFINITY ||
       (frame.reg.i + 1) <= frame.sym.m.litr[1])) {
    frame.stack.ev |= __STACK_LIT;

    DB_MSG_SAFE("[ == ] stack(lex): on literal");

    ret = lisp_lex_bytstream(stack);
    ev  = frame.stack.ev;

    assert(ret == __LEX_OK || ret == __LEX_DEFER, OR_ERR());

    // NOTE: `::lit_expr` *doesn't* trigger `PUSH_VAR`
    if (STACK_PUSHED_VAR(ev) || stack->typ.lex.lit_expr) {

      /** literal range is disjoint/infinite: save the memory a temporary SEXP
          tree; lazily evaluated
       */
      if (frame.sym.m.litr[0] != 0 && frame.reg.i >= frame.sym.m.litr[0]) {
        DB_MSG_SAFE("[ == ] stack(lex): stack push literal [masked]");

        ret = lisp_sexp_node_add(sexp_pp);
        assert(ret == 0, OR_ERR());

        if (!frame.stack.typ.lex.lazy) {
          frame.stack.typ.lex.lazy = lisp_sexp_get_head();
        }
      }

      DB_MSG_SAFE("[ == ] stack(lex): stack push literal");

      if (frame.sym.m.litr[1] != INFINITY &&
          (frame.reg.i + 1) > frame.sym.m.size[1]) {
        defer_as(err(EARGTOOBIG));
      }

      if (frame.stack.typ.lex.lazy) {
        goto yield;
      }

      if (stack->typ.lex.lit_expr) {
        stack->typ.lex.lit_expr = false;
        reg->mem.sexp = stack->typ.lex.mem.sexp;
        reg->typ      = __LISP_VAR_SEXP;
      }
      else {
        frame.stack.ev &= ~__STACK_PUSHED_VAR;
        reg->mem.hash   = stack->typ.lex.mem.hash;
        reg->typ        = __LISP_VAR_HASH;
      }

      ++reg, ++frame.reg.i;
    }

    if (STACK_POPPED(ev)) {
      frame.stack.ev &= ~__STACK_LIT;
      goto pop;
    }

    if (frame.sym.m.litr[1] != INFINITY &&
        (frame.reg.i + 1) > frame.sym.m.litr[1]) {
      frame.stack.ev &= ~__STACK_LIT;
    }

    goto yield;
  }

  // yield expression:

  ret = lisp_lex_bytstream(stack);
  assert(ret == __LEX_OK || ret == __LEX_DEFER, OR_ERR());

  ev = frame.stack.ev;

  // TODO: 'pop' the function with a mask, that way the function can keep
  // calling itself and receiving the extra arguments

  if (STACK_PUSHED_VAR(ev)) {
    DB_FMT("[ == ] stack(lex): stack push variable argp[%d]", frame.reg.i);

    frame.stack.ev &= ~__STACK_PUSHED_VAR;

    if (frame.sym.m.size[1] != INFINITY &&
        (frame.reg.i + 1) > frame.sym.m.size[1]) {
      defer_as(err(EARGTOOBIG));
    }

    if ((frame.reg.i + 1) > frame.sym.m.size[0]) {
      DB_MSG_SAFE("[ == ] stack(lex): stack push variable [masked]");

      ret = lisp_sexp_sym(sexp_pp, frame.stack.typ.lex.mem.hash);
      assert(ret == 0, OR_ERR());

      if (!frame.stack.typ.lex.lazy) {
        frame.stack.typ.lex.lazy = lisp_sexp_get_head();
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

    if (frame.sym.m.size[1] != INFINITY &&
        (frame.reg.i + 1) > frame.sym.m.size[1]) {
      defer_as(err(EARGTOOBIG));
    }

    if ((frame.reg.i + 1) > frame.sym.m.size[0]) {
      DB_MSG_SAFE("[ == ] stack(lex): stack push function [masked]");

      ret = lisp_sexp_node_add(sexp_pp);
      assert(ret == 0, OR_ERR());

      if (!frame.stack.typ.lex.lazy) {
        frame.stack.typ.lex.lazy = lisp_sexp_get_head();
      }
    }
    else {
      frame.pop = lisp_stack_lex_frame(stack);
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
