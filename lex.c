#include <unistd.h>

#include "debug.h"
#include "stack.h"

// TODO: try to make these global variables stack-local (somehow)

static struct lisp_lex lex  = {0};

static int iofd             = 0;
static char iobuf[IOBLOCK];

static inline struct lisp_lex
lisp_ev(struct lisp_lex lex, enum lisp_lex_ev ev) {
  if (ev & __LISP_SYMBOL_OUT) {
    lex.master.ev &= ~__LISP_SYMBOL_IN;
    lex.master.ev |= __LISP_SYMBOL_OUT;
    DB_MSG("<- EV: symbol out");
  }

  if (ev & __LISP_PAREN_IN) {
    lex.master.ev |= __LISP_PAREN_IN;
    if (lex.master.ev & __LISP_SYMBOL_IN) {
      lex = lisp_ev(lex, __LISP_SYMBOL_OUT);
    }
    ++lex.master.paren;
  }

  else if (ev & __LISP_PAREN_OUT) {
    lex.master.ev |= __LISP_PAREN_OUT;
    if (lex.master.ev & __LISP_SYMBOL_IN) {
      lex = lisp_ev(lex, __LISP_SYMBOL_OUT);
    }

    DB_MSG("<- EV: paren out");

    if (lex.master.paren) {
      --lex.master.paren;
    }
    else {
      defer_for_as(lex.slave, 1);
    }

    // if (!lex.master.paren) {
    //   lex.slave = lisp_do_sexp(&POOL);
    // }
  }

  done_for(lex);
}

static inline struct lisp_lex
lisp_whitespace(struct lisp_lex lex) {
  if (lex.master.ev & __LISP_SYMBOL_IN) {
    inc_hash_done();
    lex = lisp_ev(lex, __LISP_SYMBOL_OUT);

    assert_for(lex.slave != 0, 1, lex.slave);
  }

  done_for(lex);
}

static inline struct lisp_lex
lisp_csym(struct lisp_lex lex, char c) {
  lex.master.ev |= __LISP_SYMBOL_IN;

  if (!__LISP_ALLOWED_IN_NAME(c)) {
    defer_for_as(lex.slave, 1);
  }
  else {
    struct lisp_symtab_ret ret = inc_hash(lex.master.chash, c);
    lex.master.chash = ret.master;
    lex.slave        = ret.slave;
  }

  DB_FMT("vslisp: character (%c) (0x%x)", c, chash.sum);

  done_for(lex);
}

static int lisp_lex_bytstream(struct lisp_stack* stack) {
  int ret   = 0;
  uint size = stack->typ.lex.size;

  for (uint i = stack->typ.lex.cb_i; i < size; i++) {
    char c = iobuf[i];

    switch (c) {
    case __LISP_PAREN_OPEN:
      DB_MSG("-> EV: paren open");
      lex = lisp_ev(lex, __LISP_PAREN_IN);
      break;
    case __LISP_PAREN_CLOSE:
      DB_MSG("<- EV: paren close");
      lex = lisp_ev(lex, __LISP_PAREN_OUT);
      break;
    case __LISP_WHITESPACE:
      lex = lisp_whitespace(lex);
      break;
    default:
      lex = lisp_csym(lex, c);
      break;
    }

    assert(lex.slave == 0, 1);

    // stack callback
    if (lex.master.ev & (__LISP_PAREN_IN  |    /* ...( -> push     */
                         __LISP_PAREN_OUT |    /* ...) -> pop      */
                         __LISP_SYMBOL_OUT)) { /* ...a -> push_var */
      stack->typ.lex.cb_i = (i+1);
      defer_func();
    }
  }

  // didn't finish the expression: send feed callback to the frame,
  // the frame then calls `parse_bytstream' with the same callback,
  // which will call `lisp_lex_bytstream' back, continuing from
  // where it left off
  if (lex.master.paren || (lex.master.ev & __LISP_SYMBOL_IN)) {
    stack->ev |= __STACK_FEED;
  }

  DB_FMT("vslisp: paren = %d", paren);

  done_for(ret);
}

static int parse_bytstream_base(struct lisp_stack* stack) {
  int ret = 0;

  // feed `iobuf' with data from file descriptor `fd'
  stack->typ.lex.size = read(iofd, iobuf, IOBLOCK);
  stack->typ.lex.cb_i = 0;

  // called with `feed' callback:
  //   immediately exit successfully, let `frame' call
  //   `bytstream' for us
  if (stack->ev & __STACK_FEED) {
    stack->ev &= ~__STACK_FEED;

    // there are no bytes left: ensure that there's nothing
    // dangling
    if (!lex.master.size) {
      assert(
        (lex.master.paren == 0) || (!(lex.master.ev & __LISP_SYMBOL_IN)), 1);
    }

    defer_as(0);
  }

  // feed `iobuf' to the lexer, listen for callbacks
  assert(lisp_lex_bytstream(stack), 1);

  /** NOTE: these are the only callbacks issued by `lisp_lex_bytstream'
            that `parse_bytstream_base' can handle, the rest are handled
            by the stack frame
   */
  if (stack->ev & __STACK_PUSH_FUNC) { /* push */
    lex.master.ev &= ~__LISP_PAREN_IN;
    lex.slave      = lisp_stack_lex_frame(stack);
    assert(lex.slave == 0, 1);
  }
  else if (stack->ev & __STACK_PUSH_VAR) {
    lex.master.ev &= ~__LISP_SYMBOL_IN;
    lex.slave = lisp_stack_lex_frame(stack);
    assert(lex.slave == 0, 1);
    DB_MSG("TODO: implement top level symbol resolution");
  }

  assert(lex.master.paren == 0, 1)

  done_for(ret);
}

int parse_bytstream(int fd) {
  iofd = fd;

  struct lisp_stack stack;

  stack.typ.lex.cb_i     = 0;

  stack.typ.lex.cb.base  = &parse_bytstream_base;
  stack.typ.lex.cb.lexer = &lisp_lex_bytstream;

  lex.master.ev          = 0;
  lex.slave              = 0;

  // we need a stack-local `stack', so we wrap `parse_bytstream'
  // with a `*_base' function. this means this function does not
  // receive or send any callbacks from/to the stack
  return parse_bytstream_base(&stack);
}
