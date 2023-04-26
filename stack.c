#include "debug.h"
#include "lex.h"   // also includes `stack.h'
#include "err.h"
#include "mm.h"

/**
   Return the appropiate `argv' size for symbol @sym

   The preceding 1 is for `argp[0]', and the `DISJOINT' macro returns 1 if the
   function has disjoint arguments, which would mean that *at least one*
   argument should store the lazy expressions from this disjunction

   For example, if we have a function:

       (function p1 p2 [p3 ...])

   Then the argp layout is going to look like:

       0: function
       1: p1
       2: p2
       3: [p3 ...]

    And this functions would return 4
 */
static inline uint __lisp_stack_argv(struct lisp_sym sym) {
  return (1 + sym.size[0] + DISJOINT(sym.size));
}

////////////////////////////////////////////////////////////////////////////////

/**
   LISP function eval stub. Every user-defined function calls this as their
   `dat' field on their symbols
 */
// TODO: STUB
struct lisp_ret lisp_eval_fun(struct lisp_arg* argp, uint argv) {
  return (struct lisp_ret) {0};
}

/**
   Define a new stack frame from base stack @b_stack. Will yield from the SEXP
   tree until a `pop_root' event is sent. Unlike `lisp_stack_lex_frame', the
   objects we're going to parse already exist, and are already valid.
 */
// TODO: please refactor this shit into their own functions
struct lisp_ret lisp_stack_sexp_frame(struct lisp_stack* b_stack) {
  register int ret = 0;

  struct lisp_frame          frame = {
    .stack = (b_stack->ev &= ~__STACK_PUSH_FUN, *b_stack),
    .sym   = {0},
    .pop   = {0},
    .argp  = NULL,
    .argv  = 0,
  };
  struct lisp_arg            argp0 = {0};

  struct lisp_stack* const f_stack = &frame.stack;
  struct lisp_trans           tret = {0};
  register bool              empty = false;

  //// FRAME

  DB_MSG("[ stack ] sexp: new frame");

  frame.argp = &argp0;

  DB_FMT("[ stack ] sexp: yielding for: argp[%d]", frame.argv);

  tret.exp  = f_stack->expr;
  tret.stat = __TRANS_OK;

  // yield argp0
  tret          = lisp_sexp_yield(tret);
  f_stack->expr = tret.exp;

  // push expression: create a new frame, then pop from it
  if (tret.stat & __TRANS_LEFT_EXPR) {
    f_stack->expr = tret.exp;
    frame.pop     = lisp_stack_sexp_frame(f_stack);

    assert(frame.pop.slave == __LISP_OK, OR_ERR());
    assert(frame.pop.master.typ == __LISP_TYP_FUN ||
           frame.pop.master.typ == __LISP_TYP_FUNP, err(EISNOTFUNC));
  }

  // push symbol: symbol is function name
  else if (tret.stat & __TRANS_LEFT_SYM) {
    f_stack->hash = tret.exp->left.sym;
  }

  // nothing: () is nil
  else {
    empty = true;

    f_stack->hash = (struct lisp_hash) {0};
  }

  DB_MSG("[ stack ] sexp: push variable");

  frame.argp->mem.sym = lisp_symtab_get(f_stack->hash);

  assert(frame.argp->mem.sym, OR_ERR());
  assert(frame.argp->typ == __LISP_TYP_FUN  ||
         frame.argp->typ == __LISP_TYP_FUNP, err(EISNOTFUNC));

  frame.sym = *frame.argp->mem.sym;

  const uint s_argv = __lisp_stack_argv(frame.sym);

  frame.argp = mm_alloc(sizeof(struct lisp_sym)*s_argv);
  assert(frame.argp, OR_ERR());

  frame.argp[0].mem.ssym = frame.sym;
  frame.argp[0].typ      = __LISP_TYP_FUN;

  ++frame.argv;

  // argv = 0 -> () -> nil
  if (empty) {
    goto stack_done;
  }

  //// ARGP0

