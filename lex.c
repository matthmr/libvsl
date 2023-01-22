#include <unistd.h>

#include "debug.h"
#include "stack.h" // includes `sexp.h'
#include "lex.h"   // includes `symtab.h'
#include "err.h"

#undef LOCK_POOL_THREAD

// `stack.h' locks the pool definition,
// so we have to include `pool.h' again
#include "pool.h"

#define SEXP_POOLP POOLP

// TODO: try to make these global variables stack-local (somehow)

static struct lisp_lex lex  = {0};

static int iofd             = 0;
static char iobuf[IOBLOCK];

// I fucking hate C
static int parse_bytstream_base(struct lisp_stack* stack);

static inline struct lisp_lex
lisp_lex_ev(struct lisp_lex lex, enum lisp_lex_ev ev) {
  if (ev & __LISP_SYMBOL_OUT) {
    lex.master.ev &= ~__LISP_SYMBOL_IN;
    lex.master.ev |= __LISP_SYMBOL_OUT;
    DB_MSG("<- EV: symbol out");
  }

  if (ev & __LISP_PAREN_IN) {
    lex.master.ev |= __LISP_PAREN_IN;
    if (lex.master.ev & __LISP_SYMBOL_IN) {
      lex = lisp_lex_ev(lex, __LISP_SYMBOL_OUT);
    }
    ++lex.master.paren;
  }

  else if (ev & __LISP_PAREN_OUT) {
    lex.master.ev |= __LISP_PAREN_OUT;
    if (lex.master.ev & __LISP_SYMBOL_IN) {
      lex = lisp_lex_ev(lex, __LISP_SYMBOL_OUT);
    }

    DB_MSG("<- EV: paren out");

    if (lex.master.paren) {
      --lex.master.paren;
    }
    else {
      defer_for_as(lex.slave, err(EIMBALANCED));
    }

    // if (!lex.master.paren) {
    //   lex.slave = lisp_do_sexp(&POOL);
    // }
  }

  done_for(lex);
}

static inline struct lisp_lex
lisp_lex_whitespace(struct lisp_lex lex) {
  if (lex.master.ev & __LISP_SYMBOL_IN) {
    inc_hash_done();
    lex = lisp_lex_ev(lex, __LISP_SYMBOL_OUT);

    assert_for(lex.slave == 0, OR_ERR(), lex.slave);
  }

  done_for(lex);
}

static inline struct lisp_lex
lisp_lex_csym(struct lisp_lex lex, char c) {
  lex.master.ev |= __LISP_SYMBOL_IN;

  if (!__LISP_ALLOWED_IN_NAME(c)) {
    defer_for_as(lex.slave, 1);
  }

  else {
    struct lisp_hash_ret ret = inc_hash(lex.master.hash, c);
    lex.master.hash = ret.master;
    lex.slave       = ret.slave;
  }

  DB_FMT("vslisp: character (%c) (0x%x)", c, lex.master.hash.sum);

  done_for(lex);
}

