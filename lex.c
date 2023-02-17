#include <unistd.h>

#include "debug.h"
#include "lex.h"   // also includes `symtab.h'

// TODO: try to make these global variables stack-local (somehow)

static struct lisp_lex lex  = {0};

static int iofd             = 0;
static char iobuf[IOBLOCK];

static void lisp_lex_ev(enum lisp_lex_ev ev) {
  if (ev & __LISP_EV_SYMBOL_OUT) {
    DB_MSG("[ == ] lex(ev): symbol out");

    lex.master.ev &= ~__LISP_EV_SYMBOL_IN;
    lex.master.ev |= __LISP_EV_SYMBOL_OUT;
    inc_hash_done(&lex.master.hash);
  }

  else if (ev & __LISP_EV_PAREN_IN) {
    DB_MSG("[ == ] lex(ev): paren in");

    lex.master.ev |= __LISP_EV_PAREN_IN;
    ++lex.master.paren;

    if (lex.master.ev & __LISP_EV_SYMBOL_IN) {
      lisp_lex_ev(__LISP_EV_SYMBOL_OUT);
    }
  }

  else if (ev & __LISP_EV_PAREN_OUT) {
    DB_MSG("[ == ] lex(ev): paren out");

    lex.master.ev |= __LISP_EV_PAREN_OUT;

    if (lex.master.ev & __LISP_EV_SYMBOL_IN) {
      lisp_lex_ev(__LISP_EV_SYMBOL_OUT);
    }

    if (lex.master.paren == 0) {
      lex.slave = err(EIMBALANCED);
      return;
    }
  }
}

static inline void lisp_lex_whitespace(void) {
  if (lex.master.ev & __LISP_EV_SYMBOL_IN) {
    lisp_lex_ev(__LISP_EV_SYMBOL_OUT);
  }
}

static inline void lisp_lex_csym(char c) {
  lex.master.ev |= __LISP_EV_SYMBOL_IN;

  if (!__LISP_ALLOWED_IN_NAME(c)) {
    lex.slave = err(ENOTALLOWED);
    return;
  }

  struct lisp_hash_ret ret = inc_hash(lex.master.hash, c);
  lex.master.hash = ret.master;
  lex.slave       = ret.slave;

  DB_FMT("[ == ] lex(c): symbol char (%c) (%d)", c, lex.master.hash.sum);
}

static struct lisp_lex_ev_ret
lisp_lex_handle_ev(enum lisp_lex_ev lev, struct lisp_stack* stack,
                   bool prime, uint cb_idx) {
  struct lisp_lex_ev_ret evret = {
    .master = prime,
    .slave  = 0,
  };

  enum lisp_stack_ev sev = stack->ev;

  // primed:     ...a -> push_func
  // not primed: ...a -> push_var
  if (lev & __LISP_EV_SYMBOL_OUT) {
    // we prioritize `SYMBOL_OUT' over `PAREN_OUT'

    lex.master.cb_idx  = cb_idx + ((lev & __LISP_EV_PAREN_IN)? 1: 0);
    lex.master.ev     &= ~__LISP_EV_SYMBOL_OUT;

    stack->typ.lex.mem.hash = lex.master.hash;
    hash_done(&lex.master.hash);

    if (STACK_QUOT(sev)) {
      // same paren level as the function of the current literal: defer;
      // will be saved in the frame
      if ((lex.master.paren+1) == stack->typ.lex.paren) {
        defer_for_as(evret.slave, __LEX_DEFER);
      }

      // bigger paren level than the function of the current literal: save in a
      // temporary memory on the SEXP tree
      else {
        lisp_sexp_sym(sexp_pp, stack->typ.lex.mem.hash);
        defer_for_as(evret.slave, __LEX_INPUT);
      }
    }

    if (prime) {
      evret.master  = false;
      stack->ev    |= __STACK_PUSH_FUNC;
    }
    else {
      stack->ev    |= __STACK_PUSH_VAR;
    }

    defer_for_as(evret.slave, __LEX_DEFER);
  }

  // ...( -> push
  else if (lev & __LISP_EV_PAREN_IN) {
    // we prioritize `SYMBOL_OUT' over `PAREN_IN'

    lex.master.cb_idx = IDX_MH(cb_idx);
    lex.master.ev &= ~__LISP_EV_PAREN_IN;

    // if (lev & __LISP_EV_SYMBOL_OUT) {
    //   lex.master.ev &= ~__LISP_EV_SYMBOL_OUT;
    //   stack->ev     |= __STACK_PUSH_VAR;
    //   hash_done(&lex.master.hash);
    //   defer_for_as(evret.slave, __LEX_DEFER);
    // }

    if (STACK_QUOT(sev)) {
      stack->typ.lex.expr = true;
      lisp_sexp_node_add(sexp_pp);
      defer_for_as(evret.slave, 0);
    }
    else {
      stack->typ.lex.paren = (lex.master.paren+1);
    }

    if (prime) {
      /** the core VSL interpreter doesn't allow function names to be the
          return value of a hash-changing function, e.g:

          (add-prefix sym) -> sym-prefix
          ((add-prefix function) arg) -> (function-prefix arg) -> ...

          the code above is not allowed because the stack schema is unable
          to communicate with its parents
      */
      defer_for_as(evret.slave, err(ENOHASHCHANGING));
    }
    else {
      DB_MSG("[ == ] lex(ev): prime for stack");
      evret.master  = true;
      defer_for_as(evret.slave, 0);
    }
  }

  // ...) -> pop
  else if (lev & __LISP_EV_PAREN_OUT) {
    lex.master.cb_idx  = IDX_MH(cb_idx);
    lex.master.ev     &= ~__LISP_EV_PAREN_OUT;
    stack->ev         |= __STACK_POP;

    --lex.master.paren;

    if (STACK_QUOT(sev)) {
      // same paren level as the function of the current literal: defer
      if ((lex.master.paren+1) == stack->typ.lex.paren) {
        lisp_sexp_end(sexp_pp);
        defer_for_as(evret.slave, __LEX_DEFER);
      }

      // the function of the current literal popped without any expr: defer
      else if ((lex.master.paren+1) < stack->typ.lex.paren) {
        defer_for_as(evret.slave, __LEX_POP_LITR);
      }

      // bigger paren level than the function of the current literal: continue
      else {
        defer_for_as(evret.slave, 0);
      }
    }

    if (prime) {
      evret.master  = false;

      // takes precendence over `__STACK_POP'
      stack->ev |= __STACK_PUSH_FUNC;
      stack->typ.lex.mem.hash = (struct lisp_hash) {0};
    }

    defer_for_as(evret.slave, __LEX_DEFER);
  }

  done_for(evret);
}

