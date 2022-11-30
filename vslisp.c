// vslisp: a very simple lisp implementation;
//         not bootstrapped

#include <unistd.h>
#include <stdlib.h>

#ifdef DEBUG
#  include <stdio.h>
#endif

#include "vslisp.h"
//#include "prim.h"

static uint paren             = 0;
static uint deep_paren        = 0;
static uint hash_i            = 0;
static char iobuf[IOBLOCK]    = {0};

static struct lisp_hash chash = {0};
static struct lisp_cps   lcps = {0};
static struct lisp_sexp* head = NULL,
                       * root = NULL,
                       * deep = NULL;

struct MEMPOOL(lisp_sexp, SEXPPOOL);
//struct MEMPOOL(lisp_sym_hash, HASHPOOL);

struct pool_ret_t {
  void* mem;
  bool  new; /**
                0 -> in the same pool
                1 -> on a different pool
              */
};

static struct MEMPOOL_TMPL(lisp_sexp)  sexpmp = {0};
static struct MEMPOOL_TMPL(lisp_sexp)* sexpmpp = NULL;
//static struct MEMPOOL_TMPL(lisp_sym_hash) hashmp = {0};

static inline struct pool_ret_t pool_add_node(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
  // TODO: make this an `int' funcion because of
  //       ENOMEM in malloc
  struct pool_ret_t ret = {
    .mem = NULL,
    .new = false,
  };

  if (mpp->used == mpp->total) {
    if (!mpp->next) {
      mpp->next = malloc(mpp->total * sizeof(struct MEMPOOL_TMPL(lisp_sexp)));

      mpp->next->prev  = mpp;
      mpp->next->next  = NULL;
    }

    mpp->next->total = mpp->total;
    mpp->next->used  = 0;
    mpp              = mpp->next;
    ret.new          = true;
  }

  ret.mem = (mpp->mem + mpp->used);
  ++mpp->used;

  return ret;
}
static inline void lisp_sexp_node(struct MEMPOOL_TMPL(lisp_sexp)** mpp) {
  struct MEMPOOL_TMPL(lisp_sexp)* mppv = *mpp;

  struct pool_ret_t pr    = pool_add_node(mppv);
  struct lisp_sexp* nhead = pr.mem;

  if (pr.new) {
    *mpp = pr.mem;
  }

  if (!head) {
    head = root;
    return;
  }

  nhead->root = (struct off_t) {
    .pi = 0,
    .am = (nhead - mppv->mem),
  };

  if (head->left.off.t == __SEXP_EMPTY) {
    head->left = (struct off_t) {
      .t = __SEXP_SEXP,
      .pi = 0,
      .am = (nhead - head),
    };
    head->left.sym = chash;

    nhead->root.am    = (nhead - head); // <- TODO
    deep              = nhead;
  }
  else if (head->right.off.t == __SEXP_EMPTY) {
    head->right = (struct off_t) {
      .t = __SEXP_SEXP,
      .pi = 0,
      .am = (nhead - head),
    };
    head->right.sym = chash;

    nhead->root.am = (nhead - head); // <- TODO
  }
  else {
    /**
       .                      .
      / \            ===>    / \
     .   . <- HEAD          .   =
                               / \
                              .   . <- HEAD
     */
    head              = mppv->mem + head->right.off.am;

    head->left.off.pi = !pr.new;
    //head->left.off.am  = (pr.new)? 0: 1;
    head->left.off.t  = __SEXP_SEXP;
    (head - head->right.off.am)
      ->off.t = __SEXP_LEXP;

    deep = (head + head->left.off.am);

    nhead->root.off.t  = __SEXP_LEXP;
    nhead->root.off.am = (nhead - head);
    head               = nhead;
  }
}
static inline void lisp_sexp_sym(struct MEMPOOL_TMPL(lisp_sexp)** mpp) {
  if (!head) {
#ifdef DEBUG
    fputs("DO: symbol\n", stderr);
#endif
    goto done;
  }

  struct MEMPOOL_TMPL(lisp_sexp)* mppv = *mpp;

  struct pool_ret pr      = pool_add_node(mppv);
  struct lisp_sexp* nhead = pr.mem;

  if (pr.new) {
    *mpp = pr.mem;
  }

  if (head->left.off.t == __SEXP_EMPTY) {
    head->left = (struct off_t) {
      .t  = __SEXP_SYM,
      .pi = 0,
      .am = (nhead - head),
    };
    head->left.sym = chash;
  }
  else if (head->right.off.t == __SEXP_EMPTY) {
    head->right = (struct off_t) {
      .t  = __SEXP_SYM,
      .pi = 0,
      .am = (nhead - head),
    };
    head->right.sym = chash;
  }
  else {
    /**
     (a b                  (a b c
       .  <- HEAD             .
      / \            ===>    / \
     a   b                  a   = <- HEAD
                               / \
                              b   c
     */

    head->right.off.t = __SEXP_LEXP;

    //head->

    head              = (head + head->right.off.am);
  }

done:
  hash_i     = 0;
  chash.len  = 0;
  chash.hash = 0L;
}
static inline void lisp_do_sexp(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
  struct MEMPOOL_TMPL(lisp_sexp)* p = mpp;

  for (; p; p = p->prev) {
    p->used = 0;
  }

  head = NULL;
  mpp  = p;
}
static inline void lisp_sexp_end(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
#ifdef DEBUG
  fputs("DO: sexp\n", stderr);
#endif

  struct lisp_sexp* phead;

  if (head->root.pi != 0) {
    phead = mpp->prev->mem - head->root.am;
  }
  else {
    phead = mpp->mem - head->root.am;
  }

  head = phead;
}
static inline long do_chash(int i, char c) {
  long ret = 0;

  ret   = ((c + i) % 0x100);
  ret <<= ((i*8)   % 0xffffffffffffffff);

  return ret;
}
static inline struct lisp_cps lisp_ev(struct lisp_cps pstat,
                                      enum lisp_pev sev) {
  if (sev & __LISP_SYMBOL_OUT) {
    if (pstat.master & __LISP_SYMBOL_IN) {
#ifdef DEBUG
      fputs("EVENT: symbol out\n", stderr);
#endif
      pstat.master &= ~__LISP_SYMBOL_IN;
      lisp_sexp_sym(&sexpmpp);
    }
    else {
      pstat.slave = 1;
      goto done;
    }
  }
  if (sev & __LISP_PAREN_OUT) {
    if (pstat.master & __LISP_SYMBOL_IN) {
      pstat = lisp_ev(pstat, __LISP_SYMBOL_OUT);
    }

#ifdef DEBUG
    fputs("EVENT: paren out\n", stderr);
#endif

    if (paren) {
      pstat.master &= ~__LISP_PAREN_IN;
      --paren;
      lisp_sexp_end(sexpmpp);
      if (!paren) {
        lisp_do_sexp(sexpmpp);
      }
    }
    else {
      pstat.slave = 1;
      goto done;
    }
  }

 done:
  return pstat;
}
static inline struct lisp_cps lisp_stat(struct lisp_cps pstat,
                                        enum lisp_pstat sstat) {
  if (sstat & __LISP_PAREN_IN) {
    if (pstat.master & __LISP_SYMBOL_IN) {
      pstat = lisp_ev(pstat, __LISP_SYMBOL_OUT);
    }
    lisp_sexp_node(&sexpmpp);
    ++paren;
  }

  pstat.master |= sstat;

  return pstat;
}
static inline struct lisp_cps lisp_whitespace(struct lisp_cps pstat) {
  if (pstat.master & __LISP_SYMBOL_IN) {
    pstat = lisp_ev(pstat, __LISP_SYMBOL_OUT);
  }

  return pstat;
}
static inline struct lisp_cps lisp_csym(struct lisp_cps pstat, char c) {
  pstat.master |= __LISP_SYMBOL_IN;

  if (!__LISP_ALLOWED_IN_NAME(c)) {
    pstat.slave = 1;
    goto done;
  }

  ++chash.len;
  chash.hash += do_chash(hash_i, c);
  ++hash_i;

#ifdef DEBUG
  fprintf(stderr, "vslisp: character (%c) (0x%lx)\n", c, chash.hash);
#endif

 done:
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
  else if (lcps.master) {
    lcps = lisp_whitespace(lcps);
  }

  return ret;
}

int main(void) {
  sexpmp.total       = SEXPPOOL;
  sexpmp.used        = 1;
  sexpmp.prev        = NULL;
  sexpmp.next        = NULL;

  sexpmpp            = &sexpmp;

  head               = NULL;
  root               = sexpmpp->mem;
  sexpmpp->used      = 0;

  root->root.off.pi  = 0;
  root->root.off.t   = __SEXP_ROOT;
  root->left.off.t   = __SEXP_EMPTY;
  root->right.off.t  = __SEXP_EMPTY;

  int ret = parse_bytstream(STDIN_FILENO);

  if (ret) {
    write(STDERR_FILENO,
          MSG("[ !! ] vslisp: error while parsing file\n"));
  }

  return ret;
}
