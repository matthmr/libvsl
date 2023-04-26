#include <unistd.h>

#include "debug.h"
#include "lex.h"   // also includes `symtab.h'
#include "err.h"

// TODO: try to make these global variables stack-local (somehow)

static struct lisp_lex lex = {0};

static int   iofd = 0;
static char iobuf[IOBLOCK];

////////////////////////////////////////////////////////////////////////////////

/**
   Set @ev as an event for the lexer
 */
static inline enum lisp_lex_stat lisp_lex_ev(enum lisp_lex_ev ev) {
  register enum lisp_lex_stat ret = __LEX_OK;

  if (LEX_SYMBOL_OUT(ev)) {
    lex.ev &= ~__LISP_EV_SYMBOL_IN;
    lex.ev |= __LISP_EV_SYMBOL_OUT;
  }

  /////

  else if (LEX_PAREN_IN(ev)) {
    lex.ev |= __LISP_EV_PAREN_IN;

    // a( -> SYMBOL_OUT, PAREN_IN; the first takes precedence. `::cb_idx' will
    // trigger PAREN_IN again
    if (LEX_SYMBOL_IN(lex.ev)) {
      lisp_lex_ev(__LISP_EV_SYMBOL_OUT);
    }
  }

  /////

  else if (LEX_PAREN_OUT(ev)) {
    lex.ev |= __LISP_EV_PAREN_OUT;

    // a) -> SYMBOL_OUT, PAREN_OUT; the first takes precedence
    if (LEX_SYMBOL_IN(lex.ev)) {
      lisp_lex_ev(__LISP_EV_SYMBOL_OUT);
    }

    // (...)) -> EIMBALANCED
    if (lex.paren == 0) {
      defer(err(EIMBALANCED));
    }
  }

  done_for(ret);
}

/**
   Issue the 'whitespace' event
 */
static inline enum lisp_lex_stat lisp_lex_whitespace(void) {
  if (LEX_SYMBOL_IN(lex.ev)) {
    return lisp_lex_ev(__LISP_EV_SYMBOL_OUT);
  }

  return __LEX_OK;
}

/**
   Handle hash character @c as a character for a symbol
 */
static inline enum lisp_lex_stat lisp_lex_c(char c) {
  register enum lisp_lex_stat ret = __LEX_OK;

  lex.ev |= __LISP_EV_SYMBOL_IN;

  if (!__LISP_ALLOWED_IN_NAME(c)) {
    defer_as(err(ENOTALLOWED));
  }

  struct lisp_hash_ret hret = hash_c(lex.hash, c);
  lex.hash = hret.master;
  ret      = hret.slave;

  DB_FMT("[ lex ] symbol char (%c) [%d]", c, HASH_IDX(lex.hash));

  done_for(ret);
}

/**
   Handle character @c for the lexer, sending events if it needs to
 */
static inline enum lisp_lex_stat lisp_lex_handle_c(char c) {
  switch (c) {
  case __LISP_C_PAREN_OPEN:
    return lisp_lex_ev(__LISP_EV_PAREN_IN);
  case __LISP_C_PAREN_CLOSE:
    return lisp_lex_ev(__LISP_EV_PAREN_OUT);
  case __LISP_C_WHITESPACE:
    return lisp_lex_whitespace();
  default:
    return lisp_lex_c(c);
  }
}

/**
   Handle lex event, setting the current stack state for the stack frame
 */
static void
lisp_lex_handle_ev(enum lisp_lex_ev lev, struct lisp_stack* stack,
                   uint cb_idx) {
  lex.cb_idx = (cb_idx + 1);

  if (LEX_SYMBOL_OUT(lev)) {
    DB_MSG("[ lex ] handle: SYM");

    lex.ev    &= ~__LISP_EV_SYMBOL_OUT;
    stack->ev |= __STACK_PUSH_VAR;

    if (LEX_TANGLE(lev)) {
      --lex.cb_idx;
      lex.ev &= ~__LEX_TANGLE;
    }

    stack->hash = lex.hash;
    hash_done(&lex.hash);
  }

  /////

  else if (LEX_PAREN_IN(lev)) {
    DB_MSG("[ lex ] handle: `('");

    lex.ev    &= ~__LISP_EV_PAREN_IN;
    stack->ev |= __STACK_PUSH_FUN;

    ++lex.paren;
    DB_FMT("  -> lex: ++paren: %d", lex.paren);

    stack->paren = lex.paren;
  }

  /////

  else if (LEX_PAREN_OUT(lev)) {
    DB_MSG("[ lex ] handle: `)'");

    lex.ev    &= ~__LISP_EV_PAREN_OUT;
    stack->ev |= __STACK_POP;

    --lex.paren;
    DB_FMT("  -> lex: --paren: %d", lex.paren);

    stack->paren = lex.paren;
  }
}

