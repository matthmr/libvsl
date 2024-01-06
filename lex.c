#include <unistd.h>

#include "debug.h"

#include "lex.h"

//// ERRORS

ECODE(EIMBALANCED, ENOTALLOWED, ETOOBIG, EREAD);

EMSG {
  [EIMBALANCED] = ERR_STRING("libvsl: lexer", "imbalanced parens"),
  [ENOTALLOWED] = ERR_STRING("libvsl: lexer",
                             "character is not allowed in symbol"),
  [ETOOBIG]     = ERR_STRING("libvsl: lexer",
                             "symbol is too big"),
  [EREAD]       = ERR_STRING("libvsl", "error while reading file"),
};

static char _iobuf[IOBLOCK] = {0};
static char _symbuf[LISP_SYM_MAX_LEN] = {0};

struct lisp_lex lex = {
  .symbuf = {
    .str = _symbuf,
    .idx = 0,
  },
  .iobuf  = {
    .str = _iobuf,
    .idx = 0,
  },
  .iobuf_size = 0,

  .paren  = 0,
  .ev     = 0,
};

////////////////////////////////////////////////////////////////////////////////

/** Set @ev as an event for the lexer */
static inline void lisp_lex_ev(enum lisp_lex_ev ev) {
  if (LEX_SYMBOL_OUT(ev)) {
    lex.ev &= ~__LEX_EV_SYMBOL_IN;
    lex.ev |= ev;
  }

  /////

  else if (LEX_PAREN_IN(ev)) {
    lex.ev |= ev;

    // a( -> SYMBOL_OUT, PAREN_IN; the first takes precedence
    if (LEX_SYMBOL_IN(lex.ev)) {
      lisp_lex_ev(__LEX_EV_SYMBOL_OUT);
    }
  }

  /////

  else if (LEX_PAREN_OUT(ev)) {
    lex.ev |= ev;

    // a) -> SYMBOL_OUT, PAREN_OUT; the first takes precedence
    if (LEX_SYMBOL_IN(lex.ev)) {
      lisp_lex_ev(__LEX_EV_SYMBOL_OUT);
    }
  }
}

/** Handle hash character @c as a character for a symbol */
static inline enum lisp_lex_stat lisp_lex_c(char c) {
  register enum lisp_lex_stat ret = __LEX_OK;

  lex.ev |= __LEX_EV_SYMBOL_IN;

  assert(__LISP_ALLOWED_IN_NAME(c), err(ENOTALLOWED));
  assert(lex.symbuf.idx < LISP_SYM_MAX_LEN, err(ETOOBIG));

  lex.symbuf.str[lex.symbuf.idx] = c;
  lex.symbuf.idx++;

  done_for(ret);
}

/** Handle character @c for the lexer, setting up events */
static inline void lisp_lex_handle_c(char c) {
  switch (c) {

  // `(', `)'
  case __LEX_C_PAREN_OPEN:
    lisp_lex_ev(__LEX_EV_PAREN_IN);
    break;
  case __LEX_C_PAREN_CLOSE:
    lisp_lex_ev(__LEX_EV_PAREN_OUT);
    break;

  // ' '...
  case __LEX_C_WHITESPACE:
    if (LEX_SYMBOL_IN(lex.ev)) {
      lisp_lex_ev(__LEX_EV_SYMBOL_OUT);
    }

    break;

  // everything else: part of a symbol
  default:
    lisp_lex_c(c);
    break;
  }
}

/** Handle lex event, setting up the return type and the lex state */
static enum lisp_lex_stat
lisp_lex_handle_ev(enum lisp_lex_ev lev, uint cb_idx) {
  register enum lisp_lex_stat ret = __LEX_OK;

  lex.iobuf.idx = (cb_idx + 1);

  if (LEX_SYMBOL_OUT(lev)) {
    DB_BYT("[ lex ] emit: sym (");
    DB_NBYT(lex.symbuf.str, lex.symbuf.idx);
    DB_MSG(")");

    // we preemptively shift the cursor up, but if we're tangled, shifting it
    // back down will make it trigger the next time we try to yield
    if (LEX_TANGLE(lev)) {
      --lex.iobuf.idx;
      lex.ev &= ~__LEX_TANGLE;
    }
  }

  /////

  else if (LEX_PAREN_IN(lev)) {
    DB_MSG("[ lex ] emit: start expr");

    lex.paren++;

    DB_FMT("  -> lex: paren++: %d", lex.paren);
  }

  /////

  else if (LEX_PAREN_OUT(lev)) {
    // (...)) -> EIMBALANCED
    assert(lex.paren > 0, err(EIMBALANCED));

    DB_MSG("[ lex ] emit: end expr");

    --lex.paren;

    DB_FMT("  -> lex: paren--: %d", lex.paren);
  }

  done_for(ret);
}

////////////////////////////////////////////////////////////////////////////////

int lisp_lex_feed(void) {
  int ret  = 0;
  int size = read(IOFD, lex.iobuf.str, IOBLOCK);

  assert(size != -1, err(EREAD));

  DB_FMT("[ lex ] fed %d bytes", size);

  lex.iobuf_size = size;
  lex.iobuf.idx  = 0;

  done_for(ret);
}

int lisp_lex_yield(void) {
  register int   ret = __LEX_OK;
  register uint size = 0;

  enum lisp_lex_ev ev = lex.ev;

feed:
  size = lex.iobuf_size;

  for (uint ev_i = lex.iobuf.idx; ev_i < size; ev_i++) {
    lisp_lex_handle_c(lex.iobuf.str[ev_i]);

    ev = lex.ev;

    // avoid calling this function for *every* character
    if (ev && !LEX_SYMBOL_IN(ev)) {
      return lisp_lex_handle_ev(ev, ev_i);
    }
  }

  // fed 0 bytes: set any current symbol as finished, then defer as `no-input'
  if (size == 0) {
    if (LEX_SYMBOL_IN(lex.ev)) {
      lisp_lex_ev(__LEX_EV_SYMBOL_OUT);

      return lisp_lex_handle_ev(lex.ev, lex.iobuf.idx);
    }

    // top-level EOF with empty buffer: ask to stop
    if (!lex.paren) {
      defer_as(__LEX_NO_INPUT);
    }
  }

  // fell through the lexer: feed again, go back
  ret = lisp_lex_feed();
  assert(ret == 0, OR_ERR());

  goto feed;

  done_for(ret);
}
