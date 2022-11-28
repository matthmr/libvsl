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
static struct lisp_cps lcps = {0};
static struct lisp_sexp* head = NULL,
                       * root = NULL,
                       * deep = NULL;

struct MEMPOOL(lisp_sexp, SEXPPOOL);
//struct MEMPOOL(lisp_sym_hash, HASHPOOL);

static struct MEMPOOL_TMPL(lisp_sexp)  sexpmp = {0};
static struct MEMPOOL_TMPL(lisp_sexp)* sexpmpp = NULL;
//static struct MEMPOOL_TMPL(lisp_sym_hash) hashmp = {0};

static inline struct pool_ret_t pool_add_node(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
  // TODO: make this an `int' funcion because of
  //       ENOMEM in malloc
  struct pool_ret_t ret = {
    .mem = NULL,
    .new = 0,
  };

  if (mpp->used == mpp->total) {
    if (!mpp->next) {
      mpp->next = malloc(mpp->total * sizeof(struct MEMPOOL_TMPL(lisp_sexp)));

      mpp->next->prev  = mpp;
      mpp->next->next  = NULL;
      mpp->next->total = mpp->total;
      mpp->next->used  = 0;
    }

    mpp = mpp->next;
    ret.new = true;
  }

  ret.mem = (mpp->mem + mpp->used);
  ++mpp->used;

  return ret;
}
static inline void lisp_sexp_node(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
  struct pool_ret_t pr    = pool_add_node(mpp);
  struct lisp_sexp* nhead = pr.mem;

  nhead->root = (struct off_t) {
    .new = pr.new,
    .am  = pr.new? 0: (nhead - mpp->mem),
  };

  if (!head) {
    head = root;
    return;
  }

  if (head->left.sexp == neither) {
    head->left.new  = !pr.new;
    head->left.am   = (pr.new)? 0: 1;
    head->left.sexp = true;
  }
  else if (head->right.sexp == neither) {
    head->right.new  = !pr.new;
    //head->right.am  = (pr.new)? 0: 1;
    head->right.sexp = true;
  }

  head->root = nhead->root;
}
static inline void lisp_sexp_sym(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
  if (!head) {
#ifdef DEBUG
    fputs("DO: symbol\n", stderr);
#endif
    goto done;
  }

  if (head->left.sexp == neither) {
    head->left.sexp     = false;
    head->left.sym.hash = chash;
  }
  else if (head->right.sexp == neither) {
    head->right.sexp     = false;
    head->right.sym.hash = chash;
  }
  else {
    struct lisp_sexp* phead;
    if (!head->root.new) {
      phead                 = (mpp->mem - head->root.am);
      phead->right.sexp     = false;
      phead->right.sym.hash = chash;
    }
    else { }
    head = phead;
  }

done:
  chash.internal.i = 0;
  chash.len        = 0;
  chash.hash       = 0L;
}
static inline void lisp_sexp_end(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
#ifdef DEBUG
  fputs("DO: sexp\n", stderr);
#endif
  // TODO: get the parent address, point to it

  struct MEMPOOL_TMPL(lisp_sexp)* p = mpp;

  for (; p; p = p->prev) {
    p->used = 0;
  }

  head = root;
  mpp  = p;
}
static inline long do_chash(int i, char c) {
  long ret = 0;

  ret   = ((c + i) % 0x100);
  ret <<= ((i*8)   % 0xffffffffffffffff);

  return ret;
}
static inline struct lisp_cps lisp_ev(struct lisp_cps pstat,
                                      enum lisp_pev sev) {
  if (pstat.master.stat & __LISP_SYMBOL_IN) {
    pstat.master.stat &= ~__LISP_SYMBOL_IN;
    sev |= __LISP_SYMBOL_OUT;
  }
  if (sev & __LISP_PAREN_OUT) {
    if (paren) {
      pstat.master.stat &= ~__LISP_PAREN_IN;
      --paren;
    }
    else {
      pstat.slave = 1;
      goto done;
    }
  }

  pstat.master.ev |= sev;

 done:
  return pstat;
}
static inline struct lisp_cps lisp_stat(struct lisp_cps pstat,
                                        enum lisp_pstat sstat) {
  if (sstat & __LISP_PAREN_IN) {
    lisp_sexp_node(sexpmpp);
    ++paren;
  }
  if (pstat.master.stat & __LISP_SYMBOL_IN) {
    pstat = lisp_ev(pstat, __LISP_SYMBOL_OUT);
  }

  pstat.master.stat |= sstat;

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

  if (!__LISP_ALLOWED_IN_NAME(c)) {
    pstat.slave = 1;
    goto done;
  }

  ++chash.len;
  chash.hash += do_chash(chash.internal.i, c);
  ++chash.internal.i;

#ifdef DEBUG
      fprintf(stderr, "vslisp: character (%c) (0x%lx)\n", c, chash.hash);
#endif

 done:
  return pstat;
}
static inline struct lisp_cps ev_listen(struct lisp_cps pstat) {
  enum lisp_pev ev = pstat.master.ev;
  if (ev & __LISP_SYMBOL_OUT) {
#ifdef DEBUG
    fputs("EVENT: symbol out\n", stderr);
#endif
    pstat.master.ev &= ~__LISP_SYMBOL_OUT;
    lisp_sexp_sym(sexpmpp);
  }
  if (ev & __LISP_PAREN_OUT) {
#ifdef DEBUG
    fputs("EVENT: paren out\n", stderr);
#endif
    pstat.master.ev &= ~__LISP_PAREN_OUT;
    lisp_sexp_end(sexpmpp);
  }
  return pstat;
}
static int parse_ioblock(char* buf, uint size) {
  int ret = 0;

  for (uint i = 0; i < size; i++) {
    char c = buf[i];

    switch (c) {
    case __LISP_PAREN_OPEN:
#ifdef DEBUG
      fputs("vslisp: paren_open\n", stderr);
#endif
      lcps = lisp_stat(lcps, __LISP_PAREN_IN);
      break;
    case __LISP_PAREN_CLOSE:
#ifdef DEBUG
      fputs("vslisp: paren_close\n", stderr);
#endif
      lcps = lisp_ev(lcps, __LISP_PAREN_OUT);
      break;
    case __LISP_WHITESPACE:
#ifdef DEBUG
      fputs("vslisp: whitespace\n", stderr);
#endif
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
      lcps = ev_listen(lcps);
    }
  }

#ifdef DEBUG
  printf("vslisp: [ret = %d, paren = %d]\n", ret, paren);
#endif

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
  if (paren) {
    ret = 1;
  }
  else if (lcps.master.stat) {
    lcps = lisp_whitespace(lcps);
    (void) ev_listen(lcps);
  }

  return ret;
}

int main(void) {
  sexpmp.total     = SEXPPOOL;
  sexpmp.used      = 1;
  sexpmp.prev      = NULL;
  sexpmp.next      = NULL;

  sexpmpp          = &sexpmp;

  head             = NULL;
  root             = sexpmpp->mem;
  sexpmpp->used    = 0;

  root->root.new   = false;
  root->left.sexp  = neither;
  root->right.sexp = neither;

  int ret = parse_bytstream(STDIN_FILENO);

  if (ret) {
    write(STDERR_FILENO, MSG("[ !! ] vslisp: error while parsing file\n"));
  }

  return ret;
}
