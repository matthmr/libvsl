#include <unistd.h>

#include "debug.h"
#include "lex.h"   // also includes `symtab.h'

// TODO: try to make these global variables stack-local (somehow)

static struct lisp_lex lex = {0};

static int iofd = 0;
static char iobuf[IOBLOCK];

static void lisp_lex_ev(enum lisp_lex_ev ev) {
  if (LEX_SYMBOL_OUT(ev)) {
    lex.master.ev &= ~__LISP_EV_SYMBOL_IN;
    lex.master.ev |= __LISP_EV_SYMBOL_OUT;
    inc_hash_done(&lex.master.hash);
  }

  else if (LEX_PAREN_IN(ev)) {
    lex.master.ev |= __LISP_EV_PAREN_IN;

    // a( -> SYMBOL_OUT, PAREN_IN; the first takes precedence. `::cb_idx' will
    // trigger PAREN_IN again
    if (LEX_SYMBOL_IN(lex.master.ev)) {
      lisp_lex_ev(__LISP_EV_SYMBOL_OUT);
    }
  }

  else if (LEX_PAREN_OUT(ev)) {
    lex.master.ev |= __LISP_EV_PAREN_OUT;

    // a) -> SYMBOL_OUT, PAREN_OUT; the first takes precedence
    if (LEX_SYMBOL_IN(lex.master.ev)) {
      lisp_lex_ev(__LISP_EV_SYMBOL_OUT);
    }

    // (...)) -> EIMBALANCED
    if (lex.master.paren == 0) {
      lex.slave = err(EIMBALANCED);
      return;
    }
  }
}