// TODO: this function should be able to switch and push to the sexp stack,
// instead of directly to the stack
// TODO: some `prime' checks are false positives; they should only trigger
// if the outer scope says it's expecting normal functions
static int lisp_lex_bytstream(struct lisp_stack* stack) {
  int  ret  = 0;
  uint size = 0;

  uint litparen = lex.master.paren;

  static bool prime = false;

feed:
  size = lex.master.size;

  for (uint i = lex.master.cb_i; i < size; i++) {
    char c = iobuf[i];

    // main character switch
    switch (c) {
    case __LISP_PAREN_OPEN:
      DB_MSG("-> EV: paren open");
      lex = lisp_lex_ev(lex, __LISP_PAREN_IN);
      break;
    case __LISP_PAREN_CLOSE:
      DB_MSG("<- EV: paren close");
      lex = lisp_lex_ev(lex, __LISP_PAREN_OUT);
      break;
    case __LISP_WHITESPACE:
      lex = lisp_lex_whitespace(lex);
      break;
    default:
      lex = lisp_lex_csym(lex, c);
      break;
    }

    assert(lex.slave == 0, OR_ERR());

    enum lisp_lex_ev ev = lex.master.ev;

    // lex token callback
    if (ev & __LISP_PAREN_IN) {            /* ...( -> push     */
      lex.master.cb_i  = (i+1);
      lex.master.ev   &= ~__LISP_PAREN_IN;

      if (STACK_QUOT(ev)) {
        lisp_sexp_node_add(&SEXP_POOLP);
        continue;
      }

      /** the core VSL interpreter doesn't allow function names to be the
          return value of a hash-changing function, e.g:

             (add-prefix sym) -> sym-prefix
             ((add-prefix function) arg) -> (function-prefix arg) -> ...

         the code above is not allowed because the stack schema is unable
         to communicate with its parents
      */
      if (prime) {
        defer_as(err(ENOHASHCHANGING));
      }
      else {
        prime      = true;
        stack->ev |= __STACK_PUSH_FUNC;
        continue;
      }
    }

    else if (ev & __LISP_SYMBOL_OUT) {       /* primed:     ...a -> push
                                                not primed: ...a -> push_var */
      lex.master.cb_i  = (i+1);
      lex.master.ev   &= ~__LISP_SYMBOL_OUT;

      if (STACK_QUOT(ev)) {
        lisp_sexp_sym(&SEXP_POOLP, lex.master.hash);
        hash_done(&lex.master.hash);

        if (litparen == lex.master.paren) {
          stack->ev &= ~__STACK_LIT;
          // lisp_sexp_save(SEXP_POOLP, stack);
          defer_as(0);
        }
        continue;
      }

      if (prime) {
        prime      = false;
        stack->ev |= __STACK_PUSH_FUNC;

        // clean in before defering
        if (ev & __LISP_PAREN_OUT) {
          lex.master.ev &= ~__LISP_PAREN_OUT;
        }
      }
      else {
        stack->ev |= __STACK_PUSH_VAR;

        if (ev & __LISP_PAREN_OUT) {
          lex.master.ev &= ~__LISP_PAREN_OUT;
          goto close_pop;
        }
      }

      stack->typ.lex.hash = lex.master.hash;
      hash_done(&lex.master.hash);
      defer_as(0);
    }

    else if (ev & __LISP_PAREN_OUT) {      /* ...) -> pop      */
      lex.master.cb_i  = (i+1);

      lex.master.ev   &= ~__LISP_PAREN_OUT;

      if (STACK_QUOT(ev)) {
        if (litparen == lex.master.paren) {
          stack->ev &= ~__STACK_LIT;
          // lisp_sexp_save(SEXP_POOLP, stack);
          defer_as(0);
        }
        else {
          lisp_sexp_end(SEXP_POOLP);
        }

        continue;
      }

      if (prime) {
        prime      = false;
        stack->ev |= __STACK_EMPTY;
        continue;
      }

close_pop:
      stack->ev   |= __STACK_POP;

      defer_as(0);
    }
  }

  // didn't finish the expression: call the base function asking
  // for more bytes
  if (lex.master.paren || (lex.master.ev & __LISP_SYMBOL_IN)) {
    lex.master.ev |= __LISP_FEED;
    assert(parse_bytstream_base(stack) == 0, OR_ERR());
    goto feed;
  }

  done_for(ret);
}

static int parse_bytstream_base(struct lisp_stack* stack) {
  int ret = 0;

feed:
  // feed `iobuf' with data from file descriptor `fd'
  lex.master.size = read(iofd, iobuf, IOBLOCK);
  lex.master.cb_i = 0;

  assert(lex.master.size != -1, err(EREAD));

  /** lexer exited with `feed' callback:
        immediately exit successfully, let lexer
        continue */
  if (lex.master.ev & __LISP_FEED) {
    lex.master.ev   &= ~__LISP_FEED;
    defer_as(0);
  }

  // feed `iobuf' to the lexer, listen for callbacks
  assert(lisp_lex_bytstream(stack) == 0, OR_ERR());

  /** NOTE: these are the only callbacks issued by `lisp_lex_bytstream'
            that `parse_bytstream_base' can handle, the rest are handled
            by the stack frame
   */
  if (stack->ev & __STACK_PUSH_FUNC) {     /* push function */
    stack->ev &= ~__STACK_PUSHED_FUNC;
    lex.slave  = lisp_stack_lex_frame(stack);
  }
  else if (stack->ev & __STACK_PUSH_VAR) { /* push variable */
    DB_MSG("TODO: implement top level symbol resolution");
  }

  assert(lex.slave == 0, OR_ERR());

  //   the very special case of `()' as the only input passes
  //   all checks and exists without error, but also doesn't
  //   execute anything. It sets `__STACK_EMPTY' which would
  //   be used by the `frame' functions, but no such frame
  //   exists yet
  assert(lex.master.paren == 0, err(EIMBALANCED));

  if (lex.master.size) {
    stack->ev     = 0;
    lex.master.ev = 0;

    hash_done(&stack->typ.lex.hash);
    goto feed;
  }

  done_for(ret);
}

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
