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
  frame.sym.p     = lisp_symtab_get(stackp->typ.lex.mem.hash);

  frame.stack     = *stackp;
  frame.reg.i     = 1;
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
    struct lisp_fun_arg args[frame.sym.m.size[0] + 1];
    frame.reg._ = args;
  }

  frame.reg._[0].typ     = __LISP_VAR_SYM;
  frame.reg._[0].mem.sym = frame.sym.p.master;

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
  if ((frame.sym.m.litr[0] != 0 && frame.reg.i >= frame.sym.m.litr[0]) &&
      (frame.sym.m.litr[1] == INFINITY || frame.reg.i <= frame.sym.m.litr[1])) {
    DB_FMT("[ == ] stack(lex): stack push literal argp[%d]", frame.reg.i);
    frame.stack.ev |= __STACK_LIT;

yield_litr_lexer:
    ret = lisp_lex_bytstream(&frame.stack);

    switch (ret) {
    case __LEX_POP_LITR:
      DB_MSG("[ ==  ] stack(lex.handle): __LEX_POP_LITR");
      assert(frame.sym.m.litr[0] == 0, err(EARGTOOSMALL));
      frame.stack.ev &= ~__STACK_LIT;
      goto pop;

    case __LEX_INPUT:
      /** 1. call `lisp_lex_bytstream' again, which immediately exits without
             setting anything as a stack event, which exits this function
          2. call `lisp_lex_bytstream' with a bigger paren level when in a
             literal, which makes it send its tokens to the SEXP tree
      */
      DB_MSG("[ == ] stack(lex.handle): __LEX_INPUT");
      goto yield_litr_lexer;

    default:
      DB_MSG("[ == ] stack(lex.handle): default");
      assert(ret == __LEX_OK || ret == __LEX_DEFER, OR_ERR());
      break;
    }

    DB_MSG("[ == ] stack(lex): stack push literal");
    // frame.reg._[frame.reg.i].mem.hash = frame.stack.typ.lex.hash;
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
    DB_FMT("[ == ] stack(lex): stack push variable argp[%d]", frame.reg.i);

    frame.stack.ev &= ~__STACK_PUSHED_VAR;

    if (frame.sym.m.size[1] == INFINITY) {
      // TODO: this should mask the arguments as SEXP trees, which *should* be
      //       stored in the stack as an SEXP pointer (struct lisp_sexp*)
      DB_MSG("[ == ] stack(lex): stack variable triggered infinity");
    }
    else if (frame.reg.i > frame.sym.m.size[1]) {
      defer_as(err(EARGTOOBIG));
    }
    else {
      frame.sym.pv = lisp_symtab_get(frame.stack.typ.lex.mem.hash);
      assert(frame.sym.pv.slave == 0, OR_ERR());

      // frame.reg._[frame.reg.i].mem.sym = frame.sym.pv.master;
      ++frame.reg.i;
    }

    goto yield_exp;
  }

  else if (STACK_PUSHED_FUNC(ev)) {
    DB_FMT("[ == ] stack(lex): stack push function argp[%d]", frame.reg.i);

    frame.stack.ev &= ~__STACK_PUSHED_FUNC;

    if (frame.sym.m.size[1] == INFINITY) {
      // TODO: this should mask the arguments as SEXP trees, which *should* be
      //       stored in the stack as an SEXP pointer (struct lisp_sexp*)
      DB_MSG("[ == ] stack(lex): stack function triggered infinity");
    }
    else if (frame.reg.i > frame.sym.m.size[1]) {
      defer_as(err(EARGTOOBIG));
    }
    else {
      frame.pop = lisp_stack_lex_frame(&frame.stack);
      assert(frame.pop.slave == __LISP_FUN_OK, OR_ERR());

      assert(lisp_stack_lex_frame_pop_to(&frame, frame.pop.master) == 0,
             OR_ERR());

      // () also sets __STACK_POP
      if (STACK_POPPED(ev)) {
        goto pop;
      }
    }

    goto yield_exp;
  }

  else if (STACK_POPPED(ev)) {
pop:
    DB_MSG("[ == ] stack(lex): stack popped");

    frame.stack.ev &= ~__STACK_POPPED;

    if (frame.reg.i < frame.sym.m.size[0]) {
      defer_as(err(EARGTOOSMALL));
    }

    return ((lisp_fun) frame.sym.m.dat) (frame.reg._, frame.reg.i);
  }

  done_for_with(frame.pop, frame.pop.slave = ret);
}
