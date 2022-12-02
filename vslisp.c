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
  void* base;
  bool  new; /**
                0 -> in the same pool
                1 -> on a different pool
              */
};

static struct MEMPOOL_TMPL(lisp_sexp)  sexpmp = {0};
static struct MEMPOOL_TMPL(lisp_sexp)* sexpmpp = NULL;
//static struct MEMPOOL_TMPL(lisp_sym_hash) hashmp = {0};

static inline struct pool_ret_t pool_add_node(struct MEMPOOL_TMPL(lisp_sexp)* mpp) {
  // TODO: make this an `int' function because of
  //       ENOMEM in malloc
  struct pool_ret_t ret = {
    .mem = NULL,
    .base = mpp,
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
static inline struct lisp_sexp* lisp_sexp_node_root(struct MEMPOOL_TMPL(lisp_sexp)* mpp,
                                                    struct lisp_sexp* head) {
  struct lisp_sexp* ret = head;
  struct MEMPOOL_TMPL(lisp_sexp)* pp = mpp;
  int pi = ret->root.pi;

  if (!ret->root.pi) {
    ret = (ret + ret->root.am);
  }
  else {
    if (ret->root.pi > 0) {
      for (uint i = 0; i < pi; i++) {
        pp = pp->next;
      }
    }
    else {
      pi = -pi;
      for (uint i = 0; i < pi; i++) {
        pp = pp->prev;
      }
    }

    ret = (pp->mem + ret->root.am);
  }

  return ret;
}
static inline struct off_t lisp_sexp_node_set(struct pool_ret_t pr,
                                              struct lisp_sexp* old_sexp, struct lisp_sexp* new_sexp) {
  struct off_t ret = {0};

  if (pr.new) {
    // NOTE: this function is generally called on new memory, it's safe to assume positive offset
    ret.pi = 1;
    ret.am = ((struct lisp_sexp*)pr.base - old_sexp);
  }
  else {
    ret.pi = 0;
    ret.am = (new_sexp - old_sexp);
  }

  return ret;
}
static inline struct lisp_sexp* lisp_sexp_node_get(struct MEMPOOL_TMPL(lisp_sexp)* mpp,
                                                   struct lisp_sexp* head, enum sexp_t t) {
  struct lisp_sexp* ret = head;
  struct MEMPOOL_TMPL(lisp_sexp)* pp = mpp;
  int pi = 0;

  if (t & (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)) {
    pi = ret->right.off.pi;
    if (!pi) {
      ret = (ret + ret->right.off.am);
    }
    else {
      if (pi > 0) {
        for (uint i = 0; i < pi; i++) {
          pp = pp->next;
        }
      }
      else {
        pi = -pi;
        for (uint i = 0; i < -pi; i++) {
          pp = pp->prev;
        }
      }
      ret = (pp->mem + ret->right.off.am);
    }
  }
  else if (t & (__SEXP_LEFT_SEXP | __SEXP_LEFT_LEXP)) {
    pi = ret->left.off.pi;
    if (!pi) {
      ret = (ret + ret->left.off.am);
    }
    else {
      if (pi > 0) {
        for (uint i = 0; i < pi; i++) {
          pp = pp->next;
        }
      }
      else {
        pi = -pi;
        for (uint i = 0; i < pi; i++) {
          pp = pp->prev;
        }
      }
      ret = (pp->mem + ret->left.off.am);
    }
  }

  return ret;
}
static void lisp_sexp_node(struct MEMPOOL_TMPL(lisp_sexp)** mpp) {
#define IF_SAME_POOL_INVERT(x) do { if ((x)->root.pi == 0) { (x)->root.am = -(x)->root.am; } } while (0)
  if (!head) {
    head = root;
    return;
  }

  struct MEMPOOL_TMPL(lisp_sexp)* mppv = *mpp;
  struct pool_ret_t pr = pool_add_node(mppv);
  struct lisp_sexp* nhead = pr.mem;

  if (pr.new) {
    mppv = *mpp = pr.mem;
  }

  nhead->root = lisp_sexp_node_set(pr, head, nhead);
  IF_SAME_POOL_INVERT(nhead);

  nhead->t    = (__SEXP_SELF_SEXP | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);

  if (head->t & __SEXP_LEFT_EMPTY) {
    /**
          ? <- HEAD
         /
        . [nhead]
     */
    head->left.off   = lisp_sexp_node_set(pr, head, nhead);
    head->t         &= ~__SEXP_LEFT_EMPTY;
    head->t         |= __SEXP_LEFT_SEXP;
    head             = nhead;
  }
  else if (head->t & __SEXP_RIGHT_EMPTY) {
    /**
          . <- HEAD
         / \
        ?   . [nhead]
     */
    head->right.off  = lisp_sexp_node_set(pr, head, nhead);
    head->t         &= ~__SEXP_RIGHT_EMPTY;
    head->t         |= __SEXP_RIGHT_SEXP;
    head             = nhead;
  }
  else {
    /**
       ? <- HEAD [old_head]           ?
      / \                    ===>    / \
     ?   ?                          ?   =   <- HEAD [lexp_head] `
                                       / \                      `
                        old memory -> ?   .         [nhead]     `- new memory
     */
#define OLD_RIGHT (__SEXP_RIGHT_SYM | __SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)
#define RIGHT_BECOMES_LEFT(x,y) (((x) & (y)) >> 1)

    pr = pool_add_node(mppv);
    struct lisp_sexp* lexp_head = pr.mem;
    struct lisp_sexp  old_head  = *head;

    if (pr.new) {
      mppv = *mpp = pr.mem;
    }

    head->t        &= ~OLD_RIGHT;
    head->t        |= __SEXP_RIGHT_LEXP;

    nhead->root     = lisp_sexp_node_set(pr, lexp_head, nhead);
    IF_SAME_POOL_INVERT(nhead);

    lexp_head->t    = (RIGHT_BECOMES_LEFT(old_head.t, OLD_RIGHT) | __SEXP_RIGHT_SEXP | __SEXP_SELF_LEXP);
    lexp_head->root = lisp_sexp_node_set(pr, head, lexp_head);
    IF_SAME_POOL_INVERT(lexp_head);

    head->right.off = lisp_sexp_node_set(pr, head, lexp_head);

    if (old_head.t & __SEXP_RIGHT_SYM) {
      lexp_head->left.sym = old_head.right.sym;
    }
    else if (old_head.t & RIGHT_CHILD_T) {
      struct lisp_sexp* rch = lisp_sexp_node_get(mppv, head, head->t & RIGHT_CHILD_T);
      lexp_head->left.off   = lisp_sexp_node_set(pr, lexp_head, rch);

      if (!lexp_head->left.off.pi) {
        rch->root.pi = 0;
        rch->root.am = -lexp_head->left.off.am;
      }
      else {
        rch->root.pi = -lexp_head->left.off.pi;
        rch->root.am = lexp_head->left.off.am;
      }
    }

    head = lexp_head;
  }
}
static void lisp_sexp_sym(struct MEMPOOL_TMPL(lisp_sexp)** mpp) {
  if (!head) {
#ifdef DEBUG
    fputs("DO: symbol\n", stderr);
#endif
    goto done;
  }

  struct MEMPOOL_TMPL(lisp_sexp)* mppv = *mpp;
  struct lisp_sexp* nhead              = NULL;

  if (head->t & __SEXP_LEFT_EMPTY) {
    /**
          ? <- HEAD
         /
        *
     */
    head->t         &= ~__SEXP_LEFT_EMPTY;
    head->t         |= __SEXP_LEFT_SYM;
    head->left.sym   = chash;
  }
  else if (head->t == __SEXP_RIGHT_EMPTY) {
    /**
          ? <- HEAD
         / \
        ?   *
     */
    head->t         &= ~__SEXP_RIGHT_EMPTY;
    head->t         |= __SEXP_RIGHT_SYM;
    head->right.sym  = chash;
  }
  else {
    /**
       ?  <- HEAD             ?
      / \            ===>    / \
     ?   ?                  ?   = <- HEAD [nhead]
                               / \
                              ?   *
     */
#define OLD_RIGHT (__SEXP_RIGHT_SYM | __SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)

    struct lisp_sexp ohead = *head;
    struct pool_ret_t pr   = pool_add_node(mppv);

    if (pr.new) {
      *mpp          = pr.mem;
    }

    head->t        &= ~OLD_RIGHT;
    head->t        |= __SEXP_RIGHT_LEXP;

    nhead           = pr.mem;
    nhead->root.am  = (nhead - mppv->mem);
    nhead->t        = (((ohead.t & OLD_RIGHT) << 1) | __SEXP_RIGHT_SYM);

    if (ohead.t & __SEXP_RIGHT_SYM) {
      nhead->left.sym = ohead.right.sym;
    }
    else if (ohead.t & (__SEXP_RIGHT_SEXP | __SEXP_RIGHT_LEXP)) {
    }

    nhead->right.sym = chash;

    head = nhead;
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
  fputs("OUT: sexp\n", stderr);
#endif

  struct lisp_sexp* phead = head;

  for (;;) {
    if (!phead->root.pi) {
      phead = (phead + phead->root.am);
    }
    else {
      phead = (lisp_sexp_node_root(mpp, phead) + phead->root.am);
    }

    if (!(phead->t & __SEXP_SELF_LEXP)) {
      break;
    }
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

#ifdef DEBUG
static char debug_hash_buf[sizeof(ulong)];

static inline char* debug_symstr(ulong hash) {
  *(ulong*) debug_hash_buf = hash;
}

static void debug_ast(struct lisp_sexp* h, uint i) {
  ulong left_str, right_str;

  for (uint i = 0; i < (sizeof(sexpmp.mem)/sizeof(*sexpmp.mem))/2; i++) {
    struct lisp_sexp sexp = sexpmp.mem[i];

    if (sexp.t & (__SEXP_LEFT_SYM)) {
      left_str = sexp.left.sym.hash;
    }
    else {
      left_str = sexp.left.off.am;
    }

    if (sexp.t & (__SEXP_RIGHT_SYM)) {
      right_str = sexp.right.sym.hash;
    }
    else {
      right_str = sexp.left.off.am;
    }

    fprintf(stderr, "[%d] t = %x, left = %lx, right = %lx\n", i,
            sexp.t, left_str, right_str);
  }
}
#endif

int main(void) {
  sexpmp.total      = SEXPPOOL;
  sexpmp.used       = 1;
  sexpmp.prev       = NULL;
  sexpmp.next       = NULL;

  sexpmpp           = &sexpmp;

  head              = NULL;
  root              = sexpmpp->mem;
  sexpmpp->used     = 1;

  root->root.pi     = 0;
  root->t           = (__SEXP_SELF_ROOT | __SEXP_LEFT_EMPTY | __SEXP_RIGHT_EMPTY);

  int ret = parse_bytstream(STDIN_FILENO);

  if (ret) {
    write(STDERR_FILENO,
          MSG("[ !! ] vslisp: error while parsing file\n"));
  }
  else {
#ifdef DEBUG
    debug_ast(root, 0);
#endif
  }

  return ret;
}
