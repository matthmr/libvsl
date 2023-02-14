#include "debug.h"
#include "lex.h"   // also includes `stack.h'

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

static inline int
lisp_stack_lex_frame_pop_to(struct lisp_frame* frame, struct lisp_fun_arg arg) {
  // NOTE: this function is clear of *size assertion* errors

  int ret = 0;

  enum lisp_sym_typ    typ = arg.typ;
  struct lisp_fun_arg* f_arg = (frame->reg._ + frame->reg.i);

  f_arg->typ = typ;

  // TODO: stub
  switch (typ) {
  case __LISP_VAR_GEN:
    f_arg->mem.gen = arg.mem.gen;
    break;
  case __LISP_VAR_SYM:
    break;
  case __LISP_VAR_HASH:
    f_arg->mem.hash = arg.mem.hash;
    break;
  case __LISP_VAR_SEXP:
    break;
  }

  ++frame->reg.i;
  done_for(ret);
}


struct lisp_fun_ret lisp_stack_lex_frame(struct lisp_stack* stackp) {
  int ret = 0;

  DB_MSG("[ == ] stack(lex): stack push frame");

  struct lisp_frame frame = {0};
  frame.sym.p     = lisp_symtab_get(stackp->typ.lex.hash);

  frame.stack     = *stackp;
  frame.reg.i     = 0;
  frame.stack.ev &= ~__STACK_PUSHED_FUNC;

  // give the parent error precedence over `EISNOTFUNC'
  assert(frame.sym.p.slave == 0, OR_ERR());
  assert(frame.sym.p.master->typ == __LISP_VAR_FUN &&
         frame.sym.p.master->dat != NULL,
         err(EISNOTFUNC));

  frame.sym.m = *frame.sym.p.master;

  // allocate the memory; needs to be scoped because this branch does not always
  // run in this function
  {
    // FIXME: this could be heap-allocated
    struct lisp_fun_arg args[frame.sym.m.size[0]];
    frame.reg._ = args;
  }

yield_litr:
  /** see if the function asked for literals. if so, then ask for the lexer to
      send its tokens to us directly, unless there's an expression while
      gathering the literals, the lexer will immediately go back to us

      NOTE: literals are *hashes* (struct lisp_hash) not *symbols*
            (struct lisp_sym). the value of a literal can be changed externally.
            e.g:

      >>> (set x (quot y))
      >>> (set y nil)
      >>> (eval x)
      ... nil
      >>> (set y t)
      >>> (eval x)
      ... t
  */
  if ((frame.sym.m.litr[0] != 0 &&
       IDX_MH(frame.reg.i) >= frame.sym.m.litr[0]) &&
      (frame.sym.m.litr[1] == INFINITY ||
       IDX_MH(frame.reg.i) <= frame.sym.m.litr[1])) {
    DB_FMT("[ == ] stack(lex): index %d is literal", frame.reg.i);
    frame.stack.ev |= __STACK_LIT;

yield_litr_lexer:
    ret = lisp_lex_bytstream(&frame.stack);

    // the function popped while reading literals
    switch (ret) {
    case __LEX_POP_LITR:
      // assert that what we have already is enough
      assert((frame.sym.m.litr[0] != 0 &&
              frame.reg.i >= frame.sym.m.litr[0]) &&
             (frame.sym.m.litr[1] == INFINITY ||
              frame.reg.i <= frame.sym.m.litr[1]),
             err(EARGTOOSMALL));

      frame.stack.ev &= ~__STACK_LIT;

      // push first, then pop
      if (frame.reg.i > 0) {
        DB_MSG("[ == ] stack(lex): stack push literal");
        frame.reg._[frame.reg.i].mem.hash = frame.stack.typ.lex.hash;
        ++frame.reg.i;
      }
      goto pop;

    case __LEX_INPUT:
      goto yield_litr_lexer;

    default:
      assert(ret == __LEX_OK || ret == __LEX_DEFER, OR_ERR());
      break;
    }

    DB_MSG("[ == ] stack(lex): stack push literal");
    frame.reg._[frame.reg.i].mem.hash = frame.stack.typ.lex.hash;
    ++frame.reg.i;
    goto yield_litr;
  }
  else {
    frame.stack.ev &= ~__STACK_LIT;
  }

yield_exp:
  ret = lisp_lex_bytstream(&frame.stack);
  assert(ret == __LEX_OK || ret == __LEX_DEFER, OR_ERR());

  enum lisp_stack_ev ev = frame.stack.ev;

  // TODO: 'pop' the function with a mask, that way the function can keep
  // calling itself and receiving the extra arguments

  if (STACK_PUSHED_VAR(ev)) {
    DB_MSG("[ == ] stack(lex): stack push variable");

    frame.stack.ev &= ~__STACK_PUSHED_VAR;

    // this function should not have been called
    if (frame.sym.m.size[0] == 0 ||
        (frame.sym.m.size[1] != INFINITY &&
         IDX_MH(frame.reg.i) > frame.sym.m.size[1])) {
      defer_as(err(EARGTOOBIG));
    }

    if (frame.sym.m.size[0] == 0) {
      defer_as(err(EARGTOOBIG));
    }
    else if (frame.sym.m.size[1] != INFINITY) {
      assert(frame.reg.i <= frame.sym.m.size[1], err(EARGTOOBIG));
    }

    frame.sym.pv = lisp_symtab_get(frame.stack.typ.lex.hash);
    assert(frame.sym.pv.slave == 0, OR_ERR());

    frame.reg._[frame.reg.i].mem.sym = frame.sym.pv.master;
    ++frame.reg.i;
    goto yield_exp;
  }

  else if (STACK_PUSHED_FUNC(ev)) {
    frame.stack.ev &= ~__STACK_PUSHED_FUNC;

    if (frame.sym.m.size[0] == 0 ||
        (frame.sym.m.size[1] != INFINITY &&
         IDX_MH(frame.reg.i) > frame.sym.m.size[1])) {
      defer_as(err(EARGTOOBIG));
    }

    frame.pop = lisp_stack_lex_frame(&frame.stack);
    assert(frame.pop.slave == __LISP_FUN_OK, OR_ERR());

    assert(lisp_stack_lex_frame_pop_to(&frame, frame.pop.master) == 0,
           OR_ERR());
    goto yield_exp;
  }

  else if (STACK_POPPED(ev)) {
pop:
    DB_MSG("[ == ] stack(lex): stack popped");
    if (frame.sym.m.size[0] != 0 &&
        frame.reg.i < frame.sym.m.size[0]) {
      defer_as(err(EARGTOOSMALL));
    }

    frame.stack.ev &= ~__STACK_POPPED;

    return ((lisp_fun) frame.sym.m.dat)
      (frame.reg._, frame.reg.i, frame.sym.p.master);
  }

  done_for_with(frame.pop, frame.pop.slave = ret);
}
