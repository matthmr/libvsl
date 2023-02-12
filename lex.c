#include <unistd.h>

#include "debug.h"
#include "stack.h" // also includes `sexp.h'
#include "lex.h"   // also includes `symtab.h'

#undef LOCK_POOL_THREAD

#include "pool.h"  // also includes `err.h'

#define SEXP_POOLP POOLP

// TODO: try to make these global variables stack-local (somehow)

static struct lisp_lex lex  = {0};

static int iofd             = 0;
static char iobuf[IOBLOCK];

// I fucking hate C
static int parse_bytstream_base(struct lisp_stack* stack);

static void lisp_lex_ev(enum lisp_lex_ev ev) {
  if (ev & __LISP_EV_SYMBOL_OUT) {
    lex.master.ev &= ~__LISP_EV_SYMBOL_IN;
    lex.master.ev |= __LISP_EV_SYMBOL_OUT;
    inc_hash_done(&lex.master.hash);
    DB_MSG("[ == ] lex(ev): symbol out");
  }

  else if (ev & __LISP_EV_PAREN_IN) {
    lex.master.ev |= __LISP_EV_PAREN_IN;
    DB_MSG("[ == ] lex(ev): paren in");
    if (lex.master.ev & __LISP_EV_SYMBOL_IN) {
      lisp_lex_ev(__LISP_EV_SYMBOL_OUT);
    }
    ++lex.master.paren;
  }

  else if (ev & __LISP_EV_PAREN_OUT) {
    lex.master.ev |= __LISP_EV_PAREN_OUT;
    DB_MSG("[ == ] lex(ev): paren out");

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

struct lisp_lex_ev_ret {
  bool               master;
  enum lisp_lex_stat slave;
};

static struct lisp_lex_ev_ret
lisp_lex_handle_ev(enum lisp_lex_ev lev, struct lisp_stack* stack,
                   bool prime, uint c_idx) {
  struct lisp_lex_ev_ret evret = {
    .master = prime,
    .slave  = 0,
  };

  enum lisp_stack_ev sev = stack->ev;

  // ...( -> push
  if (lev & __LISP_EV_PAREN_IN) {
    lex.master.cb_idx  = IDX_MH(c_idx);
    lex.master.ev     &= ~__LISP_EV_PAREN_IN;

    if (STACK_QUOT(sev)) {
      // lisp_sexp_node_add(&SEXP_POOLP);
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

  // primed:     ...a -> push_func
  // not primed: ...a -> push_var
  else if (lev & __LISP_EV_SYMBOL_OUT) {
    /** we prioritize `SYMBOL_OUT` over `PARENT_OUT`, but the latter can also
        trigger the former. If that happens, we *don't* send anything to back
        the frame *or* to the base, we just clear the `symbol_out' state and set
        the callback index to `c_idx', which would be `)'. When the frame calls
        us back, we immediately pop.
    */

    lex.master.cb_idx  = c_idx;
    lex.master.ev     &= ~__LISP_EV_SYMBOL_OUT;

    if (STACK_QUOT(sev)) {
      stack->typ.lex.hash = lex.master.hash;
      hash_done(&lex.master.hash);

      if ((lex.master.paren+1) == stack->typ.lex.paren) {
        // go back to the frame
        defer_for_as(evret.slave, __LEX_DEFER);
      }
      else {
        // TODO: what?
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

    stack->typ.lex.hash = lex.master.hash;
    hash_done(&lex.master.hash);
    defer_for_as(evret.slave, __LEX_DEFER);
  }

  // ...) -> pop
  else if (lev & __LISP_EV_PAREN_OUT) {
    lex.master.cb_idx  = IDX_MH(c_idx);
    lex.master.ev     &= ~__LISP_EV_PAREN_OUT;
    stack->ev         |= __STACK_POP;

    --lex.master.paren;

    if (STACK_QUOT(sev)) {
      if ((lex.master.paren+1) == stack->typ.lex.paren) {
        defer_for_as(evret.slave, __LEX_DEFER);
      }

      else if ((lex.master.paren+1) < stack->typ.lex.paren) {
        defer_for_as(evret.slave, __LEX_POP_LITR);
      }

      else {
        defer_for_as(evret.slave, 0);
      }
    }

    if (prime) {
      evret.master  = false;
      // stack->ev |= __STACK_EMPTY;
      // defer_for_as(evret.slave, 0); // wtf?
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

static int lisp_lex_bytstream(struct lisp_stack* stack) {
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

/** at any given time, there's at most *two* of this function in the call stack:
      - one as the stack base
      - another as a callback from the lexer asking for more bytes
 */
static int parse_bytstream_base(struct lisp_stack* stack) {
  int ret = 0;

  assert(parse_bytstream_feed() == 0, OR_ERR());

  // feed `iobuf' to the lexer, listen for callbacks
lex:
  ret = lisp_lex_bytstream(stack);

  // no error, no input: exit
  if (ret == __LEX_INPUT) {
    defer_as(0);
  }

  assert(ret == __LEX_OK || ret == __LEX_DEFER, OR_ERR());

  /** NOTE: these are the only callbacks issued by `lisp_lex_bytstream'
            that `parse_bytstream_base' can handle, the rest are handled
            by the stack frame
   */

  // push function
  if (STACK_PUSHED_FUNC(stack->ev)) {
    stack->ev &= ~__STACK_PUSHED_FUNC;

    // TODO: this should return `nil'; and the stack should also know about this
    if (stack->ev & __STACK_EMPTY) {
      stack->ev &= ~__STACK_EMPTY;
    }
    else {
      lex.slave = lisp_stack_lex_frame(stack).slave;
    }
  }

  // push variable (top level)
  else if (STACK_PUSHED_VAR(stack->ev)) {
    stack->ev &= ~__STACK_PUSHED_VAR;

    DB_MSG("TODO: implement top level symbol resolution");
  }

  assert(lex.slave == 0, OR_ERR());
  assert(lex.master.paren == 0, err(EIMBALANCED));

  stack->ev     = 0;
  lex.master.ev = 0;

  hash_done(&stack->typ.lex.hash);
  goto lex;

  done_for(ret);
}

////////////////////////////////////////////////////////////////////////////////

int parse_bytstream(int fd) {
  iofd = fd;

  struct lisp_stack stack;

  stack.typ.lex.cb = &lisp_lex_bytstream;

  lex.master.ev = 0;
  lex.slave     = 0;

  // we need a stack-local `stack', so we wrap `parse_bytstream'
  // with a `*_base' function. this means this function does not
  // receive or send any callbacks from/to the stack
  return parse_bytstream_base(&stack);
}