static inline void lisp_lex_whitespace(void) {
  if (LEX_SYMBOL_IN(lex.master.ev)) {
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
  register int ret = 0;

  struct lisp_lex_ev_ret evret = {
    .master = prime,
    .slave  = 0,
  };

  enum lisp_stack_ev sev = stack->ev;

  // primed:     ...a -> push_func
  // not primed: ...a -> push_var
  if (LEX_SYMBOL_OUT(lev)) {
    DB_MSG("[ == ] lex(handle): symbol_out");

    lex.master.cb_idx  = IDX_MH(cb_idx);
    lex.master.ev     &= ~__LISP_EV_SYMBOL_OUT;

    stack->typ.lex.mem.hash = lex.master.hash;
    hash_done(&lex.master.hash);

    if (STACK_QUOT(sev)) {
      // same paren level as the function of the current literal: defer to the
      // stack; will be saved in the frame
      if (lex.master.paren == stack->typ.lex.paren) {
        stack->ev |= __STACK_PUSH_VAR;

        if (LEX_PAREN_OUT(lev)) {
          --lex.master.paren;
          lex.master.ev &= ~__LISP_EV_PAREN_OUT;
          DB_FMT(" -> paren--: %d", lex.master.paren);
          goto ev_paren_out_quot;
        }
        else if (LEX_PAREN_IN(lev)) {
          --lex.master.cb_idx;
          lex.master.ev &= ~__LISP_EV_PAREN_IN;
        }

        defer_as(__LEX_DEFER);
      }

      // bigger paren level than the function of the current literal: save in a
      // temporary memory on the SEXP tree (it already exists at this point).
      ret = lisp_sexp_sym(sexp_pp, stack->typ.lex.mem.hash);
      assert(ret == 0, OR_ERR());

      if (LEX_PAREN_OUT(lev)) {
        --lex.master.paren;
        lex.master.ev &= ~__LISP_EV_PAREN_OUT;
        DB_FMT(" -> paren--: %d", lex.master.paren);
        goto ev_paren_out_quot;
      }
      else if (LEX_PAREN_IN(lev)) {
        --lex.master.cb_idx;
        lex.master.ev &= ~__LISP_EV_PAREN_IN;
      }

      defer_as(__LEX_OK);
    }

    // not quoted: either push_func or push_var, depending if it's primed

    if (prime) {
      evret.master  = false;
      stack->ev    |= __STACK_PUSH_FUNC;
    }
    else {
      stack->ev    |= __STACK_PUSH_VAR;
    }

    if (LEX_PAREN_OUT(lev)) {
      prime = evret.master;
      goto ev_paren_out;
    }
    else if (LEX_PAREN_IN(lev)) {
      --lex.master.cb_idx;
      lex.master.ev &= ~__LISP_EV_PAREN_IN;
    }

    defer_as(__LEX_DEFER);
  }

  // ...( -> push
  else if (LEX_PAREN_IN(lev)) {
    DB_MSG("[ == ] lex(handle): paren_in");

    lex.master.cb_idx  = IDX_MH(cb_idx);
    lex.master.ev     &= ~__LISP_EV_PAREN_IN;

    ++lex.master.paren;
    DB_FMT(" -> paren++: %d", lex.master.paren);

    if (STACK_QUOT(sev)) {
      if (!stack->typ.lex.lit_expr) {
        stack->typ.lex.mem.sexp = lisp_sexp_get_head();
        stack->typ.lex.lit_expr = true;
      }

      lisp_sexp_node_add(sexp_pp);
      assert(ret == 0, OR_ERR());
      defer_as(__LEX_OK);
    }

    // only set the paren level if the expression is not quoted
    stack->typ.lex.paren = lex.master.paren;

    if (prime) {
      /** the core VSL interpreter doesn't allow function names to be the
          return value of a hash-changing function, e.g:

          (add-prefix sym) -> sym-prefix
          ((add-prefix function) arg) -> (function-prefix arg) -> ...

          the code above is not allowed because the stack schema is unable
          to communicate with its parents
      */
      // TODO: ^ this is not *necessarily* true; i can probably implement this
      // in the (far) future
      defer_as(err(ENOHASHCHANGING));
    }
    else {
      evret.master = true;
      defer_as(__LEX_OK);
    }
  }

  // ...) -> pop
  else if (LEX_PAREN_OUT(lev)) {
ev_paren_out:
    DB_MSG("[ == ] lex(handle): paren_out");

    lex.master.cb_idx  = IDX_MH(cb_idx);
    lex.master.ev     &= ~__LISP_EV_PAREN_OUT;

    --lex.master.paren;
    DB_FMT(" -> paren--: %d", lex.master.paren);

    if (STACK_QUOT(sev)) {
ev_paren_out_quot:
      if (stack->typ.lex.lit_expr) {
        lisp_sexp_end(sexp_pp);

        // the paren level now is the same as literal's: defer
        if (lex.master.paren == stack->typ.lex.paren) {
          // lisp_sexp_end_temp(sexp_pp);
          defer_as(__LEX_DEFER);
        }

        defer_for_as(evret.slave, __LEX_OK);
      }

      stack->ev |= __STACK_POP;
      defer_as(__LEX_DEFER);
    }

    stack->ev |= __STACK_POP;

    if (prime) {
      DB_MSG("[ == ] lex(ev): popped after prime: `()'");

      evret.master = false;

      // `()': `__STACK_PUSH_FUNC' takes precendence over `__STACK_POP'
      stack->typ.lex.mem.hash = (struct lisp_hash) {0};
      stack->ev |= __STACK_PUSH_FUNC;
    }

    defer_as(__LEX_DEFER);
  }

  done_for_with(evret, evret.slave = ret);
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
  assert(ret != __LEX_NO_INPUT, 0);
  assert(ret == __LEX_OK || ret == __LEX_DEFER, OR_ERR());

  /** NOTE: these are the only callbacks issued by `lisp_lex_bytstream'
            that `parse_bytstream_stack' can handle, the rest are handled
            by the stack frame
   */

  // push variable (top level)
  if (STACK_PUSHED_VAR(stack->ev)) {
    stack->ev &= ~__STACK_PUSHED_VAR;
    stack->typ.lex.mem.hash = (struct lisp_hash) {0};

    DB_MSG("TODO: implement top-level symbol resolution");

    goto lex;
  }

  // push function
  else if (STACK_PUSHED_FUNC(stack->ev)) {
    DB_MSG("[ == ] lex: stack push function");
    stack->ev &= ~__STACK_PUSHED_FUNC;
    lex.slave  = lisp_stack_lex_frame(stack).slave;
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

  register int ret  = 0;

  uint size = 0;

  struct lisp_lex_ev_ret evret = {0};

feed:
  size = lex.master.size;

  for (uint i = lex.master.cb_idx; i < size; i++) {
    lisp_lex_c(iobuf[i]);
    assert(lex.slave == 0, OR_ERR());

    evret = lisp_lex_handle_ev(lex.master.ev, stack, prime, i);

    prime = evret.master;
    ret   = evret.slave;

    if (ret < __LEX_OK) {
      defer();
    }

    assert(ret == __LEX_OK, OR_ERR());
  }

  if (size == 0) {
    if (LEX_SYMBOL_IN(lex.master.ev)) {
      lisp_lex_ev(__LISP_EV_SYMBOL_OUT);

      evret = lisp_lex_handle_ev(lex.master.ev, stack, prime,
                                 lex.master.cb_idx);
      prime = evret.master;
      ret   = evret.slave;

      if (ret < __LEX_OK) {
        defer();
      }

      assert(ret == __LEX_OK, OR_ERR());
    }

    assert(lex.master.paren == __LEX_OK, err(EIMBALANCED));

    // no error but also nothing else in the buffer: ask to stop
    defer_as(__LEX_NO_INPUT);
  }

  assert(parse_bytstream_feed() == 0, OR_ERR());
  goto feed;

  done_for(ret);
}

int parse_bytstream(int fd) {
  iofd = fd;

  struct lisp_stack stack;

  lex.master.paren       = 0;
  lex.master.ev          = 0;
  lex.slave              = 0;

  stack.ev               = 0;
  stack.typ.lex.lit_expr = false;
  stack.typ.lex.lazy     = NULL;
  stack.typ.lex.paren    = lex.master.paren;

  /** we need a stack-local `stack', so we wrap `parse_bytstream' with a
      `*_stack' function. this means this function (parse_bytstream) does not
      receive or send any callbacks from/to the stack */
  return parse_bytstream_stack(&stack);
}
