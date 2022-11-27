// vslisp: a very simple lisp implementation;
//         not bootstrapped

#include <unistd.h>
#include <stdlib.h>

#ifdef DEBUG
#  include <stdio.h>
#endif

#include "vslisp.h"

static uint paren = 0;
static char iobuf[IOBLOCK] = {0};

static struct hash chash = {0};
struct lisp_cps lcps = {0};

static inline void lisp_sym(struct hash chash) {}
static inline void lisp_sexp(void) {}

static inline long do_chash(int i, char c) {
  long ret = 0;

  ret   = ((c + i) % 0x100);
  ret <<= ((i*8)   % 0xffffffffffffffff);

  return ret;
}

static inline struct lisp_cps lisp_stat(struct lisp_cps pstat,
                                        enum lisp_pstat sstat) {
  switch (sstat) {
  case __LISP_PAREN_IN:
    ++paren;
    break;
  }

  pstat.master.stat |= sstat;

  return pstat;
}
static inline struct lisp_cps lisp_ev(struct lisp_cps pstat,
                                      enum lisp_pev sev) {
  switch (pstat.master.stat) {
  case __LISP_PAREN_IN:
    if (paren) {
      pstat.master.stat &= ~__LISP_PAREN_IN;
      --paren;
    }
    else {
      pstat.slave = 1;
      goto done;
    }
    break;
  case __LISP_SYMBOL_IN:
    pstat.master.stat &= ~__LISP_SYMBOL_IN;
    break;
  }

  pstat.master.ev |= sev;

 done:
  return pstat;
}
static inline struct lisp_cps lisp_whitespace(struct lisp_cps pstat) {
  if (pstat.master.stat & __LISP_SYMBOL_IN) {
    pstat = lisp_ev(pstat, __LISP_SYMBOL_OUT);
  }

  return pstat;
}
static inline struct lisp_cps lisp_csym(struct lisp_cps pstat, char c) {
  pstat.master.stat |= __LISP_SYMBOL_IN;

  ++chash.len;
  chash.hash += do_chash(chash.internal.i, c);
  ++chash.internal.i;

#ifdef DEBUG
  printf("[chash %d,%c] 0x%x", chash.internals.i, c, chash.hash);
#endif

  return pstat;
}

static int parse_ioblock(char* buf, uint size) {
  int ret = 0;

  for (uint i = 0; i < size; i++) {
    char c = buf[i];

    switch (c) {
    case __LISP_PAREN_OPEN:
      lcps = lisp_stat(lcps, __LISP_PAREN_IN);
      break;
    case __LISP_PAREN_CLOSE:
      lcps = lisp_ev(lcps, __LISP_PAREN_OUT);
      break;
    case __LISP_WHITESPACE:
      lcps = lisp_whitespace(lcps);
      break;
    default:
      lcps = lisp_csym(lcps, c);
      break;
    }

    if (lcps.slave) {
      ret = 1;
      goto done;
    }
    else {
      enum lisp_pev ev = lcps.master.ev;
      if (ev & __LISP_SYMBOL_OUT) {
        lisp_sym(chash);
      }
      if (ev & __LISP_PAREN_OUT) {
        lisp_sexp();
      }
    }
  }

 done:
  return ret;
}

static int parse_bytstream(int fd) {
  int ret = 0;

  uint r;

  do {
    r = read(fd, iobuf, IOBLOCK);
    int s = parse_ioblock(iobuf, r);
    if (s) {
      ret = s;
      goto done;
    }
  } while (r == IOBLOCK);

 done:
  return ret;
}

int main(void) {
  int ret = parse_bytstream(STDIN_FILENO);

  if (ret || paren) {
    ret = (ret || paren);
    fputs("[ !! ] vslisp: error while parsing file", stderr);
  }

  return ret;
}