  // argv > 0
  for (;;) {
    DB_FMT("[ stack ] sexp: yielding for: argp[%d]", frame.argv);

    tret = lisp_sexp_yield(tret);

    f_stack->expr = tret.exp;

    if (tret.stat & (__TRANS_LEFT_SYM | __TRANS_RIGHT_SYM)) {
      assert(frame.sym.size[1] == INFINITY ||
             frame.argv <= frame.sym.size[1], err(EARGTOOBIG));

      if (LITERAL(frame.argv, frame.sym.litr)) {
        (frame.argp + frame.argv)->mem.hash = (tret.stat & __TRANS_LEFT_SYM)?
                                                 tret.exp->left.sym:
                                                 tret.exp->right.sym;
        (frame.argp + frame.argv)->typ      = __LISP_TYP_HASH;
      }
      else {
        register struct lisp_sym* vsym =
          lisp_symtab_get((tret.stat & __TRANS_LEFT_SYM)?
                            tret.exp->left.sym:
                            tret.exp->right.sym);
        assert(vsym, OR_ERR());

        // cast `::dat' to its appropiate type
        switch (vsym->typ) {
        case __LISP_TYP_SYM:
        case __LISP_TYP_SYMP:
          (frame.argp + frame.argv)->mem.ssym = *(struct lisp_sym*) vsym->dat;
          break;
        case __LISP_TYP_SEXP:
          (frame.argp + frame.argv)->mem.sexp = (struct lisp_sexp*) vsym->dat;
          break;
        default:
          break;
        }
      }

      ++frame.argv;
    }

    else if (tret.stat & (__TRANS_LEFT_EXPR | __TRANS_RIGHT_EXPR)) {
      frame.pop = lisp_stack_sexp_frame(f_stack);
      assert(frame.pop.slave == __LISP_OK, OR_ERR());
    }

    else if (tret.stat & __TRANS_REBOUND_RIGHT) {
stack_done:
      assert(frame.argv >= frame.sym.size[0], err(EARGTOOSMALL));

      frame.pop = LISP_FUN(frame.sym.dat) (frame.argp, frame.argv);
      assert((int) frame.pop.slave == __LISP_OK, OR_ERR());
    }
  }

  done_for_with(frame.pop, frame.pop.slave = ret);
}

////////////////////////////////////////////////////////////////////////////////

/**
   Pop the return value of a function into a stack variable's argument register
 */
static inline void
lisp_stack_lex_frame_pop(struct lisp_frame* const framep) {
  register struct lisp_arg* const c_argp = (framep->argp + framep->argv);

  struct lisp_arg pop = framep->pop.master;

  DB_FMT("[ stack ] lex: pop as argp[%d]", framep->argv);

  switch (pop.typ) {
  case __LISP_TYP_GEN:
    c_argp->mem.gen  = pop.mem.gen;
    break;
  case __LISP_TYP_SYM:
    c_argp->mem.ssym = pop.mem.ssym;
    break;
  case __LISP_TYP_SYMP:
    c_argp->mem.sym  = pop.mem.sym;
    break;
  case __LISP_TYP_HASH:
    c_argp->mem.hash = pop.mem.hash;
    break;
  case __LISP_TYP_SEXP:
    c_argp->mem.sexp = pop.mem.sexp;
    break;
  default:
    break;
  }

  ++framep->argv;
}

/**
   Handle the `push_var' stack event
 */
static enum lisp_stack_stat
lisp_stack_lex_frame_ev_pv(struct lisp_frame* const framep) {
  register enum lisp_stack_stat ret = __STACK_OK;

  struct lisp_stack* const stack = &framep->stack;
  struct lisp_sym* const    symm = &framep->sym;
  const enum lisp_stack_ev    ev = stack->ev;

  const uint             c_argv = framep->argv;
  struct lisp_arg* const c_argp = (framep->argp + c_argv);

  struct lisp_sym*   vsym = NULL;

  // quoted variable push
  if (STACK_QUOT(ev)) {
    DB_MSG("  -> push: literal");

    if (stack->lit_paren && stack->paren >= stack->lit_paren) {
      DB_MSG("  -> push: symbolic");

      stack->expr = lisp_sexp_sym(stack->expr, stack->hash);
      assert(stack->expr, OR_ERR());
    }
    else {
      assert(symm->litr[1] == INFINITY || c_argv <= symm->litr[1],
             err(EARGTOOBIG));

      stack->ev        &= ~__STACK_QUOT;
      c_argp->mem.hash  = stack->hash;
      c_argp->typ       = __LISP_TYP_HASH;

      ++framep->argv;
    }

    defer_as(__STACK_OK);
  }

  assert(symm->size[1] == INFINITY || c_argv <= symm->size[1],
         err(EARGTOOBIG));

  // lazy variable push
  if (c_argv > symm->size[0]) {
    DB_MSG("  -> push: lazy");

    if (!stack->lazy) {
      stack->lazy = lisp_sexp_node(stack->lazy);
      assert(stack->lazy, OR_ERR());

      ++framep->argv;
    }

    stack->lazy = lisp_sexp_sym(stack->lazy, stack->hash);
    assert(stack->expr, OR_ERR());
  }

  else {
    vsym = lisp_symtab_get(stack->hash);
    assert(vsym, OR_ERR());

    // cast `::dat' to its appropriate type
    switch (vsym->typ) {
    case __LISP_TYP_SYM:
    case __LISP_TYP_SYMP:
      c_argp->mem.ssym = *vsym;
      break;
    case __LISP_TYP_SEXP:
      c_argp->mem.sexp = (struct lisp_sexp*) vsym->dat;
      break;
    default:
      break;
    }

    ++framep->argv;
  }

  done_for(ret);
}