static inline void lisp_lex_c(char c) {
  switch (c) {
  case __LISP_C_PAREN_OPEN:
    DB_MSG("[ == ] lex(c): paren open");
    lisp_lex_ev(__LISP_EV_PAREN_IN);
    break;
  case __LISP_C_PAREN_CLOSE:
    DB_MSG("[ == ] lex(c): paren close");
    lisp_lex_ev(__LISP_EV_PAREN_OUT);
    break;
  case __LISP_C_WHITESPACE:
    lisp_lex_whitespace();
    break;
  default:
    lisp_lex_csym(c);
    break;
  }
}

static int parse_bytstream_feed(void) {
  int ret = 0;

  lex.master.size = read(iofd, iobuf, IOBLOCK);
  assert(lex.master.size != -1, err(EREAD));

  lex.master.cb_idx = 0;

  done_for(ret);
}

/** at any given time, there's at most *two* of this function in the call stack:
      - one as the stack base
      - another as a callback from the lexer asking for more bytes
 */
static int parse_bytstream_stack(struct lisp_stack* stack) {
  int ret = parse_bytstream_feed();

  assert(ret == 0, OR_ERR());

  // feed `iobuf' to the lexer, listen for callbacks
lex:
  ret = lisp_lex_bytstream(stack);

  // no error, no input: exit
  if (ret == __LEX_INPUT) {
    defer_as(0);
  }

  assert(ret == __LEX_OK || ret == __LEX_DEFER, OR_ERR());

  /** NOTE: these are the only callbacks issued by `lisp_lex_bytstream'
            that `parse_bytstream_stack' can handle, the rest are handled
            by the stack frame
   */

  // push variable (top level)
  if (STACK_PUSHED_VAR(stack->ev)) {
    stack->ev &= ~__STACK_PUSHED_VAR;

    DB_MSG("TODO: implement top-level symbol resolution");

    goto lex;
  }

  // push function
  else if (STACK_PUSHED_FUNC(stack->ev)) {
    stack->ev &= ~__STACK_PUSHED_FUNC;
    lex.slave = lisp_stack_lex_frame(stack).slave;
  }

  // give the parent error precedence over `EIMBALANCED'
  assert(lex.slave == 0, OR_ERR());
  assert(lex.master.paren == 0, err(EIMBALANCED));

  stack->ev     = 0;
  lex.master.ev = 0;
  goto lex;

  done_for(ret);
}

////////////////////////////////////////////////////////////////////////////////

int lisp_lex_bytstream(struct lisp_stack* stack) {
  static bool prime = false;

  int  ret  = 0;
  uint size = 0;

  struct lisp_lex_ev_ret evret = {0};

feed:
  size = lex.master.size;

  for (uint i = lex.master.cb_idx; i < size; i++) {
    lisp_lex_c(iobuf[i]);
    assert(lex.slave == 0, OR_ERR());

    evret = lisp_lex_handle_ev(lex.master.ev, stack, prime, i);
    prime = evret.master;

    if (evret.slave < 0) {
      defer_as(evret.slave);
    }

    assert_for(evret.slave == 0, OR_ERR(), lex.slave);
  }

  if (size == 0) {
    assert(lex.master.paren == 0 && !(lex.master.ev & __LISP_EV_SYMBOL_IN),
           err(EIMBALANCED));

    // no error but also nothing else in the buffer: ask to stop
    defer_as(__LEX_INPUT);
  }

  assert(parse_bytstream_feed() == 0, OR_ERR());
  goto feed;

  done_for(ret);
}

int parse_bytstream(int fd) {
  iofd = fd;

  struct lisp_stack stack;
  stack.typ.lex.expr = false;

  lex.master.ev = 0;
  lex.slave     = 0;

  // we need a stack-local `stack', so we wrap `parse_bytstream'
  // with a `*_stack' function. this means this function does not
  // receive or send any callbacks from/to the stack
  return parse_bytstream_stack(&stack);
}