/**
   Feed the lexer with @IOBLOCK bytes from fd @iofd, to buffer @iobuf
 */
static int lisp_lex_feed(void) {
  int ret  = 0;
  int size = read(iofd, iobuf, IOBLOCK);

  assert(size != -1, err(EREAD));

  DB_FMT("[ lex ] fed %d bytes", size);

  lex.size   = size;
  lex.cb_idx = 0;

  done_for(ret);
}

/**
   Base parser: yield one expression then defer to the stack
 */
static int lisp_lex_parser(struct lisp_stack* stack) {
  register int ret = lisp_lex_feed();

  assert(ret == 0, OR_ERR());

yield:
  ret = lisp_lex_yield(stack);

  // no error, no input: exit
  assert(ret != __LEX_NO_INPUT, 0);
  assert(ret == __LEX_OK, OR_ERR());

  // push variable (top level)
  if (STACK_PUSHED_VAR(stack->ev)) {
    DB_MSG("TODO: implement top-level symbol resolution\n"
           " -----------------------");

    stack->ev &= ~__STACK_PUSH_VAR;
    ret        = __STACK_OK;
  }

  // push function
  else if (STACK_PUSHED_FUN(stack->ev)) {
    DB_MSG("[ lex ] top-level yield function");

    ret = lisp_stack_lex_frame(stack).slave;

    DB_MSG("[ lex ] back to top-level\n"
           " -----------------------");
  }

  // give the parent error precedence over possible `EIMBALANCED'
  assert(ret == __STACK_OK || ret == __STACK_DONE, OR_ERR());
  assert(lex.paren == 0, err(EIMBALANCED));

  ret       = 0;
  stack->ev = 0;
  lex.ev    = 0;

  goto yield;

  done_for(ret);
}

////////////////////////////////////////////////////////////////////////////////

/**
   Yield expressions from the lexer
 */
int lisp_lex_yield(struct lisp_stack* stack) {
  register int ret = 0;
  register enum lisp_lex_stat lret = __LEX_OK;

  uint size = 0;

  enum lisp_lex_ev ev = lex.ev;

feed:
  size = lex.size;

  for (uint i = lex.cb_idx; i < size; ++i) {
    lret = lisp_lex_handle_c(iobuf[i]);
    assert(lret == __LEX_OK, OR_ERR());

    ev = lex.ev;

    // avoid calling this function for *every* character
    if (!LEX_SYMBOL_IN(ev) && ev) {
      lisp_lex_handle_ev(ev, stack, i);
      defer_as(__LEX_OK);
    }
  }

  // fed 0 bytes: set any current symbol as finished, then defer as `no-input'
  if (size == 0) {
    if (LEX_SYMBOL_IN(lex.ev)) {
      lret = lisp_lex_ev(__LISP_EV_SYMBOL_OUT);
      assert(lret == __LEX_OK, OR_ERR());

      ev   = lex.ev;
      lisp_lex_handle_ev(ev, stack, (lex.cb_idx - 1));
    }

    assert(lex.paren == 0, err(EIMBALANCED));

    // no error but also nothing else in the buffer: ask to stop
    defer_as(__LEX_NO_INPUT);
  }

  ret = lisp_lex_feed();
  assert(ret == 0, OR_ERR());

  goto feed;

  done_for(ret);
}

/**
   Wrap the base lex parser (lisp_lex_parser) and define the stack
 */
int lisp_parser(int fd) {
  iofd = fd;

  struct lisp_stack stack = {0};

  lex.paren   = 0;
  lex.ev      = 0;

  lex.hash.w  = 1;

  stack.ev    = 0;
  stack.lazy  = NULL;
  stack.paren = lex.paren;

  stack.lit_paren = 0;

  return lisp_lex_parser(&stack);
}