/**
   Handle the `push_fun' stack event
 */
static enum lisp_stack_stat
lisp_stack_lex_frame_ev_pf(struct lisp_frame* const framep) {
  register enum lisp_stack_stat ret = __STACK_OK;

  struct lisp_stack* const stack = &framep->stack;
  struct lisp_sym* const    symm = &framep->sym;
  const enum lisp_stack_ev    ev = stack->ev;

  const uint             c_argv = framep->argv;

  // quoted function push
  if (STACK_QUOT(ev)) {
    DB_MSG("  -> push: literal");

    assert(symm->litr[1] == INFINITY || c_argv <= symm->litr[1],
           err(EARGTOOBIG));

    stack->expr = lisp_sexp_node(stack->expr);
    assert(stack->expr, OR_ERR());

    // since this is always called after top-level, and lit_paren is 0, this
    // will never produce false negatives
    if (stack->lit_paren < stack->paren) {
      stack->lit_paren  = stack->paren;

      // stack->ev        &= ~__STACK_QUOT;
      // stack->expr       = NULL;
      // ++framep->argv;
    }
    else {
      DB_MSG("  -> push: symbolic");
    }

    defer_as(__STACK_OK);
  }

  assert(symm->size[1] == INFINITY || c_argv <= symm->size[1],
         err(EARGTOOBIG));

  // lazy function push
  if (c_argv > symm->size[0]) {
    DB_MSG("  -> push: lazy");

    if (!stack->lazy) {
      stack->lazy = lisp_sexp_node(stack->lazy);
      assert(stack->lazy, OR_ERR());

      ++framep->argv;
    }

    stack->lazy = lisp_sexp_node(stack->lazy);
    assert(stack->lazy, OR_ERR());
  }

  else {
    defer_as(__STACK_NEW);
  }

  done_for(ret);
}

/**
   Handle the `pop' stack event
 */
static enum lisp_stack_stat
lisp_stack_lex_frame_ev_pop(struct lisp_frame* const framep) {
  register enum lisp_stack_stat ret = __STACK_OK;

  struct lisp_stack* const stack = &framep->stack;
  struct lisp_sym* const    symm = &framep->sym;
  const enum lisp_stack_ev    ev = stack->ev;

  const uint              c_argv = framep->argv;
  struct lisp_arg* const  c_argp = (framep->argp + c_argv);

  // quoted pop
  if (STACK_QUOT(ev)) {
    DB_MSG("  -> pop: literal");

    stack->expr = lisp_sexp_end(stack->expr);

    if (stack->lit_paren == (stack->paren + 1)) {
      stack->lit_paren  = 0;
      stack->ev        &= ~__STACK_QUOT;

      c_argp->mem.sexp  = stack->expr;
      c_argp->typ       = __LISP_TYP_SEXP;

      ++framep->argv;
    }
    else {
      // ignore
      DB_MSG("  -> pop: symbolic");
    }

    defer_as(__STACK_OK);
  }

  // we preemptively increment the register index, even if there's no expr
  assert((c_argv - 1) >= symm->size[0], err(EARGTOOSMALL));

  // TODO: lazy pop
  if (stack->lazy) {
    DB_MSG("  -> pop: lazy");

    stack->lazy = NULL;
    defer_as(__STACK_DONE);
  }

  else {
    defer_as(__STACK_DONE);
  }

  done_for(ret);
}

/**
   Handle stack events yielded by the lexer
 */
static inline enum lisp_stack_stat
lisp_stack_lex_frame_handle_ev(struct lisp_frame* const framep) {
  const register enum lisp_stack_ev ev = framep->stack.ev;

  if (STACK_PUSHED_VAR(ev)) {
    DB_MSG("[ stack ] lex: push variable");

    framep->stack.ev &= ~__STACK_PUSH_VAR;

    return lisp_stack_lex_frame_ev_pv(framep);
  }

  else if (STACK_PUSHED_FUN(ev)) {
    DB_MSG("[ stack ] lex: push function");

    framep->stack.ev &= ~__STACK_PUSH_FUN;

    return lisp_stack_lex_frame_ev_pf(framep);
  }

  else if (STACK_POPPED(ev)) {
    DB_MSG("[ stack ] lex: pop frame");

    framep->stack.ev &= ~__STACK_POP;

    return lisp_stack_lex_frame_ev_pop(framep);
  }

  // appease the almighty compiler
  return __STACK_OK;
}

//// STATIC

/**
   Define a new stack frame from base stack @b_stack. Will yield from the lexer
   until a `pop' event is sent
 */
// TODO: which expressions are transient and which are to be saved in the symtab
struct lisp_ret lisp_stack_lex_frame(struct lisp_stack* b_stack) {
  register int ret = 0;

  struct lisp_frame  frame = {
    .stack = (b_stack->ev &= ~__STACK_PUSH_FUN, *b_stack),
    .sym   = {0},
    .pop   = {0},
    .argp  = NULL,
    .argv  = 0,
  };

  struct lisp_arg    argp0 = {0};

  struct lisp_frame* framep = &frame;
  struct lisp_stack* f_stack = &frame.stack;
  register bool      empty = false;

  //// FRAME

  DB_MSG("[ stack ] lex: new frame");

  frame.argp = &argp0;

  DB_FMT("[ stack ] lex: yielding for: argp[%d]", frame.argv);

  // yield argp0
  ret = lisp_lex_yield(f_stack);
  assert(ret == __LEX_OK, OR_ERR());

  /** NOTE: this LISP allows for expressions like:

      ((function a1 a2 ...) b1 b2 ...)

      where `function' returns a symbol, which is a function itself
  */
  if (STACK_PUSHED_FUN(f_stack->ev)) {
    frame.pop = lisp_stack_lex_frame(f_stack);

    assert(frame.pop.slave == __LISP_OK, OR_ERR());
    assert(frame.pop.master.typ == __LISP_TYP_FUN ||
           frame.pop.master.typ == __LISP_TYP_FUNP, err(EISNOTFUNC));

    f_stack->hash = frame.pop.master.mem.sym->hash;
  }
  else if (STACK_POPPED(f_stack->ev)) {
    empty = true;

    f_stack->hash = (struct lisp_hash) {0};
  }

  DB_MSG("[ stack ] lex: push variable");
  f_stack->ev = 0;

  frame.argp->mem.sym = lisp_symtab_get(f_stack->hash);
  assert(frame.argp->mem.sym, OR_ERR());

  frame.argp->typ     = frame.argp->mem.sym->typ;
  assert(frame.argp->typ == __LISP_TYP_FUN  ||
         frame.argp->typ == __LISP_TYP_FUNP, err(EISNOTFUNC));

  frame.sym = *frame.argp->mem.sym;

  const uint s_argv = __lisp_stack_argv(frame.sym);

  frame.argp = mm_alloc(sizeof(struct lisp_sym)*s_argv);
  assert(frame.argp, OR_ERR());

  // argp0 is the frame symbol, just like in LIBC's `main'
  frame.argp[0].mem.ssym = frame.sym;
  frame.argp[0].typ      = __LISP_TYP_FUN;

  ++frame.argv;

  // argv == 0 -> nil
  if (empty) {
    goto stack_done;
  }

  //// ARGP0

  // argv > 0
  for (;;) {
    DB_FMT("[ stack ] lex: yielding for: argp[%d]", frame.argv);

    if (LITERAL(frame.argv, frame.sym.litr)) {
      f_stack->ev |= __STACK_QUOT;
    }

    // yield expressions from the lexer
    ret = lisp_lex_yield(f_stack);
    assert(ret == __LEX_OK || ret == __LEX_NO_INPUT, OR_ERR());

    ret = lisp_stack_lex_frame_handle_ev(framep);

    switch (ret) {
    case __STACK_NEW:
      frame.pop = lisp_stack_lex_frame(f_stack);
      assert(frame.pop.slave == __STACK_DONE, OR_ERR());
      lisp_stack_lex_frame_pop(framep);
      break;

    case __STACK_DONE:
stack_done:
      frame.pop = LISP_FUN(frame.sym.dat) (frame.argp, frame.argv);
      assert((int) frame.pop.slave == __LISP_OK, OR_ERR());

      // TODO: this shouldn't free the whole thing
      mm_free(frame.argp);
      defer_as(__STACK_DONE);
      break;

    default:
      assert(ret == __STACK_OK, OR_ERR());
      break;
    }
  }

  done_for_with(frame.pop, (frame.pop.slave = ret? ret: 0));
}
